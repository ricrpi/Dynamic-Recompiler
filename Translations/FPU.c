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


void Translate_FPU(code_seg_t* const codeSegment)
{
	Instruction_t*ins;
	//Instruction_t*new_ins;
	ins = codeSegment->Intermcode;
#if USE_INSTRUCTION_COMMENTS
	currentTranslation = "FPU";
#endif
	while (ins)
	{
		switch (ins->instruction)
		{
			case MIPS_MFC1:
			case MIPS_DMFC1:
			case MIPS_CFC1:
			case MIPS_MTC1:
			case MIPS_DMTC1:
			case MIPS_CTC1:
			case MIPS_BC1F:
			case MIPS_BC1T:
			case MIPS_BC1FL:
			case MIPS_BC1TL:
			case MIPS_ADD_S:
			case MIPS_SUB_S:
			case MIPS_MUL_S:
			case MIPS_DIV_S:
			case MIPS_SQRT_S:
			case MIPS_ABS_S:
			case MIPS_MOV_S:
			case MIPS_NEG_S:
			case MIPS_ROUND_L_S:
			case MIPS_TRUNC_L_S:
			case MIPS_CEIL_L_S:
			case MIPS_FLOOR_L_S:
			case MIPS_ROUND_W_S:
			case MIPS_TRUNC_W_S:
			case MIPS_CEIL_W_S:
			case MIPS_FLOOR_W_S:
			case MIPS_CVT_D_S:
			case MIPS_CVT_W_S:
			case MIPS_CVT_L_S:
			case MIPS_C_F_S:
			case MIPS_C_UN_S:
			case MIPS_C_EQ_S:
			case MIPS_C_UEQ_S:
			case MIPS_C_OLT_S:
			case MIPS_C_ULT_S:
			case MIPS_C_OLE_S:
			case MIPS_C_ULE_S:
			case MIPS_C_SF_S:
			case MIPS_C_NGLE_S:
			case MIPS_C_SEQ_S:
			case MIPS_C_NGL_S:
			case MIPS_C_LT_S:
			case MIPS_C_NGE_S:
			case MIPS_C_LE_S:
			case MIPS_C_NGT_S:
			case MIPS_ADD_D:
			case MIPS_SUB_D:
			case MIPS_MUL_D:
			case MIPS_DIV_D:
			case MIPS_SQRT_D:
			case MIPS_ABS_D:
			case MIPS_MOV_D:
			case MIPS_NEG_D:
			case MIPS_ROUND_L_D:
			case MIPS_TRUNC_L_D:
			case MIPS_CEIL_L_D:
			case MIPS_FLOOR_L_D:
			case MIPS_ROUND_W_D:
			case MIPS_TRUNC_W_D:
			case MIPS_CEIL_W_D:
			case MIPS_FLOOR_W_D:
			case MIPS_CVT_S_D:
			case MIPS_CVT_W_D:
			case MIPS_CVT_L_D:
			case MIPS_C_F_D:
			case MIPS_C_UN_D:
			case MIPS_C_EQ_D:
			case MIPS_C_UEQ_D:
			case MIPS_C_OLT_D:
			case MIPS_C_ULT_D:
			case MIPS_C_OLE_D:
			case MIPS_C_ULE_D:
			case MIPS_C_SF_D:
			case MIPS_C_NGLE_D:
			case MIPS_C_SEQ_D:
			case MIPS_C_NGL_D:
			case MIPS_C_LT_D:
			case MIPS_C_NGE_D:
			case MIPS_C_LE_D:
			case MIPS_C_NGT_D:
			case MIPS_CVT_S_W:
			case MIPS_CVT_D_W:
			case MIPS_CVT_S_L:
			case MIPS_CVT_D_L:
				TRANSLATE_ABORT();
		default: break;
		}

		ins = ins->nextInstruction;
	}
}
