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

#include "CodeSegments.h"
#include "InstructionSetARM6hf.h"
#include "InstructionSet.h"
#include "InstructionSet_ascii.h"
#include "DebugDefines.h"
#include "mem_state.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "memory.h"

#define assert2(expr)							\
  if (!(expr)){									\
	  printf_Intermediate(ins, 1);				\
	  __assert_fail (__STRING(expr), __FILE__, __LINE__, __ASSERT_FUNCTION);}


static uint32_t ALU_OP2(const Instruction_t* ins)
{
	uint32_t imm = (uint32_t)ins->immediate;
	int32_t rotate = ins->rotate;

	if (ins->I)
	{
		if (imm > 255)
		{
			int x;

			for (x = 0; x < 16; x++)
			{
				if (((uint32_t)ins->immediate >> (x*2)) & (0x03))
				{
					//Found start of bits TODO what if imm is negative?
					imm = (uint32_t)ins->immediate >> (x*2);
					rotate = x;

					break;
				}
			}
		}

#if !NDEBUG
	if (rotate >=16)
	{
		abort();
	}
	if (imm >=256)
	{
		abort();
	}
#endif
		return (((16-rotate)&0xf) << 8) | (imm&0xFF);
	}
	else if (ins->R3.regID != REG_NOT_USED)
	{
		assert2(ins->shiftType < 4);
		assert2((ins->R3.regID &~REG_HOST) < 16);
		assert2((ins->R2.regID & ~REG_HOST) < 16);
		return (ins->R3.regID & ~REG_HOST) << 8 | ins->shiftType << 5 | 1 << 4 | (ins->R2.regID & ~REG_HOST);
	}
	else
	{
		assert2(ins->shift < 32);
		assert2(ins->shiftType < 4);
		assert2((ins->R2.regID & ~REG_HOST) < 16);
		return ins->shift << 7 | ins->shiftType << 5 | (ins->R2.regID & ~REG_HOST);
	}
}

int32_t Imm8Shift(uint32_t val)
{
	int i= 0;

	if (val == (val&0xFF)) return 0;

	while (!(val&1))
	{
		val >>= 1;
		i++;
	}

	if (val == (val&0xFF)) return i;

	return -1;
}

