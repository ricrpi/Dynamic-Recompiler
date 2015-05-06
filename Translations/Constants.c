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
#include "DebugDefines.h"
#include "InstructionSetARM6hf.h"

void Translate_Constants(code_seg_t* const codeSegment)
{
	Instruction_t*ins = codeSegment->Intermcode;
#if defined(USE_INSTRUCTION_COMMENTS)
	currentTranslation = "Constants";
#endif
	//First off r0 is ALWAYS 0 so lets do that first
	while (ins)
	{
		if ((ins->Rd1.regID & ~REG_WIDE) == 0)
		{
			ins->Rd1.state = RS_CONSTANT_U8;
			ins->Rd1.u8 = 0;
		}

		if ((ins->Rd2.regID & ~REG_WIDE) == 0)
		{
			ins->Rd2.state = RS_CONSTANT_U8;
			ins->Rd2.u8 = 0;
		}

		if ((ins->R1.regID & ~REG_WIDE) == 0)
		{
			ins->R1.state = RS_CONSTANT_U8;
			ins->R1.u8 = 0;
		}

		if ((ins->R2.regID & ~REG_WIDE) == 0)
		{
			ins->R2.state = RS_CONSTANT_U8;
			ins->R2.u8 = 0;
		}

		if ((ins->R3.regID & ~REG_WIDE) == 0)
		{
			ins->R3.state = RS_CONSTANT_U8;
			ins->R3.u8 = 0;
		}

		ins = ins->nextInstruction;
	}

	ins = codeSegment->Intermcode;

	while (ins)
	{
		switch (ins->instruction)
		{

		case LUI:
		{
			regID_t base;
			int32_t offset;

			if (Imm8Shift(ins->immediate) == -1)
			{
				addLiteral(codeSegment, &base, &offset, ins->immediate << 16);

				InstrI(ins, ARM_LDR_LIT, AL, ins->Rd1.regID, REG_NOT_USED, base, offset);
			}
			else
			{
				Instruction_t* new_ins;
				InstrI(ins, ARM_MOV, AL, ins->Rd1.regID, REG_NOT_USED, REG_NOT_USED, ins->immediate);

				// sign extend
				if (ins->immediate&0x8000)
				{
					new_ins = newInstrI(ARM_MVN, AL, ins->Rd1.regID | REG_WIDE, REG_NOT_USED, REG_NOT_USED, 0);
				}
				else
				{
					new_ins = newInstrI(ARM_MOV, AL, ins->Rd1.regID | REG_WIDE, REG_NOT_USED, REG_NOT_USED, 0);
				}
				ADD_LL_NEXT(new_ins, ins);
			}

			ins->Rd1.state = RS_CONSTANT_I8;
			if (ins->immediate < 0)
			{
				ins->Rd1.i8 = 0xffffffff00000000ll | (ins->immediate << 16);
			}else
			{
			ins->Rd1.i8 = ins->immediate;
			}
		}break;
		default: break;
		}
		ins = ins->nextInstruction;
	}
}
