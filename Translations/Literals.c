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

/*
 * Function to correct the offset to be applied for literal store/loading
 */
void Translate_Literals(code_seg_t* const codeSegment)
{
	Instruction_t*ins;
	ins = codeSegment->Intermcode;
	uint32_t x = 0;

	uint32_t InterimCodeLen = 0;
	uint32_t CountLiterals = 0;

	literal_t* l = codeSegment->literals;

#if defined(USE_INSTRUCTION_COMMENTS)
	currentTranslation = "Literals";
#endif

	while (l)
	{
		CountLiterals ++;
		l = l->next;
	}

	while (ins)
	{
		ins = ins->nextInstruction;
		InterimCodeLen++;
	}

	ins = codeSegment->Intermcode;

	while (ins)
	{
		switch (ins->instruction)
		{
		case ARM_STR_LIT:
		case ARM_LDR_LIT:
			if ((ins->R2.regID&~REG_HOST) != 0xf)
			{
				ins->U = 0;
			}
			else if (codeSegment->Type == SEG_START)
			{
				ins->offset = ins->offset - x -CountLiterals*4 - 8;

				if (ins->offset < 0)
				{
					ins->U = 0;
					ins->offset = -ins->offset;
				}

			}
			else if (codeSegment->Type == SEG_END
					&& ((ins->R2.regID&~REG_HOST) == 0xf))
			{
				ins->offset = InterimCodeLen * 4 - x + ins->offset - 8;
			}

			break;
		default:
			break;
		}
		ins = ins->nextInstruction;
		x+=4;
	}
}
