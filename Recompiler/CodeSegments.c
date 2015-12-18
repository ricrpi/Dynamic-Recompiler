/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *                                                                         *
 *  Dynamic-Recompiler - Turns MIPS code into ARM code                       *
 *  Original source: http://github.com/ricrpi/Dynamic-Recompiler             *
 *  Copyright (C) 2015  Richard Hender                                       *
 *                                                                           *
 *  This program is free software: you can redistribute it and/or modify     *
 *  it under the terms of the GNU General Public License as published by     *
 *  the Free Software Foundation, either version 3 of the License, or        *
 *  (at your option) any later version.                                      *
 *                                                                           *
 *  This program is distributed in the hope that it will be useful,          *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *  GNU General Public License for more details.                             *
 *                                                                           *
 *  You should have received a copy of the GNU General Public License        *
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.    *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "CodeSegments.h"
#include "InstructionSetMIPS4.h"
#include "InstructionSetARM6hf.h"
#include "InstructionSet.h"
#include "Translate.h"

#include "memory.h"
#include "DebugDefines.h"

#include "mem_state.h"
#include "callers.h"

//==================================================================

code_segment_data_t segmentData;
uint32_t showPrintSegmentDelete = 1U;

//==================================================================

/*
 * Description: Function to overwrite the Branch instruction at the end of ARM code so that
 * 				it points to FUNC_GEN_BRANCH_UNKNOWN
 *
 * Parameters:	const code_seg_t* codeSegment		The segment to invalidate the branch on
 *
 */
void invalidateBranch(const code_seg_t* codeSegment)
{
	if (NULL != codeSegment->ARMEntryPoint
			&& codeSegment->MIPScode != NULL
			&& codeSegment->MIPScodeLen > 0U)
	{
		uint32_t* const			out 			= (uint32_t*)codeSegment->ARMcode + codeSegment->ARMcodeLen -1U;
		const size_t 			targetAddress 	= *((size_t*)(MMAP_FP_BASE + FUNC_GEN_BRANCH_UNKNOWN));
		Instruction_t*			ins 			= newEmptyInstr();

#if SHOW_CALLER
		printf("invalidateBranch(0x%08x) at 0x%08x\n", (uint32_t)codeSegment, (uint32_t)out);
#endif

#if SHOW_BRANCHUNKNOWN_STEPS
		printf_arm((uint32_t)out, *out);
#endif
		//Get MIPS condition code for branch
		mips_decode(*(codeSegment->MIPScode + codeSegment->MIPScodeLen -1), ins);

#if SHOW_BRANCHUNKNOWN_STEPS
		printf_Intermediate(ins, 1);
#endif

		//Set instruction to ARM_BRANCH for new target
		InstrB(ins, ins->cond, targetAddress, 1);

		//emit the arm code
		*out = arm_encode(ins, (size_t)out);

		InstrFree(NULL, ins);
	}
}

//=================== Intermediate Code ===============================

/*
 * Description: Function to free the intermediate instructions and literals used in code generation.
 * 				This function can be called once the output code has been emitted.
 *
 * Parameters:	code_seg_t* const codeSegment		The segment to clean
 *
 */
void freeIntermediateInstructions(code_seg_t* const codeSegment)
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

//================ Segment Generation/Linking =========================

/*
 * Description: Function to malloc a newSegment and zero initialize it.
 *
 * Parameters:	none
 *
 * Returns:		code_seg_t*
 */