uint32_t arm_encode(const Instruction_t* ins, const size_t addr)
{
	uint8_t Rd1=0, Rd2=0, R1=0, R2=0, R3=0;

	if (ins->Rd1.regID != REG_NOT_USED) Rd1 = ins->Rd1.regID & ~REG_HOST;
	if (ins->Rd2.regID != REG_NOT_USED) Rd2 = ins->Rd2.regID & ~REG_HOST;
	if (ins->R1.regID != REG_NOT_USED)  R1  = ins->R1.regID  & ~REG_HOST;
	if (ins->R2.regID != REG_NOT_USED)  R2  = ins->R2.regID  & ~REG_HOST;
	if (ins->R3.regID != REG_NOT_USED)  R3  = ins->R3.regID  & ~REG_HOST;

	Condition_e cond = ins->cond;
	if (cond > NV) cond = AL;

	assert2(R3 < 16);
	assert2(R2 < 16);
	assert2(R1 < 16);
	assert2(Rd2 < 16);
	assert2(Rd1 < 16);

	switch (ins->instruction)
	{
	case ARM_AND:
		return cond << 28 | ins->I << 25 | 0x0 << 21 | ins->S << 20 | R1 << 16 | Rd1 << 12 | ALU_OP2(ins);
	case ARM_EOR:
		return cond << 28 | ins->I << 25 | 0x1 << 21 | ins->S << 20 | R1 << 16 | Rd1 << 12 | ALU_OP2(ins);
	case ARM_SUB:
		return cond << 28 | ins->I << 25 | 0x2 << 21 | ins->S << 20 | R1 << 16 | Rd1 << 12 | ALU_OP2(ins);
	case ARM_RSB:
		return cond << 28 | ins->I << 25 | 0x3 << 21 | ins->S << 20 | R1 << 16 | Rd1 << 12 | ALU_OP2(ins);
	case ARM_ADD:
		return cond << 28 | ins->I << 25 | 0x4 << 21 | ins->S << 20 | R1 << 16 | Rd1 << 12 | ALU_OP2(ins);
	case ARM_ADC:
		return cond << 28 | ins->I << 25 | 0x5 << 21 | ins->S << 20 | R1 << 16 | Rd1 << 12 | ALU_OP2(ins);
	case ARM_SBC:
		return cond << 28 | ins->I << 25 | 0x6 << 21 | ins->S << 20 | R1 << 16 | Rd1 << 12 | ALU_OP2(ins);
	case ARM_RSC:
		return cond << 28 | ins->I << 25 | 0x7 << 21 | ins->S << 20 | R1 << 16 | Rd1 << 12 | ALU_OP2(ins);
	case ARM_TST:
		return cond << 28 | ins->I << 25 | 0x8 << 21 | 1 << 20 | R1 << 16 | ALU_OP2(ins);
	case ARM_TEQ:
		return cond << 28 | ins->I << 25 | 0x9 << 21 | 1 << 20 | R1 << 16 | ALU_OP2(ins);
	case ARM_CMP:
		return cond << 28 | ins->I << 25 | 0xA << 21 | 1 << 20 | R1 << 16 | ALU_OP2(ins);
	case ARM_CMN:
		return cond << 28 | ins->I << 25 | 0xB << 21 | 1 << 20 | R1 << 16 | ALU_OP2(ins);
	case ARM_ORR:
		return cond << 28 | ins->I << 25 | 0xC << 21 | ins->S << 20 | R1 << 16 | Rd1 << 12 | ALU_OP2(ins);
	case ARM_MOV:
		return cond << 28 | ins->I << 25 | 0xD << 21 | ins->S << 20 | Rd1 << 12 | ALU_OP2(ins);
	case ARM_BIC:
		return cond << 28 | ins->I << 25 | 0xE << 21 | ins->S << 20 | R1 << 16 | Rd1 << 12 | ALU_OP2(ins);
	case ARM_MVN:
		return cond << 28 | ins->I << 25 | 0xF << 21 | ins->S << 20 | Rd1 << 12 | ALU_OP2(ins);
	case ARM_BFC:
		abort();
		return cond << 28 | 0x3E << 21 | 0x1F; // TODO
	case ARM_BFI:
		abort();
		return cond << 28 | 0x3E << 21 | 0x1 << 4 | R1; // TODO
	case ARM_CLZ:
		return 0x16F0F10 | ins->S << 20 | Rd1 << 12 | R1;

	case ARM_LDM:
		return cond << 28 | 1 << 27 | ins->PR << 24 | ins->U << 23 | ins->W << 21 | 1 << 20 | R1 << 16 | ins->Rmask;
	case ARM_STM:
		return cond << 28 | 1 << 27 | ins->PR << 24 | ins->U << 23 | ins->W << 21 | R1 << 16 | ins->Rmask;

	case ARM_LDR:
		if (ins->I)
			return cond << 28 | 0x1 << 26 | ins->PR << 24 | ins->U << 23 | ins->B << 22 | ins->W << 21 | 1 << 20 | R2 << 16 | Rd1 << 12 | (ins->immediate&0xFFF);
		else
			return cond << 28 | 0x3 << 25 | ins->PR << 24 | ins->U << 23 | ins->B << 22 | ins->W << 21 | 1 << 20 | R2 << 16 | Rd1 << 12 | (ins->shift&0x1f << 7) | (ins->shiftType&3 << 5) | (R3&0xf);
	case ARM_STR:
		if (ins->I)
			return cond << 28 | 0x1 << 26 | ins->PR << 24 | ins->U << 23 | ins->B << 22 | ins->W << 21 | 0 << 20 | R2 << 16 | R1 << 12 | (ins->immediate&0xFFF);
		else
			return cond << 28 | 0x3 << 25 | ins->PR << 24 | ins->U << 23 | ins->B << 22 | ins->W << 21 | 0 << 20 | R2 << 16 | R1 << 12 | (ins->shift&0x1f << 7) | (ins->shiftType&3 << 5) | (R3&0xf);
	case ARM_LDRD:
		if (ins->I)
			return cond << 28 | ins->PR << 24 | ins->U << 23 | 1 << 22 | ins->W << 21 | Rd1 << 16 | R1 << 12 | ((ins->immediate&0xF0) << 4) | 0xd0 | (ins->immediate&0xF);
		else
			return cond << 28 | ins->PR << 24 | ins->U << 23 | ins->W << 21 | Rd1 << 16 | R1 << 12 | 0xd0 | (R2&0xf);
	case ARM_STRD:
		if (ins->I)
			return cond << 28 | ins->PR << 24 | ins->U << 23 | 1 << 22 | ins->W << 21 | Rd1 << 16 | R1 << 12 | ((ins->immediate&0xF0) << 4) | 0xf0 | (ins->immediate&0xF);
		else
			return cond << 28 | ins->PR << 24 | ins->U << 23 | ins->W << 21 | Rd1 << 16 | R1 << 12 | 0xf0 | (R2&0xf);

	case ARM_LDR_LIT:
			return cond << 28 | 0x1 << 26 | ins->PR << 24 | ins->U << 23 | ins->B << 22 | ins->W << 21 | 1 << 20 | R2 << 16 | Rd1 << 12 | (ins->immediate&0xFFF);
	case ARM_STR_LIT:
			return cond << 28 | 0x1 << 26 | ins->PR << 24 | ins->U << 23 | ins->B << 22 | ins->W << 21 | 0 << 20 | R2 << 16 | R1 << 12 | (ins->immediate&0xFFF);
	case ARM_LDRD_LIT:
			return cond << 28 | ins->PR << 24 | ins->U << 23 | 1 << 22 | ins->W << 21 | Rd1 << 16 | R1 << 12 | ((ins->immediate>>4)&0xf) | 0xd0 | (ins->immediate&0xf);
	case ARM_STRD_LIT:
			return cond << 28 | ins->PR << 24 | ins->U << 23 | 1 << 22 | ins->W << 21 | Rd1 << 16 | R1 << 12 | ((ins->immediate>>4)&0xf) | 0xf0 | (ins->immediate&0xf);
	case ARM_B:
		if (ins->I)
			return cond << 28 | 0xA << 24 | ins->Ln << 24 | (((ins->offset - addr )/4 - ARM_BRANCH_OFFSET)&0xffffff);
		else
			return cond << 28 | 0xA << 24 | ins->Ln << 24 | ((ins->offset - ARM_BRANCH_OFFSET)&0xffffff);
	case ARM_BX:
		return cond << 28 | 0x12fff10 | ins->Ln << 5 | R1;

	//case JR:
		//assert(ins->I == 0);
		// we just need to move the specified register into the pc on arm
		//return ins->cond << 28 | 0xd << 21 | ins->S << 20 | (REG_HOST_PC&0xf) << 12 | 1 << 4 | R1;

		//-------------------------------------------

	case LITERAL:
		return ins->immediate;

		//------- ARM Cannot handle -----------------

	default:
		break;
	}

	printf("Could not encode '%s'\n", Instruction_ascii[STRIP(ins->instruction)]);

	printf_Intermediate(ins, 1);

#if defined (ABORT_ARM_ENCODE)
	abort();
#endif
	return 0;
}

