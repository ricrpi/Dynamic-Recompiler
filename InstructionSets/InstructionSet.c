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

#include <stdio.h>
#include <stdlib.h>
#include "InstructionSet.h"
#include "InstructionSet_ascii.h"
#include "InstructionSetARM6hf.h"
#include "Translate.h"
#include "CodeSegments.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "Debugger.h"

static void sprintf_reg_t(char* str, const reg_t r)
{

#if SHOW_PRINT_INT_CONST
	char cnst[20];
	switch (r.state)
	{
		case RS_CONSTANT_I1:
		case RS_CONSTANT_I2:
		case RS_CONSTANT_I4:
			sprintf(cnst, "0x%08x         ", (uint32_t)r.i4); break;
		case RS_CONSTANT_I8:
			sprintf(cnst, "0x%016llx ", (uint64_t)r.i8); break;
		case RS_CONSTANT_U1:
		case RS_CONSTANT_U2:
		case RS_CONSTANT_U4:
			sprintf(cnst, "0x%08x         ", (uint32_t)r.u4); break;
		case RS_CONSTANT_U8:
			sprintf(cnst, "0x%016llx ", (uint64_t)r.u8); break;
		default:
			cnst[0] = '\0'; break;
	}

	if (r.regID == REG_NOT_USED)       sprintf(str, "                        ");
	else if (r.regID == REG_EMU_FP)   sprintf(str, "fp                      ");
	else if (r.regID == REG_HOST_SP)   sprintf(str, "sp                      ");
	else if (r.regID == REG_HOST_LR)   sprintf(str, "lr                      ");
	else if (r.regID == REG_HOST_PC)   sprintf(str, "pc                      ");
	else if (r.regID == REG_COUNT)
		sprintf(str, "COUNT                   ");
	else if (r.regID == REG_CAUSE)
		sprintf(str, "CAUSE                   ");
	else if (r.regID == REG_CONTEXT)
		sprintf(str, "CONTEXT                 ");
	else if (r.regID == REG_COMPARE)
		sprintf(str, "COMPARE                 ");
	else if (r.regID == REG_STATUS)
		sprintf(str, "STATUS                  ");

	else if (r.regID == REG_PC)        sprintf(str, "MIPS_PC                 ");
	else if (r.regID == REG_FCR0)      sprintf(str, "FCR0                    ");
	else if (r.regID == REG_FCR31)     sprintf(str, "FCR31                   ");
	else if (r.regID == REG_MULTHI)    sprintf(str, "MULT_HI                 ");
	else if (r.regID == REG_MULTLO)    sprintf(str, "MULT_LO                 ");
	else if (r.regID == REG_LLBIT)     sprintf(str, "LLBIT                   ");

	else if (r.regID >= REG_HOST)		 sprintf(str, "h%-3d %-18s", r.regID - REG_HOST, 			cnst);
	else if (r.regID >= REG_TEMP)		 sprintf(str, "t%-3d %-18s", r.regID - REG_TEMP, 			cnst);
	else if (r.regID >= REG_CO)          sprintf(str, "c%-3d %-18s", r.regID - REG_CO, 			cnst);
	else if (r.regID >= (REG_WIDE|REG_FP))sprintf(str,"f%-3dw%-18s", r.regID - (REG_WIDE|REG_FP), 	cnst);
	else if (r.regID >= REG_WIDE)        sprintf(str, "r%-3dw%-18s", r.regID - REG_WIDE, 			cnst);
	else if (r.regID >= REG_FP)          sprintf(str, "f%-3d %-18s", r.regID - REG_FP, 			cnst);
	else if (r.regID >= 0)
	{
		sprintf(str, "r%-3d %-18s", r.regID, 						cnst);
	}
	else                                 sprintf(str, "                      ");
#else
	if (r.regID == REG_NOT_USED)       	sprintf(str, "      ");
	else if (r.regID == REG_EMU_FP)    	sprintf(str, "emufp ");
	else if (r.regID == REG_HOST_SP)	sprintf(str, "sp    ");
	else if (r.regID == REG_HOST_LR)	sprintf(str, "lr    ");
	else if (r.regID == REG_HOST_PC)	sprintf(str, "pc    ");
	else if (r.regID == REG_COUNT)		sprintf(str, "COUNT ");
	else if (r.regID == REG_CAUSE)		sprintf(str, "CAUSE ");
	else if (r.regID == REG_CONTEXT)	sprintf(str, "CNTXT ");
	else if (r.regID == REG_COMPARE)	sprintf(str, "CMPRE ");
	else if (r.regID == REG_STATUS)		sprintf(str, "STATS ");

	else if (r.regID == REG_PC)        sprintf(str, "M_PC  ");
	else if (r.regID == REG_FCR0)      sprintf(str, "FCR0  ");
	else if (r.regID == REG_FCR31)     sprintf(str, "FCR31 ");
	else if (r.regID == REG_MULTHI)    sprintf(str, "M_HI  ");
	else if (r.regID == REG_MULTLO)    sprintf(str, "M_LO  ");
	else if (r.regID == (REG_MULTHI | REG_WIDE))    sprintf(str, "M_HIw ");
	else if (r.regID == (REG_MULTLO | REG_WIDE))    sprintf(str, "M_LOw ");
	else if (r.regID == REG_LLBIT)     sprintf(str, "LLBIT ");

	else if (r.regID >= REG_HOST)		 sprintf(str, "h%-3d  ", r.regID - REG_HOST);
	else if (r.regID >= REG_TEMP)		 sprintf(str, "t%-3d  ", r.regID - REG_TEMP);
	else if (r.regID >= REG_CO)          sprintf(str, "c%-3d  ", r.regID - REG_CO);
	else if (r.regID >= 0)
	{
		char f = 'r';
		char w = ' ';

		if (r.regID & REG_WIDE) w = 'w';
		if (r.regID & REG_FP) 	f = 'f';

		sprintf(str, "%c%-3d%c ", f, r.regID&~(REG_WIDE|REG_FP), w);
	}
	else                                 sprintf(str, "      ");
#endif
}

