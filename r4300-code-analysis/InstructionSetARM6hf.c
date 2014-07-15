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
#include <string.h>

//static unsigned char arm_regs[] = {0,1,2,3,4,5,6,7,8,9,10,12,14,11};

#define ARM_REG_COUNT (sizeof(arm_regs)/ sizeof(arm_regs[0]))

static uint32_t ALU_OP2(const Instruction_t ins)
{

	if (ins.I)
	{
		assert(ins.rotate < 16);
		assert(ins.immediate < 256);
		return ins.rotate << 8 | ins.immediate;
	}
	else if (ins.R3 != REG_NOT_USED)
	{
		assert(ins.R3 < 16);
		assert(ins.shiftType < 4);
		assert(ins.R2 < 16);
		return ins.R3 << 8 | ins.shiftType << 5 | ins.R2;
	}
	else
	{
		assert(ins.shift < 32);
		assert(ins.shiftType < 4);
		assert(ins.R3 < 16);
		return ins.shift << 7 | ins.shiftType << 5 | 1 << 4 | ins.R2;
	}
}

static uint32_t LDR_OP2(const Instruction_t ins)
{
	if (ins.I)
	{
		assert(ins.shift < 256);
		assert(ins.R2 < 16);
		return ins.shift << 8 | ins.R2;
	}else{
		assert(ins.immediate < 256*16);
		return (ins.immediate&0xfff);
	}
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
	case SRL:
	case SRA:
	case SLLV:
	case SRAV:
	case SRLV:
	case ARM_MOV:
		return ins.cond << 28 | ins.I << 25 | 0xd << 21 | ins.S << 20 | ins.R1 << 16 | ins.Rd1 << 12 | ALU_OP2(ins);
	case JR:
		assert(ins.I == 0);
		// we just need to move the specified register into the pc on arm
		return ins.cond << 28 | 0xd << 21 | ins.S << 20 | (REG_HOST_PC&0xf) << 12 | 1 << 4 | ins.R1;
	case MTC0:

	case LUI:

	case ADDIU:

	case LW:

	case BNE:


		//-------------------------------------------

	case LITERAL:
		return ins.immediate;
	case SYSCALL:
	case BREAK:
	case SYNC:
		return 0;

		//------- ARM Cannot handle -----------------

	case JALR:
		break;

	default:
		break;
	}

	printf("Can not encode instruction %u\n", ins.instruction);
	return 0;
}

