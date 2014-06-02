/*
 * ARMencode.c
 *
 *  Created on: 16 Apr 2014
 *      Author: rjhender
 */

#include "CodeSegments.h"
#include "InstructionSetARM6hf.h"
#include "InstructionSet.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>


unsigned char arm_regs[] = {0,1,2,3,4,5,6,7,8,9,10,12,14,11};

#define ARM_REG_COUNT (sizeof(arm_regs)/ sizeof(arm_regs[0]))


static uint32_t ALU_OP2(const Instruction_t ins)
{
	if (ins.I)
		return (ins.rotate&0xf) << 8 | (ins.immediate&0xff);
	else
		return (ins.shift&0xff) << 4 | (ins.Rm&0xf);
}

/*
static uint32_t rd_rn_rm(uint32_t rd, uint32_t rn, uint32_t rm)
{
  assert(rd<16);
  assert(rn<16);
  assert(rm<16);
  return((rn<<16)|(rd<<12)|rm);
}
static uint32_t rd_rn_imm_shift(uint32_t rd, uint32_t rn, uint32_t imm, uint32_t shift)
{
  assert(rd<16);
  assert(rn<16);
  assert(imm<256);
  assert((shift&1)==0);
  return((rn<<16)|(rd<<12)|(((32-shift)&30)<<7)|imm);
}
*/

uint32_t arm_encode(const Instruction_t ins)
{
	switch (ins.instruction)
	{
		case SLL:
			return ins.cond<<28 | 1 <<25 | 0xd<<21 | ins.S << 20 | ins.Rn << 16 | ins.Rd << 12 | ALU_OP2(ins);
		case SRL:
			return 0;
		case SLLV:
			return ins.cond<<28 | 0xd<<21 | ins.S << 20 | ins.Rn << 16 | ins.Rd << 12 | ALU_OP2(ins);
		default:
			break;
	}

	printf("Can not encode instruction\n");
	return 0;
}
