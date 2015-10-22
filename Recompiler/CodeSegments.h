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

#ifndef CODESEGMENTS_H_
#define CODESEGMENTS_H_

#include <stdint.h>
#include <stddef.h>
#include "InstructionSet.h"
#include "literals.h"
#include "callers.h"

//-------------------------------------------------------------------

typedef enum
{
	SEG_ALONE,			// segment has no code before or after - literals can go either side
	SEG_START,			// segment has no code before - literals go before
	SEG_END,			// segment has no code after - literals go after
	SEG_SANDWICH,		// segment has code before and after - literals need to be global
	sizeof_SEG_TYPE_E
} seg_type_e;

typedef struct _code_seg
{
	void* ARMEntryPoint;
	struct _code_seg* next;		//next code segment in linked list
	struct _code_seg* prev;		//previous code segment in linked list
	seg_type_e Type;

	literal_t* literals;

	uint32_t* MIPScode;						// an index to mips code
	uint32_t MIPScodeLen;					// a length of mips code
	uint32_t MIPSReturnRegister;			// boolean segments returns;
	uint32_t* MIPSnextInstructionIndex;

	uint32_t MIPSRegistersUsed[3];			// The registers read/written by segment

	uint32_t MIPSRegistersUsedCount;		// Count of the registers read/written by segment

	void* ARMcode;						// a pointer to arm code
	uint32_t ARMcodeLen;					// a length to arm code

	Instruction_t* Intermcode;				// a pointer to Intermediate code
	//uint32_t IntermcodeLen;				// length to Intermediate code

	struct _code_seg* pBranchNext;		// the code segment(s) we may branch to. will need relinking after DMA moves
	struct _code_seg* pContinueNext;	// the code segment(s) we may continue to. will need relinking after DMA moves

	caller_t* callers;			// array of code segments that may call this segment

} code_seg_t;


typedef struct _code_segment_data
{
	uint32_t count;
	//code_seg_t* StaticSegments;		// code run directly in ROM
	//code_seg_t** StaticBounds;
	//uint32_t StaticLength;

	//code_seg_t* DynamicSegments;	// code running in RDRAM (copied or DMA'd from ROM)
	//code_seg_t** DynamicBounds;
	//uint32_t DynamicLength;

	literal_t* literals;

	// special stubs

	code_seg_t* segStart;
	code_seg_t* segStop;
	code_seg_t* segInterrupt;
	code_seg_t* segMem;
	code_seg_t* segBranchUnknown;
	code_seg_t* segTrap;

	volatile code_seg_t* dbgCurrentSegment;

	uint8_t bProfile;
} code_segment_data_t;

//-------------------------------------------------------------------

extern uint32_t showPrintSegmentDelete;
//TODO RWD

extern code_segment_data_t segmentData;

/*
 * Function to create a newSegment
 */
code_seg_t* newSegment();

/*
 * Function to destroy a codeSegment
 */
uint32_t delSegment(code_seg_t* codeSegment);

void freeIntermediateInstructions(code_seg_t* const codeSegment);

void invalidateBranch(const code_seg_t* codeSegment);

code_segment_data_t* GenerateCodeSegmentData(const int32_t ROMsize);

void freeCodeSegmentData();

code_seg_t* CompileCodeAt(const uint32_t const address[]);

#endif /* CODESEGMENTS_H_ */