static void sprintRegList(char* str, const Instruction_t* const ins)
{
	int i;
	char* substr = str;

	substr += sprintf(substr, "<");

	for (i =0; i < 16; i++)
	{
		if ((ins->Rmask>>i)&1)
		{
			substr += sprintf(substr, "%s, ", arm_reg_a[i]);
		}
	}

	substr -= 2;

	sprintf(substr, ">");

}

static void sprintInstr(char* str, const Instruction_t* const ins)
{
	char ln[2] = {'\0','\0'};
	char writeBack[] = {0,0,0,0};
	char s[2];
	char wb = '\0';

	/*if (ins->Ln)
	{
		sprintf(ln,"l");
	}
	else
	{
		ln[0] = '\0';
	}*/

	if (ins->S) sprintf(s,"s");
		else s[0] = '\0';

	if (ins->instruction == ARM_LDM
			|| ins->instruction == ARM_STM )
	{
		if (ins->W) wb = '!';
		if (ins->PR == 1 && ins->U == 1) sprintf(writeBack, "ib%c", wb);
		else if (ins->PR == 1 && ins->U == 0
				&& ins->instruction == ARM_STM
				&& ins->R1.regID == REG_HOST_SP) sprintf(writeBack, "fd%c", wb);
		else if (ins->PR == 1 && ins->U == 0) sprintf(writeBack, "db%c", wb);
		else if (ins->PR == 0 && ins->U == 1) sprintf(writeBack, "%c", wb);	//ia [default]
		else if (ins->PR == 0 && ins->U == 0) sprintf(writeBack, "da%c", wb);

	}
	else if (ins->instruction == ARM_LDR
			|| ins->instruction == ARM_STR)
	{
		if (ins->W) wb = '!';
		if (ins->PR == 1 && ins->U == 1
				&& ins->immediate > 0) sprintf(writeBack, "ib%c", wb);
		else if (ins->PR == 1 && ins->U == 0
				&& ins->immediate > 0) sprintf(writeBack, "db%c", wb);
		else if (ins->PR == 0 && ins->U == 1
				&& ins->immediate > 0) sprintf(writeBack, "%c", wb);	//ia [default]
		else if (ins->PR == 0 && ins->U == 0
				&& ins->immediate > 0) sprintf(writeBack, "da%c", wb);
	}
	else
	{
		writeBack[0] = '\0';
	}
	sprintf(str, "%s%s%s%s%s", Instruction_ascii[STRIP(ins->instruction)], ln, arm_cond[ins->cond],s, writeBack);
}

