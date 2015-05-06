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

#ifndef ARMENCODE_H_
#define ARMENCODE_H_

#include "InstructionSet.h"
#include "CodeSegments.h"

#include <stdint.h>

#define ARM_BRANCH_OFFSET (2)


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
"nv",
""		// for AL_B where instruction is AL but is still conditional due to intra segment branching
};

int32_t Imm8Shift(uint32_t val);

void printf_arm(const uint32_t addr, const uint32_t word);

void emit_arm_code(code_seg_t* const codeSeg);

uint32_t arm_encode(const Instruction_t* ins, const size_t addr);

#endif /* ARMENCODE_H_ */
