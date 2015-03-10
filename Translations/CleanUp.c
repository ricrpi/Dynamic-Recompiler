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

void Translate_CleanUp(code_seg_t* const codeSegment)
{
	Instruction_t*ins;
	ins = codeSegment->Intermcode;

#if defined(USE_INSTRUCTION_COMMENTS)
	currentTranslation = "CleanUp";
#endif

	while (ins)
	{
		if (NO_OP == ins->instruction)
		{
			ins = InstrFree(codeSegment, ins);
		}
		else if (SLL == ins->instruction && ins->Rd1.regID == 0)	// MIPS NO_OP
		{
			ins = InstrFree(codeSegment, ins);
		}
		else if (LUI == ins->instruction)
		{
			ins = InstrFree(codeSegment, ins);
		}
		else if (CACHE == ins->instruction)
		{
			ins = InstrFree(codeSegment, ins);
		}
		else if (SYSCALL == ins->instruction)
		{
			ins = InstrFree(codeSegment, ins);
		}
		else if (BREAK == ins->instruction)
		{
			ins = InstrFree(codeSegment, ins);
		}
		else if (SYNC == ins->instruction)
		{
			ins = InstrFree(codeSegment, ins);
		}
		else
		{
			ins = ins->nextInstruction;
		}
			/*
			case TLBR: break;
			case TLBWI: break;
			case TLBWR: break;
			case TLBP: break;

			*/
	}
}