//------------------------------------------------------------------

Instruction_t* newInstrCopy(const Instruction_t* ins)
{
	Instruction_t* newInstr = (Instruction_t*)malloc(sizeof(Instruction_t));
	memcpy(newInstr, ins, sizeof(Instruction_t));
#if USE_INSTRUCTION_COMMENTS
	newInstr->comment[0] = 0;
#endif
	return newInstr;
}
Instruction_t* Instr(Instruction_t* ins, const Instruction_e ins_e, const Condition_e cond, const regID_t Rd1, const regID_t R1, const regID_t R2)
{
	ins->instruction = ins_e;
	ins->cond        = cond;
	ins->Rd1.regID   = Rd1;
	ins->R1.regID    = R1;
	ins->R2.regID    = R2;

	ins->I = 0;
	ins->S = 0;
	ins->immediate = 0;

	switch (ins_e)
	{
	case MIPS_SLL:
	case MIPS_SLLV:
		ins-> shiftType = LOGICAL_LEFT; break;
	case MIPS_SRL:
	case MIPS_SRLV:
		ins-> shiftType = LOGICAL_RIGHT; break;
	case MIPS_SRA:
	case MIPS_SRAV:
		ins-> shiftType = ARITHMETIC_RIGHT; break;
	case ARM_TST:
	case ARM_TEQ:
	case ARM_CMP:
	case ARM_CMN:
		assert(Rd1 == REG_NOT_USED); break;

	case ARM_MOV:
	case ARM_MVN:
		assert(R1 == REG_NOT_USED); break;
	case ARM_LDR_LIT:
	case ARM_STR_LIT:

		break;
	default: break;
	}

	return ins;
}

Instruction_t* InstrI(Instruction_t* ins, const Instruction_e ins_e, const Condition_e cond, const regID_t Rd1, const regID_t R1, const regID_t R2, const int32_t imm)
{
	ins->instruction = ins_e;
	ins->cond        = cond;
	ins->immediate   = imm;
	ins->Rd1.regID   = Rd1;
	ins->R1.regID    = R1;
	ins->R2.regID    = R2;

	ins->I = 1;
	ins->S = 0;

	if (ins_e != ARM_B) ins->Ln = 0U;

	switch (ins_e)
	{
	case MIPS_SLL:
	case MIPS_SLLV:
		ins-> shiftType = LOGICAL_LEFT; break;
	case MIPS_SRL:
	case MIPS_SRLV:
		ins-> shiftType = LOGICAL_RIGHT; break;

	case ARM_MOV:
	case ARM_MVN:
		assert(R1 == REG_NOT_USED); break;

	case ARM_LDR:
	case ARM_STR:
	case ARM_LDR_LIT:
	case ARM_STR_LIT:
		if (imm < 0)
		{
			ins->immediate = -ins->immediate;
			ins->U = 0;
		}
		break;
	case ARM_B:
		ins->I = 0; break;
	default: break;
	}

	return ins;
}

