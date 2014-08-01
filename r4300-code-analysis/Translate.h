
#ifndef TRANSLATE_H_
#define TRANSLATE_H_

#include "InstructionSet.h"
#include "CodeSegments.h"

extern uint32_t bCountSaturates;


typedef void (pfu4v)(unsigned int, unsigned int, unsigned int, unsigned int);
typedef void (pf4v)(int, int, int, int);

typedef void (pfu1v)(unsigned int);
typedef void (pf1v)(int);

typedef unsigned int (pfu4ru1)(unsigned int, unsigned int, unsigned int, unsigned int);
typedef int (pf4r1)(int, int, int, int);

typedef unsigned int (pfu1ru1)(unsigned int);
typedef int (pf1r1)(int);

typedef unsigned int (pfuvr1)();
typedef int (pfvr1)();


void Translate_DelaySlot(code_seg_t* const codeSegment);

/*
 * MIPS4300 has a COUNT register that is decremented every instruction
 * when it underflows, an interrupt is triggered.
 */
void Translate_CountRegister(code_seg_t* const codeSegment);

/*
 * Function to turn 64bit registers into multiple 32-bit registers
 *
 */
void Translate_32BitRegisters(code_seg_t* const codeSegment);

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
void Translate_Registers(code_seg_t* const codeSegment);

void Translate_LoadStoreWriteBack(code_seg_t* const codeSegment);

void Translate_init(code_seg_t* const codeSegment);

void Translate_Memory(code_seg_t* const codeSegment);

void Translate(code_seg_t* const codeSegment);

#endif /* TRANSLATE_H_ */
