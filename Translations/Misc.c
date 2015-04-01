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
#include "InstructionSet_ascii.h"


void Translate_Generic(code_seg_t* const codeSegment)
{
	Instruction_t*ins;
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
