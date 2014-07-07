/*
 * Translate.c
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

/*
 * Code to generate Emulation-time Code within 32MB of dynamically compiled code.
 *
 * When a Code segment is written into memory, it may not be known where it branches to. In which case
 * It will be necessary to branch to this Stub code.
 *
 * Requirements:
 * 	The address of the branch instruction to patch
 * 	The tgt address/offset to branch to
 *
 * Method:
 *
 * 	The address of the instruction to patch is LR-4
 *
 * 	If we know what the current segment is then we can lookup the target address/offset from the raw MIPS code
 *
 */
Instruction_t* Generate_BranchStubCode()
{
	Instruction_t* firstInstruction;
	Instruction_t* newInstruction;

	firstInstruction 	= newInstrPUSH(AL, REG_HOST_STM_LR);

	newInstruction 		= newInstr(ADD, AL, REG_HOST_LR, REG_HOST_PC, REG_NOT_USED, 4);

	firstInstruction->nextInstruction = newInstruction;

	newInstruction 		= newInstrPOP(AL, REG_HOST_STM_PC);

	return firstInstruction;

}

//=============================================================

static Instruction_t* insertCall(Instruction_t* ins, Condition_e cond, int32_t offset)
{
	Instruction_t* newInstruction;

	//push lr
	newInstruction 	= newInstrPUSH(AL, REG_HOST_STM_LR);

	//insert into Linked List
	newInstruction->nextInstruction = ins->nextInstruction;
	ins->nextInstruction = newInstruction;
	ins = ins->nextInstruction;

	//set lr
	newInstruction 	= newInstr(ADD, AL, REG_HOST_LR, REG_HOST_PC, REG_NOT_USED, 4);

	//insert into Linked List
	newInstruction->nextInstruction = ins->nextInstruction;
	ins->nextInstruction = newInstruction;
	ins = ins->nextInstruction;

	// load function address from [fp + offset] into PC
	newInstruction 	= newInstr(ARM_LDR, cond, REG_HOST_PC, REG_HOST_FP, REG_NOT_USED, offset);

	//insert into Linked List
	newInstruction->nextInstruction = ins->nextInstruction;
	ins->nextInstruction = newInstruction;
	ins = ins->nextInstruction;

	// pop lr
	newInstruction 	= newInstrPOP(AL, REG_HOST_STM_LR);

	//insert into Linked List
	newInstruction->nextInstruction = ins->nextInstruction;
	ins->nextInstruction = newInstruction;

	return ins->nextInstruction;
}


/*
 * MIPS4300 executes instruction after a branch or jump if it is not LINK_LIKELY
 * Therefore the instruction after the branch may need to be moved into the segment.
 *
 * For ARM we shall use the PSR to signal when to execute the DelaySlot instruction(s)
 *
 * i.e.
 *
 * ...								...
 * ADD 	R1	R2	R3					ADD		R1	R2	R3
 * SUBI	R1	R1	#4					SUMI	R1	R1	#4
 * BNE	R1			#-1		==>		ADDI	R2	R2 	#3
 * ====New Segment====				BNE		R1		#-1
 * ADDI	R2	R2	#3					msr		s		#1		// TODO lookup what imm is for setting Z
 * ...								====New Segment====		// Will be a BLOCK_START_CONT Segment
 * ...								ADDIeq	R2	R2	#3		// the delaySlot instruction(s)
 * ...								...
 *
 * All segments must clear the Z status flag before jumping to a BLOCK_START_CONT segment.
 *
 */
