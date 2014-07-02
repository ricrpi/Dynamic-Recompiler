/*
 * Optimize.c
 *
 *  Created on: 16 Apr 2014
 *      Author: rjhender
 */

#include "CodeSegments.h"
#include "InstructionSet.h"
#include "InstructionSetMIPS4.h"
#include "InstructionSetARM6hf.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include "memory.h"

typedef enum
{
	AVAILABLE,
	CLEAN,
	DIRTY,
	CONSTANT,
	RESERVED
} RegisterState;

uint32_t bCountSaturates = 0;
uint32_t uiCountFrequency = 40;	// must be less than 128 else may not be able to encode in imm8


//=============================================================

static Instruction_t* insertCall(Instruction_t* ins, Condition_e cond, int32_t offset)
{
	Instruction_t* newInstruction 	= newInstr();

	//push lr
	newInstruction->instruction = ARM_STM;
	newInstruction->Rmask = REG_HOST_STM_LR;
	newInstruction->R1 = REG_HOST_SP;
	newInstruction->W = 1;
	newInstruction->PR = 1;
	newInstruction->U = 0;

	//insert into Linked List
	newInstruction->nextInstruction = ins->nextInstruction;
	ins->nextInstruction = newInstruction;
	ins = ins->nextInstruction;

	newInstruction 	= newInstr();

	//set lr
	newInstruction->instruction = ADD;
	newInstruction->Rd1 = REG_HOST_LR;
	newInstruction->R1 = REG_HOST_PC;
	newInstruction->immediate = 4;

	//insert into Linked List
	newInstruction->nextInstruction = ins->nextInstruction;
	ins->nextInstruction = newInstruction;
	ins = ins->nextInstruction;

	newInstruction 	= newInstr();

	// load function address from [fp + offset] into PC
	newInstruction->instruction = ARM_LDR;
	newInstruction->Rd1 = REG_HOST_PC;
	newInstruction->R1 = REG_HOST_FP;
	newInstruction->offset = offset;
	newInstruction->cond = cond;

	//insert into Linked List
	newInstruction->nextInstruction = ins->nextInstruction;
	ins->nextInstruction = newInstruction;
	ins = ins->nextInstruction;

	newInstruction 	= newInstr();

	// pop lr
	newInstruction->instruction = ARM_LDM;
	newInstruction->Rmask = REG_HOST_STM_LR;
	newInstruction->R1 = REG_HOST_SP;
	newInstruction->W = 1;
	newInstruction->PR = 0;
	newInstruction->U = 1;

	//insert into Linked List
	newInstruction->nextInstruction = ins->nextInstruction;
	ins->nextInstruction = newInstruction;

	return ins->nextInstruction;
}


/*
 * MIPS4300 executes instruction after a branch or jump if it is not LINK_LIKELY
 * Therefore the instruction after the branch may need to be moved into the segment.
 */
void Optimize_DelaySlot(code_seg_t* codeSegment)
{
	Instruction_e ops = ops_type(*(codeSegment->MIPScode + codeSegment->MIPScodeLen -1));

	//if the last instruction is a branch or call and is not Likely
	if ((ops & (OPS_BRANCH | OPS_CALL))
			&& !(ops & OPS_LIKELY))
	{
		Instruction_t* newInstruction 	= newInstr();
		Instruction_t* ins 				= codeSegment->Intermcode;

		if (ins == NULL)
		{
			printf("Not initialized this code segment. Please run 'optimize intermediate'\n");
			return;
		}

		//goto second last instruction
		while (ins->nextInstruction->nextInstruction)
			ins = ins->nextInstruction;

		mips_decode(*(codeSegment->MIPScode + codeSegment->MIPScodeLen), newInstruction);

		newInstruction->nextInstruction = ins->nextInstruction;
		ins->nextInstruction = newInstruction;
	}
}

/*
 * MIPS4300 has a COUNT register that is decremented every instruction
 * when it underflows, an interrupt is triggered.
 */
