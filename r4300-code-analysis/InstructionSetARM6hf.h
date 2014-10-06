/*
 * ARMencode.h
 *
 *  Created on: 16 Apr 2014
 *      Author: rjhender
 */

#ifndef ARMENCODE_H_
#define ARMENCODE_H_

#include "InstructionSet.h"
#include "CodeSegments.h"

#include <stdint.h>

const static char* const arm_reg_a[] = {"r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7","r8","r9","r10","fp","r12","sp","lr","pc"};
const static char* const arm_cond[] = {
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

void arm_print(const uint32_t addr, const uint32_t word);

void emit_arm_code(code_seg_t* const codeSeg);

#endif /* ARMENCODE_H_ */
