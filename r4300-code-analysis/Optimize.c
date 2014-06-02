/*
 * Optimize.c
 *
 *  Created on: 16 Apr 2014
 *      Author: rjhender
 */

#include "CodeSegments.h"
#include "InstructionSet.h"

typedef enum
{
	AVAILABLE,
	CLEAN,
	DIRTY,
	CONSTANT,
	RESERVED
} RegisterState;


static RegisterState RegisterMap[64];

static Instruction_t buffer[1000];


//=============================================================

/*
 * MIPS4300 executes instruction after a branch or jump if it is not LINK_LIKELY
 * Therefore the instruction after the branch may need to be moved into the segment.
 */
static void Optimize_DelaySlot(code_seg_t codeSegment)
{

}

/*
 * MIPS4300 has a COUNT register that is decremented every instruction
 * when it underflows, an interrupt is triggered.
 */
static void Optimize_CountRegister(code_seg_t codeSegment)
{

}

/*
 * Function to turn 64bit registers into multiple 32-bit registers
 *
 */
static void Optimize_32BitRegisters(code_seg_t codeSegment)
{

}

/*
 * Function to re-number / reduce the number of registers so that they fit the HOST
 *
 * This function will need to scan a segment and when more than the number of spare
 * HOST registers is exceded, choose to eith save register(s) into the emulated registers
 * referenced by the Frame Pointer or push them onto the stack for later use.
 *
 * Pushing onto the stack may make it easier to use LDM/SDM where 32/64 bit is not compatible
 * with the layout of the emulated register space.
 *
 */
static void Optimize_ReduceRegistersUsed(code_seg_t codeSegment)
{

}

void Optimize(code_seg_t codeSegment)
{
	Optimize_CountRegister(codeSegment);
	Optimize_DelaySlot(codeSegment);
	Optimize_32BitRegisters(codeSegment);
	Optimize_ReduceRegistersUsed(codeSegment);

}