Instruction_t* InstrS(Instruction_t* ins, const Instruction_e ins_e, const Condition_e cond, const regID_t Rd1, const regID_t R1, const regID_t R2)
{
	ins->instruction = ins_e;
		ins->cond        = cond;
		ins->Rd1.regID   = Rd1;
		ins->R1.regID    = R1;
		ins->R2.regID    = R2;

		ins->I = 0;
		ins->S = 1;

		switch (ins_e)
		{
		case MIPS_SLL:
		case MIPS_SLLV:
			ins-> shiftType = LOGICAL_LEFT; break;
		case MIPS_SRL:
		case MIPS_SRLV:
			ins-> shiftType = LOGICAL_RIGHT; break;
		case MIPS_SRA:
		case MIPS_SRAV:
			ins-> shiftType = ARITHMETIC_RIGHT; break;
		case ARM_TST:
		case ARM_TEQ:
		case ARM_CMP:
		case ARM_CMN:
			assert(Rd1 == REG_NOT_USED); break;

		case ARM_MOV:
		case ARM_MVN:
			assert(R1 == REG_NOT_USED); break;
		case ARM_LDR_LIT:
		case ARM_STR_LIT:

			break;
		default: break;
		}

		return ins;
}

Instruction_t* InstrIS(Instruction_t* ins, const Instruction_e ins_e, const Condition_e cond, const regID_t Rd1, const regID_t R1, const regID_t R2, const int32_t imm)
{
	ins->instruction = ins_e;
	ins->cond        = cond;
	ins->immediate   = imm;
	ins->Rd1.regID   = Rd1;
	ins->R1.regID    = R1;
	ins->R2.regID    = R2;

	ins->I = 1;
	ins->S = 1;

	switch (ins_e)
	{
	case MIPS_SLL:
	case MIPS_SLLV:
		ins-> shiftType = LOGICAL_LEFT; break;
	case MIPS_SRL:
	case MIPS_SRLV:
		ins-> shiftType = LOGICAL_RIGHT; break;

	case ARM_MOV:
	case ARM_MVN:
		assert(R1 == REG_NOT_USED); break;

	case ARM_LDR:
	case ARM_STR:
	case ARM_LDR_LIT:
	case ARM_STR_LIT:
		if (imm < 0)
		{
			ins->immediate = -ins->immediate;
			ins->U = 0;
		}
		break;
	case ARM_B:
		ins->I = 0; break;
	default: break;
	}

	return ins;
}

Instruction_t* InstrIntB(Instruction_t* ins, const Condition_e cond, const Instruction_t* find_ins)
{
	ins->instruction = DR_INT_BRANCH;
	ins->cond = cond;
	ins->branchToThisInstruction = (Instruction_t*)find_ins;
	return ins;
}

Instruction_t* InstrIntBL(Instruction_t* ins, const Condition_e cond, const Instruction_t* find_ins)
{
	ins->instruction = DR_INT_BRANCH_LINK;
	ins->cond = cond;
	ins->branchToThisInstruction = (Instruction_t*)find_ins;
	return ins;
}

Instruction_t* InstrB(Instruction_t* ins, const Condition_e cond, const int32_t offset, const uint32_t absolute)
{
	ins->instruction = ARM_B;
	ins->cond        = cond;
	ins->Rd1.regID   = REG_NOT_USED;
	ins->R1.regID    = REG_NOT_USED;
	ins->R2.regID    = REG_NOT_USED;
	ins->I = absolute;
	ins->offset = offset;
	ins->Ln = 0U;

	return ins;
}

Instruction_t* InstrBX(Instruction_t* ins, const Condition_e cond, const regID_t reg)
{
	ins->instruction = ARM_BX;
	ins->cond        = cond;
	ins->Rd1.regID   = REG_NOT_USED;
	ins->R1.regID    = reg;
	ins->R2.regID    = REG_NOT_USED;
	ins->I = 0;
	ins->offset = 0;
	ins->Ln = 0;

	return ins;
}

Instruction_t* InstrBL(Instruction_t* ins, const Condition_e cond, const int32_t offset, const uint32_t absolute)
{
	ins->instruction = ARM_BL;
	ins->cond        = cond;
	ins->Rd1.regID   = REG_NOT_USED;
	ins->R1.regID    = REG_NOT_USED;
	ins->R2.regID    = REG_NOT_USED;
	ins->I = absolute;
	ins->Ln = 1;
	ins->offset = offset;

	return ins;
}

