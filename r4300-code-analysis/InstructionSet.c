/*
 * InstructionSet.c
 *
 *  Created on: 2 Jul 2014
 *      Author: rjhender
 */

#include <stdio.h>
#include <stdlib.h>
#include "InstructionSet.h"
#include "InstructionSetARM6hf.h"
#include "CodeSegments.h"

static void sprintReg(char* str, reg_t r)
{
	if (r == REG_HOST_FP)        sprintf(str, "fp     ");
	else if (r == REG_HOST_SP)   sprintf(str, "sp     ");
	else if (r == REG_HOST_LR)   sprintf(str, "lr     ");
	else if (r == REG_HOST_PC)   sprintf(str, "pc     ");
	else if (r == REG_COUNT)     sprintf(str, "COUNT  ");
	else if (r == REG_CAUSE)     sprintf(str, "CAUSE  ");
	else if (r == REG_CONTEXT)   sprintf(str, "CONTXT ");
	else if (r == REG_COMPARE)   sprintf(str, "COMPAR ");
	else if (r == REG_STATUS)    sprintf(str, "STATUS ");
	else if (r >= C0_REG_OFFSET) sprintf(str, "c%-3d   ", r - C0_REG_OFFSET);
	else if (r >= FP_REG_OFFSET) sprintf(str, "f%-3d   ", r - FP_REG_OFFSET);
	else if (r >= 0)             sprintf(str, "r%-3d   ", r);
	else                         sprintf(str, "       ");
}
static void sprintRegList(char* str, Instruction_t*ins)
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

static void sprintInstr(char* str, Instruction_t*ins)
{
	char ln[2];
	char writeBack[4];
	char s[2];
	char wb = '\0';

	if (ins->Ln) sprintf(ln,"l");
	else ln[0] = '\0';

	if (ins->S) sprintf(s,"s");
		else s[0] = '\0';

	if (ins->instruction == ARM_LDR
			|| ins->instruction == ARM_LDM
			|| ins->instruction == ARM_STR
			|| ins->instruction == ARM_STM )
	{
		if (ins->W) wb = '!';
		if (ins->PR == 1 && ins->U == 1) sprintf(writeBack, "ib%c", wb);
		if (ins->PR == 1 && ins->U == 0) sprintf(writeBack, "db%c", wb);
		if (ins->PR == 0 && ins->U == 1) sprintf(writeBack, "%c", wb);	//ia [default]
		if (ins->PR == 0 && ins->U == 0) sprintf(writeBack, "da%c", wb);
		if (ins->PR == 1
				&& ins->U == 0
				&& ins->instruction == ARM_STM
				&& ins->R1 == REG_HOST_SP) sprintf(writeBack, "fd%c", wb);
	}
	else
	{
		writeBack[0] = '\0';
	}
	sprintf(str, "%s%s%s%s%s", Instruction_ascii[STRIP(ins->instruction)], ln, arm_cond[ins->cond],s, writeBack);
}

//------------------------------------------------------------------

Instruction_t* newInstr()
{
	Instruction_t* newInstr;

	newInstr = (Instruction_t*)malloc(sizeof(Instruction_t));

	if (newInstr == NULL)
	{
		printf("Failed to malloc memory for Instruction\n");
		return NULL;
	}

	newInstr->nextInstruction = NULL;
	newInstr->instruction = UNKNOWN;
	newInstr->cond = AL;
	newInstr->immediate = 0;
	newInstr->shift = 0;
	newInstr->Rmask = 0;
	newInstr->Rd1 = REG_NOT_USED;
	newInstr->Rd2 = REG_NOT_USED;
	newInstr->R1 = REG_NOT_USED;
	newInstr->R2 = REG_NOT_USED;
	newInstr->R3 = REG_NOT_USED;

	newInstr->A=0;			// Accumulate
	newInstr->B=0;			// Byte/Word bit, 1 = byte
	newInstr->I=0;			// Immediate
	newInstr->Ln=0;			// Link bit for branch
	newInstr->PR=0;			// Pre/Post increment, 1 for pre
	newInstr->S=0;			// Set condition flags
	newInstr->U=1;			// Up/Down, set for inc, clear for decrement
	newInstr->W=0;			// Writeback bit set to write to base register

	return newInstr;

}

void Intermediate_print(code_seg_t* codeSegment)
{
	Instruction_t*ins;

	ins = codeSegment->Intermcode;
	int x=100;

	printf("command   Rd1    Rd2    R1     R2     R3     immediate\n");

	while (ins && x>0)
	{
		char rd1[8], rd2[8], r1[8], r2[8] , r3[8];
		char instruction[10];
		char buffer[100];
		char offset[25];

		sprintInstr(instruction, ins);

		sprintReg(rd1, ins->Rd1);
		sprintReg(rd2, ins->Rd2);
		sprintReg(r1, ins->R1);
		sprintReg(r2, ins->R2);
		sprintReg(r3, ins->R3);

		if (ins->offset) sprintf(offset, "%d (0x%X)", ins->offset, ins->offset);
		else offset[0] = '\0';

		if (ins->instruction == ARM_LDM
				|| ins->instruction == ARM_STM )
		{
			sprintRegList(buffer, ins);
			printf("%-9s %s%s%s%s\n", instruction, rd1, rd2, r1, buffer);
		}
		else
		{
			printf("%-9s %s%s%s%s%s%s\n", instruction, rd1, rd2, r1, r2, r3, offset);
		}

		ins = ins->nextInstruction;
		x--;
	}
}
