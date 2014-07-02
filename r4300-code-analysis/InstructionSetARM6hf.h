/*
 * ARMencode.h
 *
 *  Created on: 16 Apr 2014
 *      Author: rjhender
 */

#ifndef ARMENCODE_H_
#define ARMENCODE_H_

#include "InstructionSet.h"
#include <stdint.h>

const static char* arm_reg_a[] = {"r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7","r8","r9","r10","fp","r12","sp","lr","pc"};
const static char* arm_cond[] = {
"eq",
"ne",
"cs",
"cc",
"mi",
"pl",
"vs",
"vc",
"hi",
"ls",
"ge",
"lt",
"gt",
"le",
"",
"nv"
};

uint32_t arm_encode(const Instruction_t ins);

#endif /* ARMENCODE_H_ */
