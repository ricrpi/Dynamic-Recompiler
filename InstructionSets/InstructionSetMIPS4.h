/*
 * mips_h
 *
 *  Created on: 24 Mar 2014
 *      Author: ric_rpr
 */

#ifndef MIPS_H_
#define MIPS_H_

#include <stdint.h>
#include <stdio.h>
#include "InstructionSet.h"

#define MIPS_REG_0     0x00000001
#define MIPS_REG_1     0x00000002
#define MIPS_REG_2     0x00000004
#define MIPS_REG_3     0x00000008
#define MIPS_REG_4     0x00000010
#define MIPS_REG_5     0x00000020
#define MIPS_REG_6     0x00000040
#define MIPS_REG_7     0x00000080
#define MIPS_REG_8     0x00000100
#define MIPS_REG_9     0x00000200
#define MIPS_REG_10    0x00000400
#define MIPS_REG_11    0x00000800
#define MIPS_REG_12    0x00001000
#define MIPS_REG_13    0x00002000
#define MIPS_REG_14    0x00004000
#define MIPS_REG_15    0x00008000
#define MIPS_REG_16    0x00010000
#define MIPS_REG_17    0x00020000
#define MIPS_REG_18    0x00040000
#define MIPS_REG_19    0x00080000
#define MIPS_REG_20    0x00100000
#define MIPS_REG_21    0x00200000
#define MIPS_REG_22    0x00400000
#define MIPS_REG_23    0x00800000
#define MIPS_REG_24    0x01000000
#define MIPS_REG_25    0x02000000
#define MIPS_REG_26    0x04000000
#define MIPS_REG_27    0x08000000
#define MIPS_REG_28    0x10000000
#define MIPS_REG_29    0x20000000
#define MIPS_REG_30    0x40000000
#define MIPS_REG_31    0x80000000


#define MIPS_REG_LO    0x00000001
#define MIPS_REG_HI    0x00000002

#define MIPS_REG_FCR0  0x00000004
#define MIPS_REG_FCR31 0x00000008
#define MIPS_REG_LL	   0x00000010
#define MIPS_REG_PC	   0x00000020
#define MIPS_REG_CC    0x00000040	// Coprocessor Condition code
#define MIPS_TRAP      0x00000100

#define MIPS_REG_ALL   0xffffffff

#define MIPS_REG(x)		((uint32_t)(1<<(x)))

/*
 * Provides a bit mask for the registers used in an instruction:
 */
uint32_t ops_regs_input(const uint32_t uiMIPSword, uint32_t * const uiCPUregs, uint32_t * const uiVFPregs, uint32_t * const uiSpecialRegs);

uint32_t ops_regs_output(const uint32_t uiMIPSword, uint32_t * const uiCPUregs, uint32_t * const uiVFPregs, uint32_t * const uiSpecialRegs);
/*
 * Converts a raw word into an enumeration type
 */
Instruction_e ops_type(const uint32_t uiMIPSword);

/*
 * Return the Branch instruction offset for the raw word provided
 *
 * Return:
 * 		offset or 0x7FFFFFF if the instruction is not a Jump nor Branch
 * */
int32_t ops_BranchOffset(const uint32_t* const uiMIPSword);

uint32_t ops_JumpAddress(const uint32_t* const uiMIPSword);

/*
 * Provides printf() output for the raw word
 */
void fprintf_mips(FILE* stream, const uint32_t x, const uint32_t uiMIPSword);
void sprintf_mips(char* stream, const uint32_t x, const uint32_t uiMIPSword);

uint32_t mips_decode(const uint32_t uiMIPSword, Instruction_t* const ins);

#endif /* MIPS_H_ */
