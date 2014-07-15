/*
 * CodeSegments.c
 *
 *  Created on: 16 Apr 2014
 *      Author: rjhender
 */

#include "CodeSegments.h"
#include "InstructionSetMIPS4.h"
#include "InstructionSet.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "memory.h"

//-------------------------------------------------------------------

static code_segment_data_t segmentData;
static uint8_t* CodeSegBounds;
static uint32_t GlobalLiteralCount = 0;

//-------------------------------------------------------------------

static uint32_t CountRegisers(uint32_t *bitfields)
{
	int x, y;
	uint32_t c = 0;

	for (y=0; y < 3; y ++)
	{
		for (x=0; x < 32; x++)
		{
			if (bitfields[y] & (1<<x)) c++;
		}
	}
	return c;
}

static void freeLiterals(code_seg_t* codeSegment)
{
	literal_t *prev;
	literal_t *next;

	//remove any existing literals
	if (codeSegment->literals)
	{
		prev = codeSegment->literals;

		while (prev)
		{
			next = prev->next;
			free(prev);
			prev = next;
		}
	}
	codeSegment->literals = NULL;
}

static literal_t* newLiteral(uint32_t value)
{
	literal_t* newLiteral = malloc(sizeof(literal_t));

	newLiteral->value = value;
	newLiteral->next = NULL;

	return newLiteral;
}

//-------------------------------------------------------------------

/*
 * TODO what if segment length means offset > 4096?
 */
uint32_t addLiteral(code_seg_t* codeSegment, reg_t* base, uint32_t* offset, uint32_t value)
{
	int index = 1;

	if (SEG_SANDWICH == codeSegment->Type)
	{
		int x;
		for (x=1; x < 1024; x++)		//Scan existing global literals for Value
		{
			if (*((uint32_t*)MMAP_FP_BASE - x) == value)
			{
				*offset = x;
				*base = REG_HOST_FP;
				return 0;
			}
		}

		if (GlobalLiteralCount >= 1024)
		{
			printf("CodeSegments.c:%d Run out of Global Literal positions\n", __LINE__);
		}

		*offset = -(GlobalLiteralCount+1)*4;
		*base = REG_HOST_FP;
		*((uint32_t*)MMAP_FP_BASE - GlobalLiteralCount-1) = value;
		GlobalLiteralCount++;
		return 0;
	}

	if (codeSegment->literals)
	{
		literal_t* nxt = codeSegment->literals;
		while (nxt->next)
		{
			index++;
			nxt = nxt->next;
		}
		nxt->next = newLiteral(value);
		*offset = index;
		*base = REG_HOST_PC;
	}
	else
	{
		codeSegment->literals = newLiteral(value);
		*offset = 0;
		*base = REG_HOST_PC;
	}

	return 0;
}


code_seg_t* newSegment()
{
	code_seg_t* newSeg = malloc(sizeof(code_seg_t));

	memset(newSeg, 0,sizeof(code_seg_t));

	return newSeg;
}

uint32_t delSegment(code_seg_t* codeSegment)
{
	uint32_t ret = 0;

	freeIntermediateInstructions(codeSegment);
	freeLiterals(codeSegment);
	free(codeSegment);

	return ret;
}

void freeIntermediateInstructions(code_seg_t* codeSegment)
{
	Instruction_t *prevInstruction;
	Instruction_t *nextInstruction;

	//remove any existing Intermediate code
	if (codeSegment->Intermcode)
	{
		prevInstruction = codeSegment->Intermcode;

		while (prevInstruction)
		{
			nextInstruction = prevInstruction->nextInstruction;
			free(prevInstruction);
			prevInstruction = nextInstruction;
		}
	}
	codeSegment->Intermcode = NULL;

	freeLiterals(codeSegment);
}