static void arm_ldm_regs(char* str, uint32_t regMask)
{
	int i;
	int p = 0;

	for (i=0; i < 16; i++)
	{
		if (regMask & (1 << i))
		{
			p += sprintf(&str[p], "%s, ", arm_reg_a[i]);
		}
	}

	if (p) str[p-2] = '\0';

}

static void opsh(char* str, uint32_t word)
{
	uint32_t imm;

	imm = (word >> 7)&0x1f;

	if (imm)
	{
		switch (word>>5)
		{
		case 0: // Logical Left
			sprintf(str, ", lsl #%d", imm); break;
		case 1: // Logical Right
			sprintf(str, ", lsr #%d", imm); break;
		case 2: // A Right
			sprintf(str, ", asr #%d", imm); break;
		case 3: // Rotate Right
			sprintf(str, ", ror #%d", imm); break;
		}
	}
	else str[0] = 0;
}

void printf_arm(const uint32_t addr, const uint32_t word)
{
	printf("0x%08x", addr);
#if defined(SHOW_PRINT_ARM_VALUE)
	printf(" 0x%08x", word);
#endif

	// ADC - See ALU
	// ADD - See ALU
	// AND - See ALU
	// ASR - See ALU
	if((word & 0x0f000000) == 0x0a000000) // B
	{
		// TODO we could lookup the function being branched to ...
		printf("\tb%s\t0x%08x   // (%d)\n", arm_cond[word>>28], addr + 8 +((int32_t)(word&0xffffff) << 8)/(1 << 6),  ((int32_t)(word&0xffffff) << 8)/(1 << 8));
	}
	else if ((word & 0x0fe0007f) == 0x07e0001f) // BFC
	{
		printf("\tbfc%s\t%s, #%d, #%d\n", arm_cond[word>>28], arm_reg_a[(word>>12)&0xf], (word>>7)&0x1f, ((word>>16)&0x1f) - ((word>>7)&0x1f));
	}
	else if ((word & 0x0fe00070) == 0x07c00010) // BFI
	{
		printf("\tbfi%s\t%s, %s, #%d, #%d\n", arm_cond[word>>28], arm_reg_a[(word>>12)&0xf], arm_reg_a[(word)&0xf], (word>>7)&0x1f, ((word>>16)&0x1f) - ((word>>7)&0x1f));
	}
	// BIC - See ALU
	else if((word & 0x0ff000f0) == 0x01200070) //BKPT
	{
		printf("\tbkpt #%d\n", ((word>>4)&0xfff0) | (word&0xf));
	}
	else if((word & 0x0f000000) == 0x0b000000) // BL
	{
		printf("\tbl%s\t0x%08x   // (%d)\n", arm_cond[word>>28], addr + 8 +((int32_t)(word&0xffffff) << 8)/(1 << 6),  ((int32_t)(word&0xffffff) << 8)/(1 << 8));
	}
	else if ((word & 0x0fffffd0) == 0x012fff10)	// BX / BLX
	{
		if (word & 0x20)
		{
			printf("\tblx%s\t%s\n", arm_cond[word>>28], arm_reg_a[word&0xf]);
		}
		else
		{
			printf("\tbx%s\t%s\n", arm_cond[word>>28], arm_reg_a[word&0xf]);
		}
	}
	else if ((word & 0xff000010) == 0xee000000) // CDP
	{
		printf("\tcdp\t\n");
	}
	else if ((word & 0x0f000010) == 0x0e000000) // CDP2
	{
		printf("\tcdp2\t\n");
	}
	else if ((word == 0xf57ff01f)) // CLREX	ARMv6K, ARMv7
	{
		printf("\tclrex\n");
	}
	//CLZ
	//CMN See ALU
	//CMP See ALU
	//CPS
	//CPY
	//DBG
	//DMB
	//DSB
	//ENTERX
	//EOR See ALU
	//HB/HBL/HBLP/HBP (Thumb)
	//HVC
	//ISB
	//IT (Thumb)
	//LDC
	else if((word & 0x0e400000) == 0x08000000) // LDM/STM
	{
		char regs[60];
		arm_ldm_regs(regs, word & 0xFFFF);
		char pre[3];
		char wb[] = {0,0};

		if 		((word & 0x01800000) == 0x01800000) sprintf(pre, "ib");			//P & U
		else if ((word & 0x01800000) == 0x00800000) pre[0] = '\0'; //sprintf(pre, "");			//~P & U
		else if ((word & 0x01AD0000) == 0x012D0000) sprintf(pre, "fd");			//P & ~U	//fully descend
		else if ((word & 0x01800000) == 0x01000000) sprintf(pre, "db");			//P & ~U
		else if ((word & 0x01800000) == 0x00800000) sprintf(pre, "da");			//~P & ~U	//TODO check
		else sprintf(pre, "  ");

		if (word & 1<<21) wb[0] = '!';

		if (word & 0x100000)
		{
			printf("\tldm%s%s\t%s%s, {%s}\n", pre, arm_cond[word>>28], arm_reg_a[(word>>16)&0xf], wb, regs);
		}
		else
		{
			printf("\tstm%s%s\t%s%s, {%s}\n", pre, arm_cond[word>>28], arm_reg_a[(word>>16)&0xf], wb, regs);
		}
	}
	else if((word & 0x0c000000) == 0x04000000) // LDR / STR Immediate
	{
		char ins[4];
		char wb[] = {0,0};
		char byt[] = {0,0};
		char minus[] = {0,0};
		char imm[12];

		if (word & 0x100000)
			sprintf(ins, "ldr");
		else
			sprintf(ins, "str");

		if (word & (1 << 21)) wb[0] = '!';
		if (word & (1 << 22)) byt[0] = 'b';
		if (!(word & (1 << 23))) minus[0] = '-';

		sprintf(imm, "#0x%x", word & 0xfff);

		if (0 == (word & 0xfff))
		{
			printf("\t%s%s%s\t%s, [%s]      \t\n", ins, byt, arm_cond[word>>28], arm_reg_a[(word>>12)&0xf], arm_reg_a[(word>>16)&0xf]);
		}
		else if (word & (1 << 24)) // Pre/post
			printf("\t%s%s%s\t%s, [%s, %s%s]%s \t// %s%d\n", ins, byt, arm_cond[word>>28], arm_reg_a[(word>>12)&0xf], arm_reg_a[(word>>16)&0xf], minus, imm, wb, minus, (word & 0xfff)/4);
		else
			printf("\t%s%s%s\t%s, [%s], %s%s \t// %s%d\n", ins, byt, arm_cond[word>>28], arm_reg_a[(word>>12)&0xf], arm_reg_a[(word>>16)&0xf], minus, imm, minus, (word & 0xfff)/4);
	}
	else if((word & 0x0c000000) == 0x06000000) // LDR Register
	{
		char ins[4];
		char wb[] = {0,0};
		char byt[] = {0,0};
		char minus[] = {0,0};
		char str_opsh[10];

		if (word & 0x100000)
			sprintf(ins, "ldr");
		else
			sprintf(ins, "str");

		if (word & 1<<21) wb[0] = '!';
		if (word & 1<<22) byt[0] = 'b';
		if (!(word & 1<<23)) minus[0] = '-';

		opsh(str_opsh, word);

		if (word & 1 << 24) // Pre/post
			printf("\t%s%s%s\t%s, [%s, %s%s%s]%s\n", ins, byt, arm_cond[word>>28], arm_reg_a[(word>>12)&0xf], arm_reg_a[(word>>16)&0xf], minus, arm_reg_a[(word)&0xf], str_opsh, wb);
		else
			printf("\t%s%s%s\t%s, [%s], %s%s%s\n", ins, byt, arm_cond[word>>28], arm_reg_a[(word>>12)&0xf], arm_reg_a[(word>>16)&0xf], minus, arm_reg_a[(word)&0xf], str_opsh);

	}
	//LDRB
	//LDRBT
	//LDRD
	//LDREX
	//LDREXB
	//LDREXD
	//LDREXH
	//LDRH
	//LDRHT
	//LDRSB
	//LDRSBT
	//LDRSH
	//LDRSHT
	//LDRT
	//LEAVEX
	//LSL See ALU
	//LSR See ALU
	//MCR
	//MCRR
	//MLA
	//MLS
	//MOV See ALU
	//MOVT
	//MRC
	//MRRC
	else if((word & 0x0fbf0fff) == 0x010f0000) // MRS
	{

	}
	else if((word & 0x0fbffff0) == 0x0129f000) // MSR (all)
	{

	}
	else if((word & 0x0fbff000) == 0x0329f000) // MSR (flag)
	{

	}
	else if((word & 0x0fc00090) == 0x00000090) // MUL
	{
		printf("\tmul%s\n", arm_cond[word>>28]);
	}
	//MVN See ALU
	//NEG
	//ORN
	//ORR See ALU
	//PKH
	//PLD
	//PLI
	//POP
	//PUSH
	//QADD
	//QADD16
	//QADD8
	//QASX
	//QDADD
	//QDSUB
	//QSAX
	//QSUB
	//QSUB16
	//QSUB8
	//RBIT
	//REV
	//REV16
	//REVSH
	//RFE
	//ROR See ALU
	//RRX
	//RSB See ALU
	//RSC See ALU
	//SADD16
	//SADD8
	//SASX
	//SBC See ALU
	//SBFX
	//SDIV
	//SEL
	//SETEND
	//SEV
	//SVC
	else if ((word & 0x0fb00f90) == 0x01000090) // SWP
	{
		printf("\tswp\n");
	}
	//SXTAB
	//SXTAB16
	//SXTAH
	//SXTB
	//SXTB16
	//SXTH
	//TBB, TBH
	//TEQ See ALU
	//TST See ALU
	//UADD16
	//UADD8
	//UASX
	//UDF
	//UDIV
	//UHADD16
	//UHADD8
	//UHASX
	//UHSAX
	//UHSUB16
	//UHSUB8
	//UMAAL
	//UMLAL
	//UMULL
	//UQADD16
	//UQADD8
	//UQASX
	//UQSAX
	//UQSUB16
	//UQSUB8
	//USAD8
	//USADA8
	//USAT
	//USAT16
	//USAX
	//USUB16
	//USUB8
	//UXTAB
	//UXTAB16
	//UXTB
	//UXTB16
	//UXTH
	else if((word & 0x0c000000) == 0x00000000) // ALU
	{
		char Op2[30];

		if (word & 0x02000000) // Immediate
		{
			if (word & 0x00000f00)	// any rotate
			{
				uint8_t ror = ((word>>8)&0xf)*2;
				sprintf(Op2,", #%-11d (0x%08x)", ((word&0xff) << (32 - ror) | (word&0xff) >> (ror)), ((word&0xff)<< (32 - ror)) | (word&0xff) >> (ror));
			}else
			{
				sprintf(Op2,", #%-11d (0x%08x)", word&0xff, word&0xff);
			}
		}
		else
		{
			if (word & 0x00000010)	// Shift field type 1
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
			else // Shift field type 0
			{
				if ((word>>7)&0x1f)	//is Shift amount > 0?
				{
					switch ((word>>5)&3)	//type
					{
					case 0:
						sprintf(Op2,", %s, lsl #%d (0x%x)", arm_reg_a[word&0xf], (word>>7)&0x1f, (word>>7)&0x1f); break;
					case 1:
						sprintf(Op2,", %s, lsr #%d (0x%x)", arm_reg_a[word&0xf], (word>>7)&0x1f, (word>>7)&0x1f); break;
					case 2:
						sprintf(Op2,", %s, asr #%d (0x%x)", arm_reg_a[word&0xf], (word>>7)&0x1f, (word>>7)&0x1f); break;
					case 3:
						sprintf(Op2,", %s, ror #%d (0x%x)", arm_reg_a[word&0xf], (word>>7)&0x1f, (word>>7)&0x1f); break;
					}
				}
				else
				{
					sprintf(Op2,", %s", arm_reg_a[word&0xf]);
				}
			}
		}

		switch ((word >> 21)&0xf)
		{
		case 0:	// AND
			printf("\tand%s\t%s, %s%s\n", 	arm_cond[word>>28], arm_reg_a[(word>>12)&0xf], arm_reg_a[(word>>16)&0xf], Op2); break;
		case 1: // EOR
			printf("\teor%s\t%s, %s%s\n", 	arm_cond[word>>28], arm_reg_a[(word>>12)&0xf], arm_reg_a[(word>>16)&0xf], Op2); break;
		case 2: // SUB
			printf("\tsub%s\t%s, %s%s\n", 	arm_cond[word>>28], arm_reg_a[(word>>12)&0xf], arm_reg_a[(word>>16)&0xf], Op2); break;
		case 3:	// RSB
			printf("\trsb%s\t%s, %s%s\n", 	arm_cond[word>>28], arm_reg_a[(word>>12)&0xf], arm_reg_a[(word>>16)&0xf], Op2); break;
		case 4:	// ADD
			printf("\tadd%s\t%s, %s%s\n", 	arm_cond[word>>28], arm_reg_a[(word>>12)&0xf], arm_reg_a[(word>>16)&0xf], Op2); break;
		case 5: // ADC
			printf("\tadc%s\t%s, %s%s\n", 	arm_cond[word>>28], arm_reg_a[(word>>12)&0xf], arm_reg_a[(word>>16)&0xf], Op2); break;
		case 6: // SBC
			printf("\tsbc%s\t%s, %s%s\n", 	arm_cond[word>>28], arm_reg_a[(word>>12)&0xf], arm_reg_a[(word>>16)&0xf], Op2); break;
		case 7: // RSC
			printf("\trsc%s\t%s, %s%s\n", 	arm_cond[word>>28], arm_reg_a[(word>>12)&0xf], arm_reg_a[(word>>16)&0xf], Op2); break;
		case 8: // TST
			printf("\ttst%s\t%s%s\n", 		arm_cond[word>>28], arm_reg_a[(word>>16)&0xf], Op2); 							break;
		case 9: // TEQ
			printf("\tteq%s\t%s%s\n", 		arm_cond[word>>28], arm_reg_a[(word>>16)&0xf], Op2); 							break;
		case 10: // CMP
			printf("\tcmp%s\t%s%s\n", 		arm_cond[word>>28], arm_reg_a[(word>>16)&0xf], Op2); 							break;
		case 11: // CMN
			printf("\tcmn%s\t%s%s\n", 		arm_cond[word>>28], arm_reg_a[(word>>16)&0xf], Op2); 							break;
		case 12: // ORR
			printf("\torr%s\t%s, %s%s\n", 	arm_cond[word>>28], arm_reg_a[(word>>12)&0xf], arm_reg_a[(word>>16)&0xf], Op2); break;
		case 13: // MOV
			printf("\tmov%s\t%s%s\n", 		arm_cond[word>>28], arm_reg_a[(word>>12)&0xf], Op2); 							break;
		case 14: // BIC
			printf("\tbic%s\t%s, %s%s\n", 	arm_cond[word>>28], arm_reg_a[(word>>12)&0xf], arm_reg_a[(word>>16)&0xf], Op2); break;
		case 15: // MVN
			printf("\tmvn%s\t%s, %s%s\n", 	arm_cond[word>>28], arm_reg_a[(word>>12)&0xf], arm_reg_a[(word>>16)&0xf], Op2); break;
			break;
		}
	}
	//VABA
	//VABD
	//VABS
	//VACGE/VACGT/VACLE/VACLT
	//VADD
	//VADDHN
	//VADDL/VADDW
	//VAND
	//VBIC
	//VBIF
	//VCEQ
	//VCGE
	//VCGT
	//VCLE
	//VCLS
	//VCLT
	//VCLZ
	//VCMP/VCMPE
	//VCNT
	//VCVT
	//VCVTB/VCVTT
	//VDIV
	//VDUP
	//VEOR
	//VEXT
	//VFMA/VFMS
	//VFNMA/VFNMS
	//VHADD
	//VLD1
	//VLD2
	//VLD3
	//VLD4
	//VLDM
	//VLDR
	//VMAX/VMIN
	//VMLA/VMLAL
	//VMOV
	//VMOVL
	//VMOVN
	//VMRS
	//VMSR
	//VMUL
	//VMVN
	//VNEG
	//VNMLA
	//VORN
	//VORR
	//VPADAL
	//VPADD
	//VPADDL
	//VPMAX/VPMIN
	//VPOP
	//VPUSH
	//VQABS
	//VQADD
	//VQDMLAL
	//VQDMULH
	//VQDMULL
	//VQMOVN
	//VQNEG
	//VQRDMULH
	//VQRSHL
	//VQRSHRN
	//VQSHL
	//VQSHRN
	//VQSUB
	//VRADDHN
	//VRECPE
	//VRECPS
	//VREV16/VREV32
	//VRHADD
	//VRSHL
	//VRSHR
	//VRSHRN
	//VRQSRTS
	//VRSRA
	//VRSUBHN
	//VSHL
	//VSHLL
	//VSHR
	//VSHRN
	//VSLI
	//VSQRT
	//VSRA
	//VSRI
	//VST1
	//VST2
	//VST3
	//VST4
	//VSTM
	//VSTR
	//VSUB
	//VSUBHN
	//VSUBL
	//VSWP
	//VTBL
	//VTRN
	//VTST
	//VUZP
	//VZIP
	//WFE
	else if ((word&0x0fffffff) == 0x0320f002)
	{
		printf("\twfe\n");
	}
	//WFI
	else if ((word&0x0fffffff) == 0x0320f003)
	{
		printf("\twfi\n");
	}
	//YIELD
	else
	{
		printf ("\tUnknown Command - value =  %010d (%0x08x)\n", word, word);
#if defined(ABORT_ARM_DECODE)
		abort();
#endif
	}
	return;
}

