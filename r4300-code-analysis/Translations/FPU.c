/*
 * FPU.c
 *
 *  Created on: 23 Oct 2014
 *      Author: rjhender
 */

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
			case MFC0: break;
			case MTC0: break;
			case MFC1: break;
			case DMFC1: break;
			case CFC1: break;
			case MTC1: break;
			case DMTC1: break;
			case CTC1: break;
			case BC1F: break;
			case BC1T: break;
			case BC1FL: break;
			case BC1TL: break;
			case ADD_S: break;
			case SUB_S: break;
			case MUL_S: break;
			case DIV_S: break;
			case SQRT_S: break;
			case ABS_S: break;
			case MOV_S: break;
			case NEG_S: break;
			case ROUND_L_S: break;
			case TRUNC_L_S: break;
			case CEIL_L_S: break;
			case FLOOR_L_S: break;
			case ROUND_W_S: break;
			case TRUNC_W_S: break;
			case CEIL_W_S: break;
			case FLOOR_W_S: break;
			case CVT_D_S: break;
			case CVT_W_S: break;
			case CVT_L_S: break;
			case C_F_S: break;
			case C_UN_S: break;
			case C_EQ_S: break;
			case C_UEQ_S: break;
			case C_OLT_S: break;
			case C_ULT_S: break;
			case C_OLE_S: break;
			case C_ULE_S: break;
			case C_SF_S: break;
			case C_NGLE_S: break;
			case C_SEQ_S: break;
			case C_NGL_S: break;
			case C_LT_S: break;
			case C_NGE_S: break;
			case C_LE_S: break;
			case C_NGT_S: break;
			case ADD_D: break;
			case SUB_D: break;
			case MUL_D: break;
			case DIV_D: break;
			case SQRT_D: break;
			case ABS_D: break;
			case MOV_D: break;
			case NEG_D: break;
			case ROUND_L_D: break;
			case TRUNC_L_D: break;
			case CEIL_L_D: break;
			case FLOOR_L_D: break;
			case ROUND_W_D: break;
			case TRUNC_W_D: break;
			case CEIL_W_D: break;
			case FLOOR_W_D: break;
			case CVT_S_D: break;
			case CVT_W_D: break;
			case CVT_L_D: break;
			case C_F_D: break;
			case C_UN_D: break;
			case C_EQ_D: break;
			case C_UEQ_D: break;
			case C_OLT_D: break;
			case C_ULT_D: break;
			case C_OLE_D: break;
			case C_ULE_D: break;
			case C_SF_D: break;
			case C_NGLE_D: break;
			case C_SEQ_D: break;
			case C_NGL_D: break;
			case C_LT_D: break;
			case C_NGE_D: break;
			case C_LE_D: break;
			case C_NGT_D: break;
			case CVT_S_W: break;
			case CVT_D_W: break;
			case CVT_S_L: break;
			case CVT_D_L: break;
		default: break;
		}

		ins = ins->nextInstruction;
	}
}