static void AddSegmentToLinkedList(code_seg_t* newSeg)
{
	code_seg_t* seg;
	code_seg_t** pseg;

	newSeg->next = NULL;

	//TODO dynamic
	seg = segmentData.StaticSegments;
	pseg = &segmentData.StaticSegments;

	if (seg == NULL)
	{
		*pseg = newSeg;
	}
	else if (seg->next == NULL)
	{
		if ((*pseg)->MIPScode < newSeg->MIPScode)
		{
			(*pseg)->next = newSeg;
		}else
		{
			newSeg->next = *pseg;
			*pseg = newSeg;
		}
	}
	else
	{
		while ((seg->next) && (seg->next->MIPScode < newSeg->MIPScode))
		{
			seg = seg->next;
		}

		// seg->next will either be NULL or seg->next->MIPScode is greater than newSeg->MIPScode
		newSeg->next = seg->next;
		seg->next = newSeg;
	}
}

/*
 * Generate Code block validity
 *
 * Scan memory for code segments.
 * */

static int32_t UpdateCodeBlockValidity(int8_t* Block, uint32_t* address, uint32_t length)
{
	int32_t x, y;
	uint32_t prevWordCode = 0;
	code_seg_t* newSeg;

	int32_t SegmentsCreated = 0;

	for (x=0; x < length/4; x++)
	{
		for (y = x; y < length/4; y++)
		{
			Instruction_e op;
			op = ops_type(address[y]);

			//we are not in valid code
			if (INVALID == op)
			{
				prevWordCode = 0;
				break;
			}

			if ((op & OPS_JUMP) == OPS_JUMP)
			{
				newSeg = newSegment();
				newSeg->MIPScode = address + x;
				newSeg->MIPScodeLen = y - x + 1;

				if (ops_type(*address) == JR) //only JR can set PC to the Link Register (or other register!)
					newSeg->MIPSReturnRegister = (*address>>21)&0x1f;

				if (!prevWordCode)
					newSeg->Type = SEG_ALONE;
				else
					newSeg->Type = SEG_END;

				SegmentsCreated++;
				AddSegmentToLinkedList(newSeg);

				prevWordCode = 0;
				break;
			}
			else if(((op & OPS_CALL) == OPS_CALL)
					|| ((op & OPS_BRANCH) == OPS_BRANCH)	//MIPS does not have an unconditional branch
			)
			{
				newSeg = newSegment();

				int32_t offset =  ops_JumpAddressOffset(&address[y]);

				//Is this an internal branch - need to create two segments
				// TODO is this <= or < for x < y + offset?
				if (offset < 0 && x <= y + offset)
				{
					newSeg->MIPScode = address + x;
					newSeg->MIPScodeLen = y - x + offset + 1;

					if (!prevWordCode)
						newSeg->Type = SEG_START;
					else
						newSeg->Type = SEG_SANDWICH;
					SegmentsCreated++;
					AddSegmentToLinkedList(newSeg);

					newSeg = newSegment();
					newSeg->MIPScode = address + y + offset + 1;
					newSeg->MIPScodeLen = -offset;
					newSeg->Type = SEG_SANDWICH;
					SegmentsCreated++;
					AddSegmentToLinkedList(newSeg);

				}
				else // TODO what if we are branching external to the block?
				{
					newSeg->MIPScode = address + x;
					newSeg->MIPScodeLen = y - x + 1;

					if (!prevWordCode)
						newSeg->Type = SEG_START;
					else
						newSeg->Type = SEG_SANDWICH;
					SegmentsCreated++;
					AddSegmentToLinkedList(newSeg);
				}

				prevWordCode = 1;
				break;
			}
		} // for (y = x; y < length/4; y++)
		x = y;
	} // for (x=0; x < length/4; x++)
	return SegmentsCreated;
}

/*
 * TODO check if linking to an instruction that is NOT the first in a segment
 */
