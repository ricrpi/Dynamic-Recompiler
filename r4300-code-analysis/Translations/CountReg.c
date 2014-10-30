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
#if 0
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
				//TODO QADD
				abort();
			}
			else
			{
				newInstruction = newInstrIS(ARM_ADD,AL,REG_COUNT,REG_COUNT,REG_NOT_USED, uiCountFrequency);
				ADD_LL_NEXT(newInstruction, ins);

				instrCountRemaining -= uiCountFrequency;

				ins = insertCall_To_C(codeSegment, ins, PL, (uint32_t)cc_interrupt);
				instrCount = 0;
			}
		}

		ins = ins->nextInstruction;
	}
#endif

	//now add a final update before end of function
	if (instrCountRemaining)
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
			newInstruction = newInstrIS(ARM_ADD, AL, REG_COUNT, REG_COUNT, REG_NOT_USED, instrCountRemaining&0xff);
			newInstruction->nextInstruction = ins->nextInstruction;
			ADD_LL_NEXT(newInstruction, ins);

			if (instrCountRemaining > 255)
			{
				newInstruction = newInstrIS(ARM_ADD, AL, REG_COUNT, REG_COUNT, REG_NOT_USED, instrCountRemaining&0xff00);
				ADD_LL_NEXT(newInstruction, ins);
			}

			//newInstruction = newInstr(ARM_CMP, AL, REG_NOT_USED, REG_COMPARE, REG_COUNT);
			//ADD_LL_NEXT(newInstruction, ins);

			// We need to set IP7 of the Cause Register and call cc_interrupt()
			newInstruction = newInstrI(ARM_ORR, AL, REG_CAUSE, REG_CAUSE, REG_NOT_USED, 0x8000);
			ADD_LL_NEXT(newInstruction, ins);

			#if defined(USE_INSTRUCTION_COMMENTS)
				currentTranslation = "Call cc_interrupt()";
			#endif

			ins = insertCall_To_C(codeSegment, ins, PL, (uint32_t)cc_interrupt);

			return;
		}
	}
}