code_seg_t* newSegment()
{
	code_seg_t* newSeg = malloc(sizeof(code_seg_t));
	memset(newSeg, 0,sizeof(code_seg_t));

#if SHOW_PRINT_SEGMENT_DELETE == 2
	{
#elif SHOW_PRINT_SEGMENT_DELETE == 1
	if (showPrintSegmentDelete)
	{
#else
	if (0)
	{
#endif
		printf("new Segment 0x%08x\n", (uint32_t)newSeg);
	}

	return newSeg;
}

/*
 * Description: Function to free a segment and its associated data.
 *
 * 				Function will also invalidate the branches of code segments that branch to this one
 * 				and delete segments that 'continue' execution to this one.
 *
 * Parameters:	none
 *
 * Returns:		code_seg_t*
 */
uint32_t delSegment(code_seg_t* codeSegment)
{
	uint32_t ret = 0;

#if SHOW_PRINT_SEGMENT_DELETE == 2
	{
#elif SHOW_PRINT_SEGMENT_DELETE == 1
	if (showPrintSegmentDelete)
	{
#else
	if (0)
	{
#endif
	printf("deleting Segment 0x%08x for mips code at 0x%08x\n", (uint32_t)codeSegment, (uint32_t)codeSegment->MIPScode);
	}

	freeIntermediateInstructions(codeSegment);	// free memory used for Intermediate Instructions
	freeLiterals(codeSegment);					// free memory used by literals

	// Clean up all segments that branch or continue to this segment
	if (codeSegment->callers != NULL)
	{
		caller_t* caller = codeSegment->callers;

		// loop through all the callers
		while (caller != NULL)
		{
			if (caller->codeSeg != NULL)
			{
				// if the found caller branches to this segment then remove the reference
				if (caller->codeSeg->pBranchNext == codeSegment)
				{
					caller->codeSeg->pBranchNext = NULL;
				}
				// if the found caller continues to this segment then it MUST also be deleted
				else if (caller->codeSeg->pContinueNext == codeSegment)
				{
					delSegment(caller->codeSeg);
				}
			}
			caller = caller->next;
		}

		// Invalidate any segments that branch to this segment and free the caller_t objects
		freeCallers(codeSegment);
	}

	// update the codeSegment mapping
	setMemState((size_t)codeSegment->MIPScode, codeSegment->MIPScodeLen, NULL);

	free(codeSegment);

	return ret;
}

/*
 * Description: Function to Compile the code starting at 'address'.
 *
 * 				This function will scan the code until an unconditional Jump is found. Referred to as a super block
 *
 * 				It may detect multiple segments in which case these will all 'continue' execution into the next segment.
 *
 * 				It also detects internal loops and separates these into their own segment, for better register optimization.
 *
 *
 * Parameters:	const uint32_t* const address				The address to start compiling from.
 *
 * Returns:		code_seg_t*	The first generated code Segment.
 */
code_seg_t* CompileCodeAt(const uint32_t const address[])
{
	uint32_t 		x;
	Instruction_e 	op;
	uint32_t 		uiMIPScodeLen = 0U;
	code_seg_t* 	newSeg;
	code_seg_t* 	prevSeg = NULL;

	// Find the shortest length of contiguous blocks (super block) of MIPS code at 'address'
	// Contiguous blocks should end with an OPS_JUMP && !OPS_LINK
	for (;;) // (index + uiMIPScodeLen < upperAddress/sizeof(code_seg_t*))
	{
		op = ops_type(address[uiMIPScodeLen]);

		if (DR_INVALID == op)
		{
			uiMIPScodeLen = 0U;
			break;
		}

		uiMIPScodeLen++;

		if ((op & OPS_JUMP) == OPS_JUMP
				&& (op & OPS_LINK) != OPS_LINK)	//unconditional jump or function return
		{
			break;
		}
	}

#if	SHOW_COMPILEAT_STEPS
	printf("CompiltCodeAt(0x%x) - found %u potential instructions\n", address, uiMIPScodeLen);
#endif

	//Now expire the segments within the super block and remove the segments
	for (x = 0U; x < uiMIPScodeLen; x++)
	{
		code_seg_t* toDelete;
		toDelete = getSegmentAt((size_t)(address + x));
		if (toDelete)
		{
			setMemState((size_t)toDelete->MIPScode, toDelete->MIPScodeLen, NULL);
			delSegment(toDelete);

			//we can skip next few words as we have already cleared them.
			x += toDelete->MIPScodeLen - 1U;
		}
	}

	uint32_t segmentStartIndex = 0U;

	// Create new segments
	for (x = 0U; x < uiMIPScodeLen; x++)
	{
		op = ops_type(address[x]);

		if ((op & OPS_JUMP) == OPS_JUMP)
		{
			//uint32_t uiAddress = ops_JumpAddress(&address[x]);

			newSeg = newSegment();
			newSeg->MIPScode = (uint32_t*)(address + segmentStartIndex);
			newSeg->MIPScodeLen = x - segmentStartIndex + 1U;

#if	SHOW_COMPILEAT_STEPS
				printf("CompiltCodeAt(0x%x) - new external jump segment 0x%x at address 0x%x\n", address + x * 4U, newSeg, newSeg->MIPScode);
#endif
			if (op == MIPS_JR) //only JR can set PC to the Link Register (or other register!)
			{
				newSeg->MIPSReturnRegister = (*address>>21U)&0x1fU;
			}

			if (prevSeg)
			{
				prevSeg->pContinueNext = newSeg;
				addToCallers(prevSeg, newSeg);
			}
			prevSeg = newSeg;

			if (!segmentStartIndex)
			{
				newSeg->Type = SEG_START;
			}
			else
			{
				newSeg->Type = SEG_END;
			}

			setMemState((size_t)(address + segmentStartIndex), newSeg->MIPScodeLen, newSeg);

			segmentStartIndex = x + 1U;

			// we should have got to the end of the super block
			if (!(op & OPS_LINK))
			{
				break;
			}
		}
		else if((op & OPS_BRANCH) == OPS_BRANCH)	//MIPS does not have an unconditional branch
		{
			int32_t offset =  ops_BranchOffset(&address[x]);

			//Is this an internal branch - need to create two segments
			// if use x<= y + offset then may throw SIGSEGV if offset is -1!
			if (offset < 0 && x + offset >= segmentStartIndex )
			{
				newSeg = newSegment();
				newSeg->MIPScode = (uint32_t*)(address + segmentStartIndex);
				newSeg->MIPScodeLen = x + offset - segmentStartIndex + 1U;

#if	SHOW_COMPILEAT_STEPS
				printf("CompiltCodeAt(0x%x) - new internal branching, pre-loop segment 0x%x to address 0x%x\n", address, newSeg, newSeg->MIPScode);
#endif

				if (prevSeg != NULL)
				{
					prevSeg->pContinueNext = newSeg;
					addToCallers(prevSeg, newSeg);
				}
				prevSeg = newSeg;

				if (segmentStartIndex == 0U)
				{
					newSeg->Type = SEG_START;
				}
				else
				{
					newSeg->Type = SEG_SANDWICH;
				}

				setMemState((size_t)(address + segmentStartIndex), newSeg->MIPScodeLen, newSeg);
				segmentStartIndex = x + 1U;

				newSeg = newSegment();
				newSeg->MIPScode = (uint32_t*)(address + segmentStartIndex + offset);
				newSeg->MIPScodeLen = -offset;
				newSeg->Type = SEG_SANDWICH;

#if	SHOW_COMPILEAT_STEPS
				printf("CompiltCodeAt(0x%x) - new internal branching self-looping segment 0x%x to address 0x%x\n", address, newSeg, newSeg->MIPScode);
#endif
				prevSeg->pContinueNext = newSeg;
				addToCallers(prevSeg, newSeg);
				prevSeg = newSeg;

				setMemState((size_t)(address + segmentStartIndex + offset), newSeg->MIPScodeLen, newSeg);
				segmentStartIndex = x + 1U;
			}
			else // if we are branching external to the block?
			{
				newSeg = newSegment();
				newSeg->MIPScode = (uint32_t*)(address + segmentStartIndex);
				newSeg->MIPScodeLen = x - segmentStartIndex + 1U;
#if	SHOW_COMPILEAT_STEPS
				printf("CompiltCodeAt(0x%x) - new external branching segment 0x%x to address 0x%x\n", address, newSeg, newSeg->MIPScode);
#endif
				setMemState((size_t)(address + segmentStartIndex), newSeg->MIPScodeLen, newSeg);

				//
				if (prevSeg != NULL)
				{
					prevSeg->pContinueNext = newSeg;
					addToCallers(prevSeg, newSeg);
				}

#if 1
				code_seg_t* externalSegment = getSegmentAt((uint32_t)(address + segmentStartIndex + offset));
				if (externalSegment != NULL)
				{

#if	SHOW_COMPILEAT_STEPS
					printf("CompiltCodeAt(0x%x) - segment 0x%x branches to segment 0x%x to address \n", address, newSeg, externalSegment, externalSegment->MIPScode);
#endif
					newSeg->pBranchNext = externalSegment;
					// addToCallers() will be done when compiling code within C_Interface.c:branchUnknown()
				}
#endif
				prevSeg = newSeg;

				if (segmentStartIndex == 0U)
				{
					newSeg->Type = SEG_START;
				}
				else
				{
					newSeg->Type = SEG_SANDWICH;
				}

				segmentStartIndex = x + 1U;
			}
		}
	} // for (x = 0; x < uiMIPScodeLen; x++)

	newSeg = getSegmentAt((size_t)address);

	if (newSeg)
	{
		// Now we can translate and emit code to the next 'break' in instructions
		while (newSeg->pContinueNext)
		{
			segmentData.dbgCurrentSegment = newSeg;
			Translate(newSeg);
#if	SHOW_COMPILEAT_STEPS
			printf("CompiltCodeAt(0x%x) - Translating segment      0x%x\n", address, newSeg);
#endif
			newSeg = newSeg->pContinueNext;
		}

		segmentData.dbgCurrentSegment = newSeg;
		Translate(newSeg);
#if	SHOW_COMPILEAT_STEPS
			printf("CompiltCodeAt(0x%x) - Translating last segment 0x%x\n", address, newSeg);
#endif
	}
	else
	{
		printf("CompileCodeAt() failed for address 0x%08x\n", (uint32_t)address);
	}

	return getSegmentAt((size_t)address);
}

/*
 * Description:	Function to Generate a code_segment_data structure
 *
 * 				It assumes memory has been mapped (at 0x80000000) and the ROM suitably copied into 0x88000000
 *
 * Returns:		code_segment_data_t*			A pointer to the code_segment_data object
 */
code_segment_data_t* GenerateCodeSegmentData(const int32_t ROMsize)
{
	static uint32_t bCalled = 0U;

	if (bCalled == 0U)
	{
		bCalled = 1U;
		memMap_t Blocks[2];

		Blocks[0].address	= 0x88000000;
		Blocks[0].size		= ROMsize;

		Blocks[1].address	= 0x80000000;
		Blocks[1].size		= RD_RAM_SIZE;

		//Initialize the target memory mapping
		initMemState(Blocks, sizeof(Blocks)/sizeof(memMap_t));


		segmentData.segStart = Generate_CodeStart(&segmentData);
		emit_arm_code(segmentData.segStart);
		*((uint32_t*)(MMAP_FP_BASE + FUNC_GEN_START)) = (uint32_t)segmentData.segStart->ARMEntryPoint;

		segmentData.segStop = Generate_CodeStop(&segmentData);
		emit_arm_code(segmentData.segStop);
		*((uint32_t*)(MMAP_FP_BASE + FUNC_GEN_STOP)) = (uint32_t)segmentData.segStop->ARMEntryPoint;

		segmentData.segBranchUnknown = Generate_BranchUnknown(&segmentData);
		emit_arm_code(segmentData.segBranchUnknown);
		*((uint32_t*)(MMAP_FP_BASE + FUNC_GEN_BRANCH_UNKNOWN)) = (uint32_t)segmentData.segBranchUnknown->ARMEntryPoint;

		*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uint32_t*)segmentData.segBranchUnknown->ARMcode + segmentData.segBranchUnknown->ARMcodeLen;

	#ifndef TEST
		// Compile the First contiguous block of Segments
		code_seg_t* seg = CompileCodeAt(0x88000040U);
		segmentData.dbgCurrentSegment = seg;

		*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;
	#endif

	#if 0
		printf("FUNC_GEN_START                   0x%x\n", (uint32_t)segmentData.segStart->ARMEntryPoint);
		printf("FUNC_GEN_STOP                    0x%x\n", (uint32_t)segmentData.segStop->ARMEntryPoint);
		printf("FUNC_GEN_BRANCH_UNKNOWN          0x%x\n", (uint32_t)segmentData.segBranchUnknown->ARMEntryPoint);
		printf("RECOMPILED_CODE_START            0x%x\n", (uintptr_t)seg->ARMEntryPoint);
	#endif

	}

	return &segmentData;
}

/*
 * Description:	Function to free any objects associated within the code_segment_data
 *
 *				TODO should we scan the codeSegment map to find any missed code Segments?
 */
void freeCodeSegmentData()
{
	delSegment(segmentData.segStart);
	delSegment(segmentData.segStop);
	delSegment(segmentData.segBranchUnknown);

	freeMemState();
}
