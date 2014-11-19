/*
 * Trap.c
 *
 *  Created on: 23 Oct 2014
 *      Author: rjhender
 */

#include "Translate.h"

void Translate_Trap(code_seg_t* const codeSegment)
{
	Instruction_t*ins;
	//Instruction_t*new_ins;
	ins = codeSegment->Intermcode;

#if defined(USE_INSTRUCTION_COMMENTS)
	currentTranslation = "Trap";
#endif
	// TODO FPU Traps!

	while (ins)
	{
		switch (ins->instruction)
		{
			case SLL: break;
			case SRL: break;
			case SRA: break;
			case SLLV: break;
			case SRLV: break;
			case SRAV: break;
			case SYSCALL: break;
			case BREAK: break;
			case SYNC: break;
			case MFHI: break;
			case MTHI: break;
			case MFLO: break;
			case MTLO: break;
			case DSLLV:break;
			case DSRLV:break;
			case DSRAV: break;
			case MULT: break;
			case MULTU: break;
			case DIV: break;
			case DIVU: break;
			case DMULT: break;
			case DMULTU: break;
			case DDIV: break;
			case DDIVU: break;
			case ADD: break;	// TODO TRAP Overflow
			case ADDU: break;
			case SUB: break;	// TODO TRAP Overflow
			case SUBU: break;
			case AND: break;
			case OR: break;
			case XOR: break;
			case NOR: break;
			case SLT: break;
			case SLTU: break;
			case DADD: break;	// TODO TRAP Overflow
			case DADDU: break;
			case DSUB: break;	// TODO TRAP Overflow
			case DSUBU: break;
			case TGE: break;	// TODO TRAP Conditional
			case TGEU: break;	// TODO TRAP Conditional
			case TLT: break;	// TODO TRAP Conditional
			case TLTU: break;	// TODO TRAP Conditional
			case TEQ: break; 	// TODO TRAP Conditional
			case TNE: break;	// TODO TRAP Conditional
			case DSLL: break;
			case DSRL: break;
			case DSRA: break;
			case DSLL32: break;
			case DSRL32: break;
			case DSRA32: break;
			case TGEI: break;	// TODO TRAP Conditional
			case TGEIU: break;	// TODO TRAP Conditional
			case TLTI: break;	// TODO TRAP Conditional
			case TLTIU: break;	// TODO TRAP Conditional
			case TEQI: break;	// TODO TRAP Conditional
			case TNEI: break;	// TODO TRAP Conditional
			case ADDI: 			//TODO TRAP Overflow
				//ins->instruction = ARM_ADD;
				//ins->S = 1;
				//insertCall_To_C(codeSegment, ins, VS, MMAP_FP_BASE + FUNC_GEN_TRAP);
				break;
			case ADDIU:
			//	ins->instruction = ARM_ADD;
				break;
			case SLTI: break;
			case SLTIU: break;
			case ANDI: break;
			case ORI: break;
			case XORI: break;
			case LUI: break;
			case MFC0:
				ins->instruction = ARM_MOV;
				ins->R2 = ins->R1;
				ins->R1.regID = REG_NOT_USED;
				break;
			case MTC0:
				ins->instruction = ARM_MOV;
				ins->R2 = ins->R1;
				ins->R1.regID = REG_NOT_USED;
				break;
			case TLBR: break;
			case TLBWI: break;
			case TLBWR: break;
			case TLBP: break;
			case ERET: break;
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
			case DADDI: break;  // TODO TRAP Overflow
			case DADDIU: break;
			case CACHE: break;
			case LL: break;
			case LWC1: break;
			case LLD: break;
			case LDC1: break;
			case LD: break;
			case SC: break;
			case SWC1: break;
			case SCD: break;
			case SDC1: break;
			case SD: break;

			case J: break;
			case JR: break;
			case JAL: break;
			case JALR: break;

			case BLTZ: break;
			case BGEZ: break;
			case BEQ: break;
			case BNE: break;
			case BLEZ: break;
			case BGTZ: break;

			case BLTZL: break;
			case BGEZL: break;
			case BEQL: break;
			case BNEL: break;
			case BLEZL: break;
			case BGTZL: break;

			case BLTZAL: break;
			case BGEZAL: break;
			case BLTZALL: break;
			case BGEZALL: break;

			case SB: break;
			case SH: break;
			case SWL: break;
			case SW: break;
			case SDL: break;
			case SDR: break;
			case SWR: break;

			case LDL: break;
			case LDR: break;
			case LB: break;
			case LH: break;
			case LWL: break;
			case LW: break;
			case LBU: break;
			case LHU: break;
			case LWR: break;
			case LWU: break;

		default: break;
		}

		ins = ins->nextInstruction;
	}
}
