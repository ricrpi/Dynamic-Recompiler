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

void Translate_init(code_seg_t* const codeSegment)
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

#if defined(USE_INSTRUCTION_INIT_REGS)
		memcpy(&newInstruction->Rd1_init,&newInstruction->Rd1, sizeof(reg_t));
		memcpy(&newInstruction->Rd2_init,&newInstruction->Rd2, sizeof(reg_t));
		memcpy(&newInstruction->R1_init,&newInstruction->R1, sizeof(reg_t));
		memcpy(&newInstruction->R2_init,&newInstruction->R2, sizeof(reg_t));
		memcpy(&newInstruction->R3_init,&newInstruction->R3, sizeof(reg_t));
#endif

#if defined(USE_INSTRUCTION_COMMENTS)
		sprintf_mips(newInstruction->comment, (uint32_t)(codeSegment->MIPScode + x), *(codeSegment->MIPScode + x));
#endif
		if (NULL == prevInstruction)
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
