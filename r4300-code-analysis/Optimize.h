
#ifndef OPTIMIZE_H_
#define OPTIMIZE_H_

#include "InstructionSet.h"
#include "CodeSegments.h"

extern uint32_t bCountSaturates;

void Optimize_DelaySlot(code_seg_t* codeSegment);

/*
 * MIPS4300 has a COUNT register that is decremented every instruction
 * when it underflows, an interrupt is triggered.
 */
void Optimize_CountRegister(code_seg_t* codeSegment);

/*
 * Function to turn 64bit registers into multiple 32-bit registers
 *
 */
void Optimize_32BitRegisters(code_seg_t* codeSegment);

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
void Optimize_ReduceRegistersUsed(code_seg_t* codeSegment);

void Optimize_LoadStoreWriteBack(code_seg_t* codeSegment);

void Optimize_init(code_seg_t* codeSegment);

void Optimize(code_seg_t* codeSegment);

void Optimize_printBuffer(code_seg_t* codeSegment);

#endif /* OPTIMIZE_MIPS4_H_ */
