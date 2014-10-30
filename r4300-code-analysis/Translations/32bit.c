/*
 * 32bit.c
 *
 *  Created on: 23 Oct 2014
 *      Author: rjhender
 */

#include "Translate.h"

/*
 * Function to turn 64bit registers into multiple 32-bit registers
 *
 */
void Translate_32BitRegisters(code_seg_t* const codeSegment)
{
	Instruction_t*ins;
	Instruction_t*new_ins;
	regID_t Rd1, R1, R2;
	ins = codeSegment->Intermcode;

#if defined(USE_INSTRUCTION_COMMENTS)
	currentTranslation = "32BitRegisters";
#endif

	while (ins)
	{
		Rd1 = ins->Rd1.regID;
		R1 = ins->R1.regID;
		R2 = ins->R2.regID;
		switch (ins->instruction)
		{
			case DSLLV:
				/*
				 *		Rd1 W        Rd1            R1 W           R1               R2 W          R2
				 * [FF FF FF FF | FF FF FF FE] = [FF FF FF FF | FF FF FF FF] << [00 00 00 00 | 00 00 00 3F]
				 *
				 *
				 */

				// 1. Work out lower Word
				Instr(ins, ARM_MOV, AL, ins->Rd1.regID , REG_NOT_USED, ins->R1.regID);
				ins->shiftType = LOGICAL_LEFT;
				ins->R3.regID = R2;

				// 2. Work out upper word
				new_ins = newInstr(ARM_MOV, AL, ins->Rd1.regID | REG_WIDE, REG_NOT_USED, ins->R1.regID | REG_WIDE);
				new_ins->shiftType = LOGICAL_LEFT;
				new_ins->R3.regID = ins->R2.regID;
				ADD_LL_NEXT(new_ins, ins);

				// 3. Work out lower shifted to upper
				new_ins = newInstrIS(ARM_RSB, AL, REG_TEMP_GEN2, REG_NOT_USED, ins->R2.regID, 32);
				ADD_LL_NEXT(new_ins, ins);

				new_ins = newInstr(ARM_ORR, PL, ins->Rd1.regID | REG_WIDE, REG_NOT_USED, ins->R1.regID);
				new_ins->shiftType = LOGICAL_RIGHT;
				new_ins->R3.regID = REG_TEMP_GEN2;
				ADD_LL_NEXT(new_ins, ins);

				// 4. Work out R1 << into Rd1 W (i.e. where R2 > 32) If this occurs then Step 1 and 2 didn't do anything
				new_ins = newInstrIS(ARM_SUB, AL, REG_TEMP_GEN1, REG_NOT_USED, ins->R1.regID, 32);
				ADD_LL_NEXT(new_ins, ins);

				new_ins = newInstr(ARM_ORR, PL, ins->Rd1.regID | REG_WIDE, ins->R1.regID, REG_TEMP_GEN1);
				ADD_LL_NEXT(new_ins, ins);

				break;
			case DSRLV:
				/*
				 *
				 * [7F FF FF FF | FF FF FF FF] = [FF FF FF FF | FF FF FF FF] >> [00 00 00 00 | 00 00 00 3F]
				 *
				 *
				 */

				//Work out lower Word
				Instr(ins, ARM_MOV, AL, ins->Rd1.regID, REG_NOT_USED, ins->R1.regID);
				ins->shiftType = LOGICAL_RIGHT;
				ins->R3.regID = R2;

				//Work out upper word
				new_ins = newInstr(ARM_MOV, AL, ins->Rd1.regID| REG_WIDE, REG_NOT_USED, ins->Rd1.regID | REG_WIDE);
				new_ins->shiftType = LOGICAL_RIGHT;
				new_ins->R3.regID = ins->R2.regID;
				ADD_LL_NEXT(new_ins, ins);

				//Work out upper shifted to lower
				new_ins = newInstrIS(ARM_SUB, AL, REG_TEMP_GEN1, REG_NOT_USED, ins->R1.regID, 32);
				ADD_LL_NEXT(new_ins, ins);

				new_ins = newInstr(ARM_MOV, PL, REG_TEMP_GEN2, ins->R1.regID, REG_NOT_USED);
				new_ins->shiftType = LOGICAL_RIGHT;
				new_ins->R3.regID = REG_TEMP_GEN1;
				ADD_LL_NEXT(new_ins, ins);

				new_ins = newInstr(ARM_ORR, PL, ins->Rd1.regID| REG_WIDE, ins->Rd1.regID | REG_WIDE, REG_TEMP_GEN1);
				ADD_LL_NEXT(new_ins, ins);
				break;
			case DSRAV: break;
			case MULT: break;
			case MULTU: break;
			case DIV:					//TODO DIV uses signed!
				/*
				 *  clz  r3, r0                 r3 ← CLZ(r0) Count leading zeroes of N
					clz  r2, r1                 r2 ← CLZ(r1) Count leading zeroes of D
					sub  r3, r2, r3             r3 ← r2 - r3.
												 This is the difference of zeroes
												 between D and N.
												 Note that N >= D implies CLZ(N) <= CLZ(D)
					add r3, r3, #1              Loop below needs an extra iteration count

					mov r2, #0                  r2 ← 0
					b .Lloop_check4
					.Lloop4:
					  cmp r0, r1, lsl r3        Compute r0 - (r1 << r3) and update cpsr
					  adc r2, r2, r2            r2 ← r2 + r2 + C.
												  Note that if r0 >= (r1 << r3) then C=1, C=0 otherwise
					  subcs r0, r0, r1, lsl r3  r0 ← r0 - (r1 << r3) if C = 1 (this is, only if r0 >= (r1 << r3) )
					.Lloop_check4:
					  subs r3, r3, #1           r3 ← r3 - 1
					  bpl .Lloop4               if r3 >= 0 (N=0) then branch to .Lloop1

					mov r0, r2
				 */
				{
					// Count leading Zeros of N
					Instr(ins, ARM_CLZ, AL, REG_TEMP_GEN3, R1, REG_NOT_USED);

					// Count leading Zeros of D
					new_ins = newInstr(ARM_CLZ, AL, REG_TEMP_GEN2, R2, REG_NOT_USED);
					ADD_LL_NEXT(new_ins, ins);

					new_ins = newInstr(ARM_MOV, AL, REG_TEMP_GEN4, REG_NOT_USED, R1);
					ADD_LL_NEXT(new_ins, ins);

					new_ins = newInstr(ARM_SUB, AL, REG_TEMP_GEN3, REG_TEMP_GEN2, REG_TEMP_GEN3);
					ADD_LL_NEXT(new_ins, ins);

					new_ins = newInstrI(ARM_ADD, AL, REG_TEMP_GEN3, REG_TEMP_GEN3, REG_NOT_USED, 1);
					ADD_LL_NEXT(new_ins, ins);

					new_ins = newInstrI(ARM_MOV, AL, REG_TEMP_GEN2, REG_NOT_USED, REG_NOT_USED, 0);
					ADD_LL_NEXT(new_ins, ins);

					new_ins = newInstrB(AL, 4, 0);
					new_ins->I = 0;
					ADD_LL_NEXT(new_ins, ins);

					new_ins = newInstr(ARM_CMP, AL, REG_NOT_USED, R1, REG_TEMP_GEN4);
					new_ins->R3.regID = REG_TEMP_GEN3;
					new_ins->shiftType = LOGICAL_LEFT;
					ADD_LL_NEXT(new_ins, ins);

					new_ins = newInstrI(ARM_ADC, AL, REG_TEMP_GEN2, REG_TEMP_GEN2, REG_TEMP_GEN2, 0);
					ADD_LL_NEXT(new_ins, ins);

					new_ins = newInstr(ARM_SUB, CS, REG_TEMP_GEN4, REG_TEMP_GEN4, R2);
					new_ins->R3.regID = REG_TEMP_GEN3;
					new_ins->shiftType = LOGICAL_LEFT;
					ADD_LL_NEXT(new_ins, ins);

					new_ins = newInstrIS(ARM_SUB, AL, REG_TEMP_GEN2, REG_NOT_USED, REG_NOT_USED, 1);
					ADD_LL_NEXT(new_ins, ins);

					new_ins = newInstrB(PL, -4, 0);
					new_ins->I = 0;
					ADD_LL_NEXT(new_ins, ins);

					new_ins = newInstr(ARM_MOV, AL, Rd1, REG_NOT_USED, REG_TEMP_GEN4);
					ADD_LL_NEXT(new_ins, ins);
				}
				break;
			case DIVU: break;
			case DMULT: break;
			case DMULTU: break;
			case DDIV: break;
			case DDIVU: break;
			case ADD: break;
			case ADDU: break;
			case SUB: break;
			case SUBU: break;
			case AND:

				break;
			case OR: break;
			case XOR: break;
			case NOR: break;
			case SLT: break;
			case SLTU: break;
			case DADD: break;
			case DADDU: break;
			case DSUB: break;
			case DSUBU: break;
			case TGE: break;
			case TGEU: break;
			case TLT: break;
			case TLTU: break;
			case TEQ: break;
			case TNE: break;
			case DSLL: break;
			case DSRL: break;
			case DSRA: break;
			case DSLL32: break;
			case DSRL32: break;
			case DSRA32: break;
			case TGEI: break;
			case TGEIU: break;
			case TLTI: break;
			case TLTIU: break;
			case TEQI: break;
			case TNEI: break;
			case ADDI: break;
			case ADDIU: break;
			case SLTI: break;
			case SLTIU: break;
			case ANDI: break;
			case ORI: break;
			case XORI: break;
			case LUI: break;
			case MFC0: break;
			case MTC0: break;
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
			case DADDI: break;
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
