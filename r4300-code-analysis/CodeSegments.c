/*
 * CodeSegments.c
 *
 *  Created on: 16 Apr 2014
 *      Author: rjhender
 */

#include "CodeSegments.h"
#include "InstructionSetMIPS4.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static code_segment_data_t segmentData;
static uint8_t* CodeSegBounds;


static uint32_t CountRegisers(uint64_t bitfield)
{
	int x;
	uint32_t c = 0;
	for (x=0; x < 64; x++)
	{
		if (bitfield&0x1) c++;
		bitfield = bitfield >> 1;
	}
	return c;
}

code_segment_data_t* GenerateCodeSegmentData(uint32_t* ROM, uint32_t size)
{
	int x,y;
	uint32_t segmentCount = 0;

	//find the number of segments

	int32_t iStart = 0;
	
	code_seg_t* prevCodeSeg = NULL;
	code_seg_t* nextCodeSeg;

	segmentData.FirstSegment = NULL;

	CodeSegBounds = malloc(size/4);
	memset(CodeSegBounds, BLOCK_INVALID, size/4);
	
	/*
	 * Generate Code block validity
	 *
	 * Scan the file for code blocks and ensure there are no invalid instructions in it.
	 *
	 * */
	for (x=64/4; x< size/4; x++)
	{
		uint32_t bValidBlock = 1;
		
		for (y = x; y < size/4; y++)
		{
			int op = ops_type(ROM[y]);

			//we are not in valid code
			if (INVALID == op)
			{
				bValidBlock = 0;
				break;
			}
			if ((op & OPS_JUMP) == OPS_JUMP)
			{
				CodeSegBounds[y] = BLOCK_END;
				//if (x < 400) printf("Segment at 0x%08X to 0x%08X (%d) j\n", (x)*4, y*4, x-6);
				break;
			}
			else if(((op & OPS_CALL) == OPS_CALL)
				|| ((op & OPS_BRANCH) == OPS_BRANCH)	//MIPS does not have an unconditional branch
				)
			{
				CodeSegBounds[y] = BLOCK_CONTINUES;
				//if (x < 400) printf("Segment at 0x%08X to 0x%08X (%d) b\n", (x)*4, y*4, x-6);
				break;
			}
		}

		//mark block as valid
		if (bValidBlock)
		{
			if (y - x > 0) memset(&CodeSegBounds[x], BLOCK_PART, y - x );
			x = y;
		}
	}

	/*
	 * Build CodeSegments using map
	 *
	 * There may be branches within a segment that branch to other code segments.
	 * If this happens then the segment needs to be split.
	 * */

	iStart = 64/4;

	for (x=64/4; x< size/4; x++)
	{
		//if in invalid block then continue scanning
		if (CodeSegBounds[x] == BLOCK_INVALID){
			iStart = x+1;
			continue;
		}

		if (CodeSegBounds[x] == BLOCK_PART)	continue;

		//we have reached the end of the segment
		if (CodeSegBounds[x] == BLOCK_END)
		{
			int32_t offset =  ops_JumpAddressOffset(ROM[x]);

			nextCodeSeg = malloc(sizeof(code_seg_t));
			if (!segmentCount) segmentData.FirstSegment = nextCodeSeg;

			nextCodeSeg->MIPScode = iStart*4;
			nextCodeSeg->MIPScodeLen = x - iStart +1;
			nextCodeSeg->ARMcodeLen = 0;
			nextCodeSeg->MIPSnextInstructionIndex = offset;
			nextCodeSeg->blockType = BLOCK_END;

			if (ops_type(ROM[x]) == JR) //only JR can set PC to the Link Register (or other register!)
			{
				nextCodeSeg->MIPSReturnRegister = (ROM[x]>>21)&0x1f;
			}
			else
			{
				nextCodeSeg->MIPSReturnRegister = 0;
			}

			nextCodeSeg->nextCodeSegmentLinkedList = NULL;
			if (prevCodeSeg) prevCodeSeg->nextCodeSegmentLinkedList = nextCodeSeg;
			prevCodeSeg = nextCodeSeg;
			segmentCount++;

			iStart = x+1;
		}

		//check to see if instruction is a branch and if it stays local, to segment
		else if (CodeSegBounds[x] == BLOCK_CONTINUES)
		{
			//nextSegment = findNextSegment(x, size);

			int32_t offset =  ops_JumpAddressOffset(ROM[x]);

			nextCodeSeg = malloc(sizeof(code_seg_t));
			if (!segmentCount) segmentData.FirstSegment = nextCodeSeg;

			//is this a loop currently in a segment?
			if (offset < 0 && (x + offset > iStart) )
			{
				nextCodeSeg->MIPScode = (iStart) * 4;
				nextCodeSeg->MIPScodeLen = x + offset - iStart;
				nextCodeSeg->MIPSnextInstructionIndex = (x + offset)*4;
				nextCodeSeg->MIPSReturnRegister = 0;

				nextCodeSeg->ARMcodeLen = 0;
				nextCodeSeg->blockType = BLOCK_CONTINUES;


				nextCodeSeg->nextCodeSegmentLinkedList = NULL;
				if (prevCodeSeg) prevCodeSeg->nextCodeSegmentLinkedList = nextCodeSeg;
				prevCodeSeg = nextCodeSeg;

				segmentCount++;

				nextCodeSeg = malloc(sizeof(code_seg_t));

				nextCodeSeg->MIPScode = (x + offset) * 4;
				nextCodeSeg->MIPScodeLen = -offset + 1;
				nextCodeSeg->MIPSnextInstructionIndex = (x + 1)*4;
				nextCodeSeg->ARMcodeLen = 0;
				nextCodeSeg->MIPSReturnRegister = 0;
				nextCodeSeg->blockType = BLOCK_CONTINUES;
			}

			else //must branch to another segment
			{
				nextCodeSeg->MIPScode = (iStart) * 4;
				nextCodeSeg->MIPScodeLen = x - iStart+1;
				nextCodeSeg->MIPSnextInstructionIndex = (x + offset)*4;
				nextCodeSeg->ARMcodeLen = 0;
				nextCodeSeg->MIPSReturnRegister = 0;

				nextCodeSeg->blockType = BLOCK_CONTINUES;
			}

			nextCodeSeg->nextCodeSegmentLinkedList = NULL;
			if (prevCodeSeg) prevCodeSeg->nextCodeSegmentLinkedList = nextCodeSeg;
			prevCodeSeg = nextCodeSeg;

			segmentCount++;

			iStart = x+1;
		}
	}

	//update the count of segments
	segmentData.count = segmentCount;


	/*
	 * Generate the register usage for the MIPS code.
	 */
	nextCodeSeg = segmentData.FirstSegment;
	while (nextCodeSeg != NULL)
	{
		nextCodeSeg->MIPSRegistersUsed = 0;
		for (x=0; x < nextCodeSeg->MIPScodeLen; x++)
		{
			nextCodeSeg->MIPSRegistersUsed |= ops_regs_used(ROM[nextCodeSeg->MIPScode/4 + x]);
		}

		nextCodeSeg->MIPSRegistersUsedCount = CountRegisers(nextCodeSeg->MIPSRegistersUsed);
		nextCodeSeg = nextCodeSeg->nextCodeSegmentLinkedList;
	}

	return &segmentData;
}
