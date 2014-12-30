/*
 * Misc.c
 *
 *  Created on: 29 Nov 2014
 *      Author: rjhender
 */

#include "Translate.h"


void Translate_Generic(code_seg_t* const codeSegment)
{
	Instruction_t*ins;
	Instruction_t*new_ins;
	ins = codeSegment->Intermcode;

#if defined(USE_INSTRUCTION_COMMENTS)
	currentTranslation = "Generic";
#endif
	while (ins)
	{
		switch (ins->instruction)
		{
			case MFHI:
			case MTHI:
			case MFLO:
			case MTLO:
				TRANSLATE_ABORT();
				break;
			case MFC0:
				TRANSLATE_ABORT();
				break;
			case MTC0:
				if (ins->R1.regID == 0)
				{
					InstrI(ins, ARM_MOV, AL, ins->Rd1.regID, REG_NOT_USED, REG_NOT_USED, 0);
				}
				else
				{
					Instr(ins, ARM_MOV, AL, ins->Rd1.regID, REG_NOT_USED, ins->R1.regID);
				}
				break;
			case ERET:
				TRANSLATE_ABORT();
				break;
			//case CACHE: TRANSLATE_ABORT();	// This instruction is removed in CleanUp
				break;
		default: break;
		}

		ins = ins->nextInstruction;
	}
}
