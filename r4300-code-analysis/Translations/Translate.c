/*
 * Translate.c
 *
 *  Created on: 16 Apr 2014
 *      Author: rjhender
 */

#include "InstructionSet.h"
#include "InstructionSet_ascii.h"
#include "InstructionSetMIPS4.h"
#include "InstructionSetARM6hf.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include "memory.h"
#include "Translate.h"

#include "Debugger.h"	// RWD

uint32_t uiCountFrequency 	= 40;	// must be less than 128 else may not be able to encode in imm8
uint32_t bMemoryOffsetCheck = 0;
uint32_t bDoDMAonInterrupt 	= 1;

char* currentTranslation = NULL;

//=============================================================

Instruction_t* insertP_R_A(code_seg_t* const code_seg, Instruction_t* ins, const Condition_e cond)
{
	Instruction_t* new_ins;
	regID_t base;
	int32_t offset;

#if defined(USE_INSTRUCTION_COMMENTS)
	char* oldCurrentTranslation = currentTranslation;
	currentTranslation = "insertP_R_A()";
#endif

	addLiteral(code_seg, &base, &offset, (uint32_t)&p_r_a);

	// we have to save r0 to r3 but as EAPI uses sp to point to arg 5 onwards we can't save all the registers in one go
	new_ins 	= newInstrPUSH(AL, REG_HOST_STM_EABI);
	ADD_LL_NEXT(new_ins, ins);

	new_ins 	= newInstrPUSH(AL, REG_HOST_STM_ALL & ~REG_HOST_STM_EABI);
	ADD_LL_NEXT(new_ins, ins);

	new_ins 		= newInstrI(ARM_LDR_LIT, AL, REG_HOST_R0, REG_NOT_USED, base, offset);
	ADD_LL_NEXT(new_ins, ins);

#if 1
	// load function address from [fp + offset] into PC
	new_ins 	= newInstr(ARM_BX, cond, REG_NOT_USED, REG_HOST_R0, REG_NOT_USED);
	new_ins->Ln = 1;
	ADD_LL_NEXT(new_ins, ins);
#else
	//set lr
	new_ins 	= newInstr(ARM_MOV, AL, REG_HOST_LR, REG_NOT_USED, REG_HOST_PC);
	ADD_LL_NEXT(new_ins, ins);

	// load function address from [fp + offset] into PC
	new_ins 	= newInstr(ARM_MOV, cond, REG_HOST_PC, REG_NOT_USED, REG_HOST_R0);
	ADD_LL_NEXT(new_ins, ins);
#endif

	// pop lr
	new_ins 	= newInstrPOP(AL, REG_HOST_STM_ALL & ~REG_HOST_STM_EABI);
	ADD_LL_NEXT(new_ins, ins);

	new_ins 	= newInstrPOP(AL, REG_HOST_STM_EABI);
	ADD_LL_NEXT(new_ins, ins);

#if defined(USE_INSTRUCTION_COMMENTS)
	currentTranslation = oldCurrentTranslation;
#endif

	return ins;
}


Instruction_t* insertCall_To_C(code_seg_t* const code_seg, Instruction_t* ins, const Condition_e cond, uint32_t functionAddress, uint32_t Rmask)
{
	Instruction_t* newInstruction;
	regID_t base;
	int32_t offset;

	addLiteral(code_seg, &base, &offset, functionAddress);

	newInstruction 		= newInstrI(ARM_LDR_LIT, AL, REG_TEMP_CALL2C, REG_NOT_USED, base, offset);
	ADD_LL_NEXT(newInstruction, ins);

	//push lr
	newInstruction 	= newInstrPUSH(AL, Rmask | REG_HOST_STM_LR);
	ADD_LL_NEXT(newInstruction, ins);

	newInstruction 	= newInstr(ARM_BX, cond, REG_NOT_USED, REG_TEMP_CALL2C, REG_NOT_USED);
	newInstruction->Ln = 1;
	ADD_LL_NEXT(newInstruction, ins);

	// pop lr
	newInstruction 	= newInstrPOP(AL, Rmask | REG_HOST_STM_LR);
	ADD_LL_NEXT(newInstruction, ins);

	return ins;
}

Instruction_t* insertCall_To_C_Jump(code_seg_t* const code_seg, Instruction_t* ins, const Condition_e cond, uint32_t functionAddress, uint32_t Rmask, Instruction_t* ReturnIns)
{
	Instruction_t* newInstruction;
	regID_t base;
	int32_t offset;

	addLiteral(code_seg, &base, &offset, functionAddress);

	newInstruction 		= newInstrI(ARM_LDR_LIT, AL, REG_TEMP_CALL2C, REG_NOT_USED, base, offset);
	ADD_LL_NEXT(newInstruction, ins);

	//push lr
	newInstruction 	= newInstrPUSH(AL, Rmask | REG_HOST_STM_LR);
	ADD_LL_NEXT(newInstruction, ins);

	//set lr
	newInstruction 	= newInstr(ARM_MOV, AL, REG_HOST_LR, REG_NOT_USED, REG_NOT_USED);
	newInstruction->branchToThisInstruction = ReturnIns;
	ADD_LL_NEXT(newInstruction, ins);

	// load function address from [fp + offset] into PC
	newInstruction 	= newInstr(ARM_MOV, cond, REG_HOST_PC, REG_NOT_USED, REG_TEMP_CALL2C);
	ADD_LL_NEXT(newInstruction, ins);

	// pop lr
	newInstruction 	= newInstrPOP(AL, Rmask | REG_HOST_STM_LR);
	ADD_LL_NEXT(newInstruction, ins);

	return ins;
}

