/*
 * CountReg.c
 *
 *  Created on: 23 Oct 2014
 *      Author: rjhender
 */

#include "Translate.h"
#include "memory.h"


uint32_t bCountSaturates 	= 0;

/*
 * MIPS4300 has a COUNT register that is decremented every instruction
 * when it underflows, an interrupt is triggered.
 */
void Translate_CountRegister(code_seg_t* const codeSegment)
{
	Instruction_t*ins = codeSegment->Intermcode;
	uint32_t instrCount = 0;
	uint32_t instrCountRemaining = codeSegment->MIPScodeLen;

#if defined(USE_INSTRUCTION_COMMENTS)
	currentTranslation = "CountRegister";
#endif

	if (ins == NULL)
	{
		printf("Not initialized this code segment. Please run 'optimize intermediate'\n");
		return;
	}

	if (bCountSaturates)
	{
		printf("Optimize_CountRegister failed. Not implemented QADD \n");
		abort();
	}

	//loop through the instructions and update COUNT if it is copied from co-processor
	while (ins)
	{
		instrCount++;

		if (ins->instruction == MFC0 && ins->R1.regID == REG_COUNT)
		{
			//add COUNT update
			Instruction_t* newInstruction 	= newEmptyInstr();

			if (bCountSaturates)
			{
				//TODO QADD
				abort();
			}
			else
			{
				//TODO what can instrCount > codeSegment->MIPScodeLen?
				assert(instrCount <= codeSegment->MIPScodeLen);

				newInstruction = newInstrI(ARM_ADD, AL, REG_COUNT, REG_COUNT, REG_NOT_USED, instrCount);
				ADD_LL_NEXT(newInstruction, ins);

				instrCountRemaining -= instrCount;
			}
		}

		ins = ins->nextInstruction;
	}

	ins = codeSegment->Intermcode;

	//now add a final update before end of function
	if (instrCountRemaining > 0)
	{
		Instruction_t* newInstruction;

		//goto second last instruction
		if (ins->nextInstruction)
		{
			while (ins->nextInstruction->nextInstruction) ins = ins->nextInstruction;
		}

		//create COUNT update instructions
		if (bCountSaturates)
		{
			//TODO QSUB
		}
		else
		{
			if (instrCountRemaining > 255)
			{
				newInstruction = newInstrI(ARM_ADD, AL, REG_COUNT, REG_COUNT, REG_NOT_USED, instrCountRemaining&0xff00);
				ADD_LL_NEXT(newInstruction, ins);
			}

			newInstruction = newInstrIS(ARM_ADD, AL, REG_COUNT, REG_COUNT, REG_NOT_USED, instrCountRemaining&0xff);
			ADD_LL_NEXT(newInstruction, ins);

			#if defined(USE_INSTRUCTION_COMMENTS)
				currentTranslation = "Call cc_interrupt()";
			#endif

			ins = insertCall_To_C(codeSegment, ins, PL, (uint32_t)cc_interrupt, 0);

			return;
		}
	}
}
