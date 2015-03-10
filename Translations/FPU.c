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

void Translate_FPU(code_seg_t* const codeSegment)
{
	Instruction_t*ins;
	//Instruction_t*new_ins;
	ins = codeSegment->Intermcode;
#if defined(USE_INSTRUCTION_COMMENTS)
	currentTranslation = "FPU";
#endif
	while (ins)
	{
		switch (ins->instruction)
		{
			case MFC1:
			case DMFC1:
			case CFC1:
			case MTC1:
			case DMTC1:
			case CTC1:
			case BC1F:
			case BC1T:
			case BC1FL:
			case BC1TL:
			case ADD_S:
			case SUB_S:
			case MUL_S:
			case DIV_S:
			case SQRT_S:
			case ABS_S:
			case MOV_S:
			case NEG_S:
			case ROUND_L_S:
			case TRUNC_L_S:
			case CEIL_L_S:
			case FLOOR_L_S:
			case ROUND_W_S:
			case TRUNC_W_S:
			case CEIL_W_S:
			case FLOOR_W_S:
			case CVT_D_S:
			case CVT_W_S:
			case CVT_L_S:
			case C_F_S:
			case C_UN_S:
			case C_EQ_S:
			case C_UEQ_S:
			case C_OLT_S:
			case C_ULT_S:
			case C_OLE_S:
			case C_ULE_S:
			case C_SF_S:
			case C_NGLE_S:
			case C_SEQ_S:
			case C_NGL_S:
			case C_LT_S:
			case C_NGE_S:
			case C_LE_S:
			case C_NGT_S:
			case ADD_D:
			case SUB_D:
			case MUL_D:
			case DIV_D:
			case SQRT_D:
			case ABS_D:
			case MOV_D:
			case NEG_D:
			case ROUND_L_D:
			case TRUNC_L_D:
			case CEIL_L_D:
			case FLOOR_L_D:
			case ROUND_W_D:
			case TRUNC_W_D:
			case CEIL_W_D:
			case FLOOR_W_D:
			case CVT_S_D:
			case CVT_W_D:
			case CVT_L_D:
			case C_F_D:
			case C_UN_D:
			case C_EQ_D:
			case C_UEQ_D:
			case C_OLT_D:
			case C_ULT_D:
			case C_OLE_D:
			case C_ULE_D:
			case C_SF_D:
			case C_NGLE_D:
			case C_SEQ_D:
			case C_NGL_D:
			case C_LT_D:
			case C_NGE_D:
			case C_LE_D:
			case C_NGT_D:
			case CVT_S_W:
			case CVT_D_W:
			case CVT_S_L:
			case CVT_D_L:
				TRANSLATE_ABORT();
		default: break;
		}

		ins = ins->nextInstruction;
	}
}