//=============================================================

void Translate_init(code_seg_t* const codeSegment)
{
	int x;
	Instruction_t*newInstruction;
	Instruction_t*prevInstruction = NULL;

	freeIntermediateInstructions(codeSegment);

	//now build new Intermediate code
	for (x=0; x < codeSegment->MIPScodeLen; x++)
	{
		// Filter out No-ops
		//if (0 != *(codeSegment->MIPScode + x))
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

	}

	return;
}

void Translate_Generic(code_seg_t* const codeSegment)
{
	Instruction_t*ins;
	Instruction_t*new_ins;
	ins = codeSegment->Intermcode;

#if defined(USE_INSTRUCTION_COMMENTS)
	currentTranslation = "Generic";
#endif
	while (ins)
	{
		int32_t imm = ins->immediate;

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
			case DSLLV:	break;
			case DSRLV:	break;
			case DSRAV: break;
			case MULT: break;
			case MULTU: break;
			case DIV: break;
			case DIVU: break;
			case DMULT: break;
			case DMULTU: break;
			case DDIV: break;
			case DDIVU: break;
			case ADD: break;
			case ADDU: break;
			case SUB: break;
			case SUBU: break;
			case AND: break;
			case OR: break;
			case XOR: break;
			case NOR: break;
			case SLT:
				{
					regID_t dest = ins->Rd1.regID;
					ins = Instr(ins, ARM_CMP, AL, REG_NOT_USED, ins->R1.regID, ins->R2.regID);

					new_ins = newInstrI(ARM_MOV, GE, dest, REG_NOT_USED, REG_NOT_USED, 0);
					ADD_LL_NEXT(new_ins, ins);

					new_ins = newInstrI(ARM_MOV, LT, dest, REG_NOT_USED, REG_NOT_USED, 1);
					ADD_LL_NEXT(new_ins, ins);
				}
				break;
			case SLTU:
				{
					regID_t dest = ins->Rd1.regID;
					ins = Instr(ins, ARM_CMP, AL, REG_NOT_USED, ins->R1.regID, ins->R2.regID);

					new_ins = newInstrI(ARM_MOV, CS, dest, REG_NOT_USED, REG_NOT_USED, 0);
					ADD_LL_NEXT(new_ins, ins);

					new_ins = newInstrI(ARM_MOV, CC, dest, REG_NOT_USED, REG_NOT_USED, 1);
					ADD_LL_NEXT(new_ins, ins);
				}
			break;
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
			case SLTI:
				if (ins->immediate > 255)
				{

				}
				else if (ins->immediate > 0)
				{
					regID_t dest = ins->Rd1.regID;

					ins = InstrI(ins, ARM_CMP, AL, REG_NOT_USED, ins->R1.regID, REG_NOT_USED, ins->immediate&0xFF);

					new_ins = newInstr(ARM_MOV, LT, dest, REG_NOT_USED, ins->R1.regID);
					ADD_LL_NEXT(new_ins, ins);
				}
				else if (ins->immediate > -0xff)
				{
					regID_t dest = ins->Rd1.regID;

					ins = InstrI(ins, ARM_CMN, AL, REG_NOT_USED, ins->R1.regID, REG_NOT_USED, -(ins->immediate&0xFF));

					new_ins = newInstr(ARM_MOV, LT, dest, REG_NOT_USED, ins->R1.regID);
					ADD_LL_NEXT(new_ins, ins);
				}
				else
				{

				}
				break;
			case SLTIU:	// TODO don't think this is right
				if (ins->immediate > 255)
				{

				}
				else if (ins->immediate > 0)
				{
					regID_t dest = ins->Rd1.regID;

					ins = InstrI(ins, ARM_CMP, AL, REG_NOT_USED, ins->R1.regID, REG_NOT_USED, ins->immediate&0xFF);

					new_ins = newInstr(ARM_MOV, CC, dest, REG_NOT_USED, ins->R1.regID);
					ADD_LL_NEXT(new_ins, ins);
				}
				else
				{

				}
				break;
			case ANDI: break;
			case ORI: break;
			case XORI: break;
			case LUI: break;
			case MFC0: break;
			case MTC0:
				if (ins->R1.regID == 0)
				{
					ins->instruction = ARM_MOV;
					ins->I = 1;
					ins->R1.regID = REG_NOT_USED;
					ins->immediate = 0;
				}
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



void Translate_Write(code_seg_t* codeSegment)
{
	emit_arm_code(codeSegment);
}




void Translate(code_seg_t* const codeSegment)
{
	int x;

	for (x=0; x < COUNTOF(Translations); x++)
	{
		Translations[x].function(codeSegment);
	}
}