static void LinkStaticSegments()
{
	code_seg_t* seg;
	code_seg_t* searchSeg;
	Instruction_e op;
	uint32_t* word;


	seg = segmentData.StaticSegments;

	while (seg)
	{
		word = (seg->MIPScode + seg->MIPScodeLen - 1);
		
		op = ops_type(*word);
		//This segment could branch to itself or another
		int32_t offset;

		if ((op & OPS_JUMP) == OPS_JUMP)
		{
			searchSeg = segmentData.StaticSegments;

			offset =  ops_JumpAddressOffset(word);

			if (ops_type(*word) == JR) //only JR can set PC to the Link Register (or other register!)
					//(*(seg->MIPScode + seg->MIPScodeLen-1)>>21)&0x1f;

			while (searchSeg)
			{
				if (((uint32_t*)(((uint32_t)seg->MIPScode)&0xfc000000) + offset) == searchSeg->MIPScode)
				{
					seg->pBranchNext = searchSeg;
					break;
				}
				searchSeg = searchSeg->next;
			}

		}
		else if(((op & OPS_CALL) == OPS_CALL)
				|| ((op & OPS_BRANCH) == OPS_BRANCH)	//MIPS does not have an unconditional branch
		)
		{
			offset =  ops_JumpAddressOffset(word);
			if (-offset == seg->MIPScodeLen)
			{
				seg->pBranchNext = seg;
			}
			else
			{
				searchSeg = segmentData.StaticSegments;

				while (searchSeg)
				{
					if (word + offset == searchSeg->MIPScode)
					{
						seg->pBranchNext = searchSeg;
						break;
					}
					searchSeg = searchSeg->next;
				}
			}
			seg->pContinueNext = seg->next;
		}
		else // this must be a continue only segment
		{
			seg->pContinueNext = seg->next;
		}
		seg = seg->next;
	}

}

/*
 * Function to scan address range to find MIPS code.
 * The address could be an emulated virtual addresses
 *
 *  1. If the addres
 * 	1. Invalidate any code_segments that have changed
 * 	2. Generate new code_segments for address range
 *
 * 	Returns number of Segments Added (+)/Removed (-)
 *
 */
int32_t ScanForCode(uint32_t* address, uint32_t length)
{
	uint32_t* addr = address;

	int8_t* Bounds;

	switch ((((uint32_t)address)>>23) & 0xFF)
	{
	case 0x80:
	case 0xA0:
		Bounds = segmentData.DynamicBounds;
		addr = (uint32_t*)((uint32_t)address & 0x80FFFFFF);
		break;
	case 0x88:
		Bounds = segmentData.StaticBounds;
		addr = (uint32_t*)((uint32_t)address & 0x88FFFFFF);
		break;
	case 0x90:
		printf("PIF boot ROM: ScanForCode()\n");
		return 0;
	default:
		Bounds = segmentData.DynamicBounds;
	}

	return UpdateCodeBlockValidity(Bounds, addr, length);
}

/*
 * Function to Generate a code_segment_data structure
 *
 * It assumes memory has been mapped (at 0x80000000) and the ROM suitably copied into 0x88000000
 */
code_segment_data_t* GenerateCodeSegmentData(int32_t ROMsize)
{
	segmentData.StaticSegments = NULL;
	segmentData.DynamicSegments = NULL;

	//segmentData.StaticBounds = malloc(ROMsize/sizeof(*segmentData.StaticBounds));
	//segmentData.DynamicBounds = malloc(RD_RAM_SIZE/sizeof(*segmentData.DynamicBounds));

	//memset(segmentData.StaticBounds, BLOCK_INVALID, ROMsize/sizeof(*segmentData.StaticBounds));
	//memset(segmentData.DynamicBounds, BLOCK_INVALID, RD_RAM_SIZE/sizeof(*segmentData.DynamicBounds));

	//TODO not scanning entire ROM!!!
	segmentData.count = ScanForCode((uint32_t*)(ROM_ADDRESS+64), ROMsize-64);

	LinkStaticSegments();

	return &segmentData;

}