Instruction_t* InstrPUSH(Instruction_t* ins, const Condition_e cond, const uint32_t Rmask)
{
	assert(Rmask < (1<<16));

	ins->instruction = ARM_STM;
	ins->cond = cond;
	ins->R1.regID = REG_HOST_SP;
	ins->Rmask = Rmask;
	ins->W = 1;
	ins->PR = 1;
	ins->U = 0;

	return ins;

}

Instruction_t* newEmptyInstr()
{
	Instruction_t* newInstr;

	newInstr = (Instruction_t*)malloc(sizeof(Instruction_t));
	memset(newInstr, 0, sizeof(Instruction_t));

	if (newInstr == NULL)
	{
		printf("Failed to malloc memory for Instruction\n");
		return NULL;
	}

	newInstr->nextInstruction = NULL;
	newInstr->instruction = DR_UNKNOWN;
	newInstr->cond = AL;
	newInstr->immediate = 0;
	newInstr->shift = 0;
	newInstr->Rmask = 0;
	newInstr->Rd1.regID = REG_NOT_USED;
	newInstr->Rd2.regID = REG_NOT_USED;
	newInstr->R1.regID = REG_NOT_USED;
	newInstr->R2.regID = REG_NOT_USED;
	newInstr->R3.regID = REG_NOT_USED;
	newInstr->R4.regID = REG_NOT_USED;

	newInstr->A=0;			// Accumulate
	newInstr->B=0;			// Byte/Word bit, 1 = byte
	newInstr->I=0;			// Immediate
	newInstr->Ln=0;			// Link bit for branch
	newInstr->PR=1;			// Pre/Post increment, 1 for pre
	newInstr->S=0;			// Set condition flags
	newInstr->U=1;			// Up/Down, set for inc, clear for decrement
	newInstr->W=0;			// Writeback bit set to write to base register

#if USE_INSTRUCTION_INIT_REGS
	newInstr->Rd1_init.regID = REG_NOT_USED;
	newInstr->Rd2_init.regID = REG_NOT_USED;
	newInstr->R1_init.regID = REG_NOT_USED;
	newInstr->R2_init.regID = REG_NOT_USED;
	newInstr->R3_init.regID = REG_NOT_USED;
#endif

#if USE_INSTRUCTION_COMMENTS
	if (currentTranslation)
	strcpy(newInstr->comment, currentTranslation);
#endif

	return newInstr;

}



Instruction_t* newInstr(const Instruction_e ins, const Condition_e cond, const regID_t Rd1, const regID_t R1, const regID_t R2)
{
	Instruction_t* newIns = newEmptyInstr();

	return Instr(newIns, ins, cond, Rd1, R1, R2);
}

Instruction_t* newInstrI(const Instruction_e ins, const Condition_e cond, const regID_t Rd1, const regID_t R1, const regID_t R2, const int32_t imm)
{
	Instruction_t* newIns = newEmptyInstr();

	return InstrI(newIns, ins, cond, Rd1, R1, R2, imm);
}

Instruction_t* newInstrS(const Instruction_e ins, 	const Condition_e cond, const regID_t Rd1, const regID_t R1, const regID_t R2)
{
	Instruction_t* newIns = newEmptyInstr();

	Instr(newIns, ins, cond, Rd1, R1, R2);
	newIns->S = 1;

	return newIns;
}

Instruction_t* newInstrIS(const Instruction_e ins, 	const Condition_e cond, const regID_t Rd1, const regID_t R1, const regID_t R2, const int32_t imm)
{
	Instruction_t* newIns = newEmptyInstr();
	InstrI(newIns, ins, cond, Rd1, R1, R2, imm);
	newIns->S = 1;

	return newIns;
}

Instruction_t* newInstrIntB(const Condition_e cond, const Instruction_t* ins)
{
	Instruction_t* newInstr = newEmptyInstr();
	return InstrIntB(newInstr, cond, ins);
}