void Translate_DelaySlot(code_seg_t* codeSegment)
{
	Instruction_e ops = ops_type(*(codeSegment->MIPScode + codeSegment->MIPScodeLen -1));
	Instruction_t* newInstruction;
	Instruction_t* ins;

	ins 			= codeSegment->Intermcode;

	if (ins == NULL)
		{
			printf("Not initialized this code segment. Please run 'optimize intermediate'\n");
			return;
		}

	//if the last instruction is a branch or call and is not Likely
	if ((ops & (OPS_BRANCH | OPS_CALL))
			&& !(ops & OPS_LIKELY))
	{
		newInstruction 	= newEmptyInstr();

		//goto second last instruction
		while (ins->nextInstruction->nextInstruction)
			ins = ins->nextInstruction;

		mips_decode(*(codeSegment->MIPScode + codeSegment->MIPScodeLen), newInstruction);

		newInstruction->nextInstruction = ins->nextInstruction;
		ins->nextInstruction = newInstruction;

		//Set Status Register setting
		/*	APSR.N = imm32<31>;
			APSR.Z = imm32<30>;
			APSR.C = imm32<29>;
			APSR.V = imm32<28>;
			APSR.Q = imm32<27>;
		 */
		newInstruction 	= newInstr(ARM_MSR,AL, REG_NOT_USED, REG_NOT_USED, REG_NOT_USED, 0x40);
		newInstruction->rotate = 4;	// To load into bits 24-31
		newInstruction->I = 1;

		newInstruction->nextInstruction = ins->nextInstruction;
		ins->nextInstruction = newInstruction;
	}
	else if(codeSegment->blockType == BLOCK_CONTINUES)	//TODO Optimize - We do not need to mess with the status register if not branching to BLOCK_START_CONT segment
	{
		//goto second last instruction
		while (ins->nextInstruction->nextInstruction)
			ins = ins->nextInstruction;

		//Clear Status Register setting
		newInstruction 	= newInstr(ARM_MSR,AL, REG_NOT_USED, REG_NOT_USED, REG_NOT_USED, 0);
		newInstruction->rotate = 4;	// To load into bits 24-31
		newInstruction->I = 1;

		newInstruction->nextInstruction = ins->nextInstruction;
		ins->nextInstruction = newInstruction;
	}

	// Apply the conditional execution if this is a BLOCK_START_CONT segment.
	if (codeSegment->blockType == BLOCK_START_CONT){
		codeSegment->Intermcode->cond = NE;
	}
}

/*
 * MIPS4300 has a COUNT register that is decremented every instruction
 * when it underflows, an interrupt is triggered.
 */
void Translate_CountRegister(code_seg_t* codeSegment)
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
			Instruction_t* newInstruction 	= newEmptyInstr();

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
		Instruction_t* newInstruction 	= newEmptyInstr();

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
void Translate_32BitRegisters(code_seg_t* codeSegment)
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
void Translate_ReduceRegistersUsed(code_seg_t* codeSegment)
{
	Instruction_t*ins;
	ins = codeSegment->Intermcode;

	while (ins)
	{
		ins = ins->nextInstruction;
	}
}

void Translate_LoadStoreWriteBack(code_seg_t* codeSegment)
{
	Instruction_t*ins;
	ins = codeSegment->Intermcode;

	while (ins)
	{
		ins = ins->nextInstruction;
	}
}

/*
 *	Function to Translate memory operations into Emulated equivalent
 *
 *  Emulated memory access can be cached, non-cached or virtual
 *
 *  As the emulator memory will be mmaped to 0x80000000, non-cached need not do
 *  any translation. cached memory 'just needs' BIC Rd, R1, #1, LSL # 29
 *
 *  Virtual will need to call a function to lookup address
 */
void Translate_Memory(code_seg_t* codeSegment)
{
	Instruction_t*ins;
	ins = codeSegment->Intermcode;

	while (ins)
	{
		if (ins->nextInstruction->instruction == LW)
		{

		}

		ins = ins->nextInstruction;
	}
}

void Translate_init(code_seg_t* codeSegment)
{
	int x;
	Instruction_t*newInstruction;
	Instruction_t*prevInstruction = NULL;

	freeIntermediateInstructions(codeSegment);

	//now build new Intermediate code
	for (x=0; x < codeSegment->MIPScodeLen; x++)
	{
		newInstruction = newEmptyInstr();

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

void Translate(code_seg_t* codeSegment)
{
	Translate_init(codeSegment);

	Translate_CountRegister(codeSegment);
	Translate_DelaySlot(codeSegment);
	Translate_32BitRegisters(codeSegment);
	Translate_ReduceRegistersUsed(codeSegment);
}