void Optimize_CountRegister(code_seg_t* codeSegment)
{
	Instruction_t*ins = codeSegment->Intermcode;
	uint32_t instrCount =0;
	uint32_t instrCountRemaining = codeSegment->MIPScodeLen;

	if (ins == NULL)
	{
		printf("Not initialized this code segment. Please run 'optimize intermediate'\n");
		return;
	}

	if (bCountSaturates)
	{
		printf("Optimize_CountRegister failed. Not implemented QSUB \n");
		return;
	}

	//loop through the instructions and update COUNT every countFrequency

	while (ins->nextInstruction->nextInstruction)
	{
		instrCount++;

		if (instrCount >= uiCountFrequency && instrCountRemaining >= uiCountFrequency)
		{
			//add COUNT update
			Instruction_t* newInstruction 	= newInstr();

			if (bCountSaturates)
			{
				//TODO QSUB
			}
			else
			{
				newInstruction->instruction = ARM_SUB;
				newInstruction->S = 1;
				newInstruction->Rd1 = REG_COUNT;
				newInstruction->R1 = REG_COUNT;
				newInstruction->immediate = uiCountFrequency;
				instrCountRemaining -= uiCountFrequency;

				//insert into Linked List
				newInstruction->nextInstruction = ins->nextInstruction;
				ins->nextInstruction = newInstruction;
				ins = ins->nextInstruction;

				ins = insertCall(ins, MI, FUNC_GEN_INTERRUPT);
				instrCount = 0;
			}
		}

		ins = ins->nextInstruction;
	}
	//now add a final update before end of function

	if (instrCount && instrCountRemaining)
	{
		Instruction_t* newInstruction 	= newInstr();

		//create COUNT update instructions
		if (bCountSaturates)
		{
			//TODO QSUB
		}
		else
		{
			newInstruction->instruction = SUB;
			newInstruction->S = 1;
			newInstruction->offset = instrCountRemaining;
			newInstruction->Rd1 = REG_COUNT;
			newInstruction->R1 = REG_COUNT;

			newInstruction->nextInstruction = ins->nextInstruction;

			ins->nextInstruction = newInstruction;
			ins = ins->nextInstruction;

			ins = insertCall(ins, MI, FUNC_GEN_INTERRUPT);

			return;
		}
	}
}

/*
 * Function to turn 64bit registers into multiple 32-bit registers
 *
 */
void Optimize_32BitRegisters(code_seg_t* codeSegment)
{
	//Instruction_t* i = newInstr();

	Instruction_t*ins;
	ins = codeSegment->Intermcode;

	while (ins)
	{
	/*	switch (ins->instruction)
		{
		case DADDIU: //TODO this is an immediate.
			//Change into 32bit then add instruction to accumulate
			ins->instruction = ADDIU;

			i->instruction = ARM_ADC;
			i->M_Rs = 32 + ins->M_Rs;
			i->M_Rt = 32 + ins->M_Rt;
			i->Rd = 32 + ins->Rd;

			break;
		default:
			break;
		}
*/
		ins = ins->nextInstruction;
	}
}

/*
 * Function to re-number / reduce the number of registers so that they fit the HOST
 *
 * This function will need to scan a segment and when more than the number of spare
 * HOST registers is exceeded, choose to either save register(s) into the emulated registers
 * referenced by the Frame Pointer or push them onto the stack for later use.
 *
 * Pushing onto the stack may make it easier to use LDM/SDM where 32/64 bit is not compatible
 * with the layout of the emulated register space.
 *
 */
void Optimize_ReduceRegistersUsed(code_seg_t* codeSegment)
{
	Instruction_t*ins;
	ins = codeSegment->Intermcode;

	while (ins)
	{
		ins = ins->nextInstruction;
	}
}

void Optimize_LoadStoreWriteBack(code_seg_t* codeSegment)
{
	Instruction_t*ins;
	ins = codeSegment->Intermcode;

	while (ins)
	{
		ins = ins->nextInstruction;
	}
}

void Optimize_init(code_seg_t* codeSegment)
{
	int x;
	Instruction_t*newInstruction;
	Instruction_t*prevInstruction = NULL;

	freeIntermediateInstructions(codeSegment);

	//now build new Intermediate code
	for (x=0; x < codeSegment->MIPScodeLen; x++)
	{
		newInstruction = newInstr();

		mips_decode(*(codeSegment->MIPScode + x), newInstruction);

		if (x == 0)
		{
			codeSegment->Intermcode = newInstruction;

		}
		else
		{
			prevInstruction->nextInstruction = newInstruction;
		}
		prevInstruction = newInstruction;
	}

	return;
}

void Optimize(code_seg_t* codeSegment)
{
	Optimize_init(codeSegment);

	Optimize_CountRegister(codeSegment);
	Optimize_DelaySlot(codeSegment);
	Optimize_32BitRegisters(codeSegment);
	Optimize_ReduceRegistersUsed(codeSegment);
}