Instruction_t* newInstrIntBL(const Condition_e cond, const Instruction_t* ins)
{
	Instruction_t* newInstr = newEmptyInstr();
	return InstrIntBL(newInstr, cond, ins);
}

/*
 * offset: 		The number of instructions to branch over
 * absolute:	=1? Treats offset as an address
 */
Instruction_t* newInstrB(const Condition_e cond, const int32_t offset, const uint32_t absolute)
{
	Instruction_t* newInstr = newEmptyInstr();

	return InstrB(newInstr, cond, offset, absolute);
}

Instruction_t* newInstrBX(const Condition_e cond, const regID_t reg)
{
	Instruction_t* newInstr = newEmptyInstr();

	return InstrBX(newInstr, cond, reg);
}

/*
 * offset: 		The number of instructions to branch over
 * absolute:	=1? Treats offset as an address
 */
Instruction_t* newInstrBL(const Condition_e cond, const int32_t offset, const uint32_t absolute)
{
	Instruction_t* newInstr = newEmptyInstr();

	return InstrBL(newInstr, cond, offset, absolute);
}

Instruction_t* newInstrPUSH(const Condition_e cond, const uint32_t Rmask)
{
	Instruction_t* newInstr = newEmptyInstr(0);

	assert(Rmask < (1<<16));

	newInstr->instruction = ARM_STM;
	newInstr->cond = cond;
	newInstr->R1.regID = REG_HOST_SP;
	newInstr->Rmask = Rmask;
	newInstr->W = 1;
	newInstr->PR = 1;
	newInstr->U = 0;

	return newInstr;

}

Instruction_t* newInstrPOP(const Condition_e cond, const uint32_t Rmask)
{
	Instruction_t* newInstr = newEmptyInstr(0);

	assert(Rmask < (1<<16));

	newInstr->instruction = ARM_LDM;
	newInstr->cond = cond;
	newInstr->R1.regID = REG_HOST_SP;
	newInstr->Rmask = Rmask;
	newInstr->W = 1;
	newInstr->PR = 0;
	newInstr->U = 1;

	return newInstr;

}

void CodeSeg_print(const code_seg_t* const codeSegment)
{
	Instruction_t*ins;

	ins = codeSegment->Intermcode;
	int x=0;

	while (ins && x< 1000)
	{
		printf_Intermediate(ins, !x );

		ins = ins->nextInstruction;
		x++;
	}
}