void arm_decode(uint32_t word)
{
	if ((word & 0x0fb00f90) == 0x01000090) // swap
	{
		printf("\tswap\n");
	}
	else if((word & 0x0fc00090) == 0x00000090) // Multiply
	{
		printf("\tmul%s\n", arm_cond[word>>28]);
	}
	else if((word & 0x0e000010)== 0x06000010) // UNDEFINED
	{
		printf ("Undefined Command\n");
	}
	else if((word & 0x0f000000) == 0x0a000000) // Branch
	{
		printf("\tb%s\t0x%x\n",arm_cond[word>>28], word&0xffffff);
	}
	else if((word & 0x0f000000) == 0x0b000000) // Branch and Link
	{
		printf("\tbl%s\t0x%x\n",arm_cond[word>>28], word&0xffffff);
	}
	else if((word & 0x0e000000) == 0x08000000) // LDM/STM
	{
		if (word & 0x100000)
		{
			printf("\tldm%s\t%s, {}\n",arm_cond[word>>28], arm_reg_a[(word>>16)&0xf]);	//TODO fill in registers
		}
		else
		{
			printf("\tstm%s\t%s, {}\n",arm_cond[word>>28], arm_reg_a[(word>>16)&0xf]);
		}
	}
	else if((word & 0x0fbf0fff) == 0x010f0000) // MRS
	{

	}
	else if((word & 0x0fbffff0) == 0x0129f000) // MSR (all)
	{

	}
	else if((word & 0x0fbff000) == 0x0329f000) // MSR (flag)
	{

	}
	else if((word & 0x0c000000) == 0x04000000) // LDR
	{
		if (word & 0x100000)
		{
			printf("\tldr%s\t%s, {}\n",arm_cond[word>>28], arm_reg_a[(word>>16)&0xf]);	//TODO fill in registers
		}
		else
		{
			printf("\tstr%s\t%s, {}\n",arm_cond[word>>28], arm_reg_a[(word>>16)&0xf]);
		}
	}
	else if((word & 0x0c000000) == 0x00000000) // ALU
	{
		char Op2[30];

		if (word & 0x02000000) // Immediate
		{
			if (word & 0x00000f00)	// rotate
			{
				uint8_t ror = ((word>>8)&0xf)*2;
				sprintf(Op2,", #%d", ((word&0xff)>> ror) || (word&0xff) << (32 - ror));
			}else
			{
				sprintf(Op2,", #%d", word&0xff);
			}
		}
		else
		{
			if (word & 0x00000010)	// Shift field type 1
			{
				if ((word>>7)&0x1f)	//is Shift amount > 0?
				{
					switch ((word>>5)&3)	//type
					{
					case 0:
						sprintf(Op2,", %s, lsl #%d", arm_reg_a[word&0xf], (word>>7)&0x1f); break;
					case 1:
						sprintf(Op2,", %s, lsr #%d", arm_reg_a[word&0xf], (word>>7)&0x1f); break;
					case 2:
						sprintf(Op2,", %s, asr #%d", arm_reg_a[word&0xf], (word>>7)&0x1f); break;
					case 3:
						sprintf(Op2,", %s, ror #%d", arm_reg_a[word&0xf], (word>>7)&0x1f); break;
					}
				}
				else
				{
					sprintf(Op2,", %s", arm_reg_a[word&0xf]);
				}
			}
			else // Shift field type 0
			{
				switch ((word>>5)&3)	//type
				{
				case 0:
					sprintf(Op2,", %s, lsl %s", arm_reg_a[word&0xf], arm_reg_a[(word>>8)&0xf]); break;
				case 1:
					sprintf(Op2,", %s, lsr %s", arm_reg_a[word&0xf], arm_reg_a[(word>>8)&0xf]); break;
				case 2:
					sprintf(Op2,", %s, asr %s", arm_reg_a[word&0xf], arm_reg_a[(word>>8)&0xf]); break;
				case 3:
					sprintf(Op2,", %s, ror %s", arm_reg_a[word&0xf], arm_reg_a[(word>>8)&0xf]); break;
				}
			}
		}

		switch (word >> 21)
		{
		case 0:	// AND
			printf("\tand%s\t%s, %s, %s\n",arm_cond[word>>28], arm_reg_a[(word>>12)&0xf], arm_reg_a[(word>>16)&0xf], Op2); break;
		case 1: // EOR
			printf("\teor%s\t%s, %s, %s\n",arm_cond[word>>28], arm_reg_a[(word>>12)&0xf], arm_reg_a[(word>>16)&0xf], Op2); break;
		case 2: // SUB
			printf("\tsub%s\t%s, %s, %s\n",arm_cond[word>>28], arm_reg_a[(word>>12)&0xf], arm_reg_a[(word>>16)&0xf], Op2); break;
		case 3:
			printf("\trsb%s\t%s, %s, %s\n",arm_cond[word>>28], arm_reg_a[(word>>12)&0xf], arm_reg_a[(word>>16)&0xf], Op2); break;
		case 4:
			printf("\tadd%s\t%s, %s, %s\n",arm_cond[word>>28], arm_reg_a[(word>>12)&0xf], arm_reg_a[(word>>16)&0xf], Op2); break;
		case 5:
			printf("\tadc%s\t%s, %s, %s\n",arm_cond[word>>28], arm_reg_a[(word>>12)&0xf], arm_reg_a[(word>>16)&0xf], Op2); break;
		case 6:
			printf("\tsbc%s\t%s, %s, %s\n",arm_cond[word>>28], arm_reg_a[(word>>12)&0xf], arm_reg_a[(word>>16)&0xf], Op2); break;
		case 7:
			printf("\trsc%s\t%s, %s, %s\n",arm_cond[word>>28], arm_reg_a[(word>>12)&0xf], arm_reg_a[(word>>16)&0xf], Op2); break;
		case 8:
			printf("\ttst%s\t%s, %s\n",arm_cond[word>>28], arm_reg_a[(word>>16)&0xf], Op2); break;
		case 9:
			printf("\tteq%s\t%s, %s\n",arm_cond[word>>28], arm_reg_a[(word>>16)&0xf], Op2); break;
		case 10:
			printf("\tcmp%s\t%s, %s\n",arm_cond[word>>28], arm_reg_a[(word>>16)&0xf], Op2); break;
		case 11:
			printf("\tcmn%s\t%s, %s\n",arm_cond[word>>28], arm_reg_a[(word>>16)&0xf], Op2); break;
		case 12:
			printf("\torr%s\t%s, %s, %s\n",arm_cond[word>>28], arm_reg_a[(word>>12)&0xf], arm_reg_a[(word>>16)&0xf], Op2); break;
		case 13:
			printf("\tmov%s\t%s, %s, %s\n",arm_cond[word>>28], arm_reg_a[(word>>12)&0xf], arm_reg_a[(word>>16)&0xf], Op2); break;
		case 14:
			printf("\tbic%s\t%s, %s, %s\n",arm_cond[word>>28], arm_reg_a[(word>>12)&0xf], arm_reg_a[(word>>16)&0xf], Op2); break;
		case 15:
			printf("\tmvn%s\t%s, %s, %s\n",arm_cond[word>>28], arm_reg_a[(word>>12)&0xf], arm_reg_a[(word>>16)&0xf], Op2); break;
			break;
		}

	}
	else if ((word & 0x0c000000) == 0x0c000000)
	{
		printf("\tTODO co processor\n"); // TODO arm co-processor instructions
	}
	else
	{
		printf ("Unknown Command 0x%08x\n", word);
	}
	return;
}