void emit_arm_code(code_seg_t* const codeSeg)
{
	static uint32_t* out = NULL;

	/* TODO check for (out > MMAP_DR_BASE + 0x02000000)
	 * eventually we will need to wrap back to MMAP_DR_BASE and
	 * invalidate code Segments that are overwritten.
	 */

	if (!out) out = (uint32_t*)MMAP_DR_BASE;

	Instruction_t *ins = codeSeg->Intermcode;
	literal_t* lits = codeSeg->literals;

#if defined(ASSERT_ARM_NOT_COMPILED)
	assert (!codeSeg->ARMcode);
#endif

	if (!ins)
	{
		printf("cannot emit arm code as not compiled yet\n");
		return;
	}

	codeSeg->ARMcode = out;
	codeSeg->ARMcodeLen = 0;

	//write out start literals
	if (lits)
	{
		if (codeSeg->Type == SEG_START
				|| codeSeg->Type == SEG_ALONE)
		{
			while (lits)
			{
				*out = lits->value;
				codeSeg->ARMcodeLen++;
				out++;
				lits = lits->next;
			}
		}
	}

	codeSeg->ARMEntryPoint = out;

	printf("emit arm code. segment 0x%08x output to 0x%08x\n", (uint32_t)codeSeg, (uint32_t)codeSeg->ARMcode);

	//write out code instructions
	while (ins)
	{
		*out = arm_encode(ins, (size_t)out);
		ins->outputAddress = (size_t)out;
		codeSeg->ARMcodeLen++;
		out++;
		ins = ins->nextInstruction;
	}

	// write out end literals
	if (lits)
	{
		if (codeSeg->Type == SEG_END)
		{
			while (lits)
			{
				*out = lits->value;
				codeSeg->ARMcodeLen++;
				out++;
				lits = lits->next;
			}
		}
	}
}