void printf_Intermediate(const Instruction_t* const ins, uint8_t heading)
{

	if (heading)
	{
		#if USE_INSTRUCTION_COMMENTS
			printf("           \t");
		#endif

#if USE_INSTRUCTION_INIT_REGS
	#if SHOW_PRINT_INT_CONST
		printf("command   Rd1                     Rd2                     R1                      R2                      R3                       A B I Ln PR S U W immediate                 shift\n");
	#else
		printf("command   Rd1         Rd2         R1          R2          R3           A B I Ln PR S U W immediate                 shift\n");
	#endif
#else
	#if SHOW_PRINT_INT_CONST
		printf("command   Rd1                     Rd2                     R1                      R2                      R3                       A B I Ln PR S U W immediate                 shift\n");
	#else
		printf("command   Rd1   Rd2   R1    R2    R3     A B I Ln PR S U W immediate                 shift\n");
	#endif
#endif
	}

	#define SZE 50
	char rd1[SZE], rd2[SZE], r1[SZE], r2[SZE] , r3[SZE];
	char instruction[10];
	char buffer[100];
	char offset[25];
	char shift[25];

	sprintInstr(instruction, ins);

	sprintf_reg_t(rd1, ins->Rd1);
	sprintf_reg_t(rd2, ins->Rd2);
	sprintf_reg_t(r1, ins->R1);
	sprintf_reg_t(r2, ins->R2);
	sprintf_reg_t(r3, ins->R3);

#if USE_INSTRUCTION_INIT_REGS
	sprintf_reg_t(rd1 + strlen(rd1), ins->Rd1_init);
	sprintf_reg_t(rd2 + strlen(rd2), ins->Rd2_init);
	sprintf_reg_t(r1 + strlen(r1), ins->R1_init);
	sprintf_reg_t(r2 + strlen(r2), ins->R2_init);
	sprintf_reg_t(r3 + strlen(r3), ins->R3_init);
#endif

	sprintf(offset, "%-11d (0x%08X)", ins->offset, ins->offset);
	sprintf(shift, "%-2d (0x%02X)", ins->shift, ins->shift);

	#if USE_INSTRUCTION_COMMENTS
		if (ins->comment[0] == '0')
		{
			if (ins->outputAddress)
			{
				printf("\n%s0x%08x \t", ins->comment, ins->outputAddress);
			}
			else
			{
				printf("\n%s           \t", ins->comment);
			}
		}
		else
		{
			if (ins->outputAddress)
			{
				printf("0x%08x \t", ins->outputAddress);
			}
			else
			{
				printf("           \t");
			}
		}
	#endif

	if (ins->instruction == ARM_LDM
			|| ins->instruction == ARM_STM )
	{
		sprintRegList(buffer, ins);
		#if SHOW_PRINT_INT_CONST
		printf("%-9s %s%s%s%-100s", instruction, rd1, rd2, r1, buffer);
		#else
		printf("%-9s %s%s%s%-71s", instruction, rd1, rd2, r1, buffer);
		#endif



	}
	else
	{
		#if SHOW_PRINT_INT_CONST
			printf("%-9s %-24s%-24s%-24s%-24s%-24s %d %d %d %d  %d  %d %d %d %-25s %-9s"
				, instruction
				, rd1, rd2, r1, r2, r3
				, ins->A
				, ins->B
				, ins->I
				, ins->Ln
				, ins->PR
				, ins->S
				, ins->U
				, ins->W
				, offset, shift);
		#else
			printf("%-9s %-6s%-6s%-6s%-6s%-6s %d %d %d %d  %d  %d %d %d %-25s %-9s"
				, instruction
				, rd1, rd2, r1, r2, r3
				, ins->A
				, ins->B
				, ins->I
				, ins->Ln
				, ins->PR
				, ins->S
				, ins->U
				, ins->W
				, offset, shift);
		#endif
	}

	#if USE_INSTRUCTION_COMMENTS
		if (ins->comment[0] == '0')
		{
			printf("\n");
		}
		else
		{
			printf(" %s\n", ins->comment);
		}
	#endif
}


void Intermediate_Literals_print(const code_seg_t* const codeSegment)
{
	literal_t*literal;
	uint32_t x = 0U;

	literal = codeSegment->literals;


	if (NULL == literal)
	{
		printf("No literals for current segment\n");
	}

	while (literal)
	{
		printf("%2d.  %11d 0x%08x\n", x, literal->value, literal->value);
		x++;

		literal = literal->next;
	}
}

Instruction_t* InstrFree(code_seg_t* const codeSegment, Instruction_t* ins)
{
	if (codeSegment)
	{
		Instruction_t* in = codeSegment->Intermcode;

		Instruction_t* prev = NULL;
		if (in == ins)
		{
			codeSegment->Intermcode = codeSegment->Intermcode->nextInstruction;

			free(in);
			return codeSegment->Intermcode;
		}
		else
		{
			//Search for instruction before one to be deleted
			while (in)
			{
				//Make a note of the instruction before the one to be deleted
				if (in->nextInstruction == ins) prev = in;

				//need to fix any intra-segment branches that point to instruction to be deleted
				if (in->branchToThisInstruction == ins)
				{
					in->branchToThisInstruction = ins->nextInstruction;
					assert(ins->nextInstruction);
				}

				in = in->nextInstruction;
			}

			// if we have the previous instruction then point its next to instruction after one to be deleted.
			if (prev) prev->nextInstruction = ins->nextInstruction;

			free(ins);
			return prev->nextInstruction;
		}
	}

	free(ins);
	return NULL;
}
