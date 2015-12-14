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

#if USE_INSTRUCTION_COMMENTS
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

		if (ins->instruction == MIPS_MFC0 && ins->R1.regID == REG_COUNT)
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

		//goto instruction before a branch or jump
		if (ins->nextInstruction)
		{
			while (!((ins->nextInstruction->instruction & OPS_BRANCH)
							|| (ins->nextInstruction->instruction &OPS_JUMP)))
			{
				ins = ins->nextInstruction;
			}
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

			#if USE_INSTRUCTION_COMMENTS
				currentTranslation = "Call cc_interrupt()";
			#endif

			ins = insertCall_To_C(codeSegment, ins, PL, (uint32_t)cc_interrupt, 0);

			return;
		}
	}
}
