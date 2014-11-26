/*
 * Memory.c
 *
 *  Created on: 23 Oct 2014
 *      Author: rjhender
 */

#include "Translate.h"
#include "memory.h"

uint8_t uMemoryBase 		= 0x80;


static Instruction_t* insertCheckAddressRaw(Instruction_t* ins, regID_t R1)
{
	Instruction_t* new_ins;
	InstrI(ins, ARM_AND, AL, REG_TEMP_SCRATCH0, R1, REG_NOT_USED, 0xD0 << 24);	// ignore cache bit

	new_ins = newInstrI(ARM_CMP, AL, REG_NOT_USED, REG_TEMP_SCRATCH0,REG_NOT_USED, 0x80 << 24);		// is this address raw?
	ADD_LL_NEXT(new_ins, ins);
	return ins;
}

static Instruction_t* MoveToMemoryBase(Instruction_t* ins, Condition_e c,regID_t Rx)
{
	Instruction_t* new_ins;

	if (uMemoryBase < 0x80)
	{
		new_ins = newInstrI(ARM_SUB, c, Rx, Rx, REG_NOT_USED, (0x80 - uMemoryBase) << 24);
		ADD_LL_NEXT(new_ins, ins);
	} else if (uMemoryBase > 0x80)
	{
		new_ins = newInstrI(ARM_ADD, c, Rx, Rx, REG_NOT_USED, (uMemoryBase - 0x80) << 24);
		ADD_LL_NEXT(new_ins, ins);
	}
	return ins;
}

static Instruction_t* FixOffsetTooLarge(Instruction_t* ins, regID_t Rx)
{
	Instruction_t* new_ins;

	if (uMemoryBase < 0x80)
	{
		new_ins = newInstrI(ARM_SUB, NE, Rx, Rx, REG_NOT_USED, (0x80 - uMemoryBase) << 24);
		ADD_LL_NEXT(new_ins, ins);
	} else if (uMemoryBase > 0x80)
	{
		new_ins = newInstrI(ARM_ADD, NE, Rx, Rx, REG_NOT_USED, (uMemoryBase - 0x80) << 24);
		ADD_LL_NEXT(new_ins, ins);
	}
	return ins;
}

/*
 *	Function to Translate memory operations into Emulated equivalent
 *
 *  Emulated memory access can be cached, non-cached or virtual
 *
 *  As the emulator memory will be mmaped to 0x??000000, non-cached need not do
 *  any translation. cached memory 'just needs' BIC Rd, R1, #1, LSL # 29
 *
 *  Virtual will need to call a function to lookup address
 */
void Translate_Memory(code_seg_t* const codeSegment)
{
	Instruction_t*ins;
	ins = codeSegment->Intermcode;

	//TODO we need to check that memory being modified is not MIPS code!

#if defined(USE_INSTRUCTION_COMMENTS)
	currentTranslation = "Memory";
#endif

	regID_t Rd1;
	regID_t	R1;
	regID_t R2;
	int32_t	funcTempImm;
	Instruction_t* new_ins;
	Instruction_t* ins1, *ins2;

	while (ins)
	{
		Rd1 = ins->Rd1.regID;
		R1 = ins->R1.regID;
		R2 = ins->R2.regID;
		funcTempImm = ins->immediate;

		switch (ins->instruction)
		{
		case MFHI: break;
		case MTHI: break;
		case MFLO: break;
		case MTLO: break;

		case MULT: break;
		case MULTU: break;
		case DIV: break;
		case DIVU: break;
		case DMULT: break;
		case DMULTU: break;
		case DDIV: break;
		case DDIVU: break;

		case SLT: break;
		case SLTU: break;

		case LUI: break;
		case MFC0: break;
		case MTC0: break;

		case MFC1: break;
		case DMFC1: break;
		case CFC1: break;
		case MTC1: break;
		case DMTC1: break;
		case CTC1: break;

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

		case SB:
			ins = insertCheckAddressRaw(ins, R1);

			break;
		case SH: break;
		case SWL: break;
		case SW:
			ins = insertCheckAddressRaw(ins, R1);

			// used to force loading of registers
			ins1 = new_ins = newInstr(NO_OP, AL, REG_NOT_USED, R1, R2);
			ADD_LL_NEXT(new_ins, ins);

			ins1 = new_ins = newInstr(INT_BRANCH, NE, REG_NOT_USED, REG_NOT_USED, REG_NOT_USED);
			ADD_LL_NEXT(new_ins, ins);

			//Turn 0xA0 to 0x80
			new_ins = newInstrI(ARM_BIC, AL_B, REG_TEMP_SCRATCH0, R1, REG_NOT_USED, (0x20) << 24);
			ADD_LL_NEXT(new_ins, ins);

			ins = MoveToMemoryBase(ins, AL_B, REG_TEMP_SCRATCH0); // get to host address if uMemoryBase != 0x80

			if (funcTempImm > 0)
			{
				//check immediate is not too large for ARM and if it is then add additional imm
				if (funcTempImm > 0xFFF)
				{
					new_ins = newInstrI(ARM_ADD, AL_B, REG_TEMP_SCRATCH0, REG_TEMP_SCRATCH0, REG_NOT_USED, (funcTempImm)&0xf000);
					ADD_LL_NEXT(new_ins, ins);
				}
				// now store the value at REG_TEMP_GEN1 ( This will be R2 + host base + funcTempImm&0xf000 )
				new_ins = newInstrI(ARM_STR, AL_B, REG_NOT_USED, R2, REG_TEMP_SCRATCH0, funcTempImm&0xfff);
				new_ins->U = 1;
				ADD_LL_NEXT(new_ins, ins);
			}
			else
			{
				if  (funcTempImm < -0xFFF)
				{
					new_ins = newInstrI(ARM_SUB, AL_B, REG_TEMP_SCRATCH0, REG_TEMP_SCRATCH0, REG_NOT_USED, (-funcTempImm)&0xf000);
					ADD_LL_NEXT(new_ins, ins);
				}

				// now store the value at REG_TEMP_GEN1 ( This will be R2 + host base + funcTempImm&0xf000 )
				new_ins = newInstrI(ARM_STR, AL_B, REG_NOT_USED, R2, REG_TEMP_SCRATCH0, (-funcTempImm)&0xfff);
				new_ins->U = 0;
				ADD_LL_NEXT(new_ins, ins);
			}

			ins2 = new_ins = newInstr(INT_BRANCH, AL_B, REG_NOT_USED, REG_NOT_USED, REG_NOT_USED);
			ADD_LL_NEXT(new_ins, ins);

			//new_ins 	= newInstrPUSH(AL, REG_HOST_STM_EABI);
			//ADD_LL_NEXT(new_ins, ins);

			//If this is equal then we have a virtual address
			//TODO not included imm
			new_ins = newInstr(ARM_MOV, AL_B, REG_HOST_R0, REG_NOT_USED, R1);
			ADD_LL_NEXT(new_ins, ins);

			ins1->branchToThisInstruction = new_ins;

			new_ins = newInstr(ARM_MOV, AL_B, REG_HOST_R1, REG_NOT_USED, R2);
			ADD_LL_NEXT(new_ins, ins);

			ins = insertCall_To_C(codeSegment,ins, AL_B, (size_t)&virtual_address, 0);

			new_ins = newInstr(ARM_STR, AL_B, REG_NOT_USED, R2, REG_HOST_R0);
			ADD_LL_NEXT(new_ins, ins);

			//new_ins 	= newInstrPOP(AL, REG_HOST_STM_EABI);
			//ADD_LL_NEXT(new_ins, ins);

			ins2->branchToThisInstruction = ins->nextInstruction;

			//===============================================================

			//ins = insertP_R_A(codeSegment, ins, AL);

			// TODO we need to check memory changed is not in code space
			break;
		case SDL: break;
		case SDR: break;
		case SWR: break;

		case LDL: break;
		case LDR: break;
		case LB: break;
		case LH: break;
		case LWL: break;
		case LW:
			ins = insertCheckAddressRaw(ins, R1);

			// used to force loading of registers
			ins1 = new_ins = newInstr(NO_OP, AL, REG_NOT_USED, R1, REG_NOT_USED);
			ADD_LL_NEXT(new_ins, ins);

			ins1 = new_ins = newInstr(INT_BRANCH, NE, REG_NOT_USED, REG_NOT_USED, REG_NOT_USED);
			ADD_LL_NEXT(new_ins, ins);

			//Turn 0xA0 to 0x80
			new_ins = newInstrI(ARM_BIC, AL_B, REG_TEMP_SCRATCH0, R1, REG_NOT_USED, (0x20) << 24);
			ADD_LL_NEXT(new_ins, ins);

			ins = MoveToMemoryBase(ins, AL_B, REG_TEMP_SCRATCH0); // get to host address if uMemoryBase != 0x80

			if (funcTempImm > 0)
			{
				//check immediate is not too large for ARM and if it is then add additional imm
				if (funcTempImm > 0xFFF)
				{
					new_ins = newInstrI(ARM_ADD, AL_B, REG_TEMP_SCRATCH0, REG_TEMP_SCRATCH0, REG_NOT_USED, (funcTempImm)&0xf000);
					ADD_LL_NEXT(new_ins, ins);
				}
				// now store the value at REG_TEMP_SCRATCH0 ( This will be R2 + host base + funcTempImm&0xf000 )
				new_ins = newInstrI(ARM_LDR, AL_B, Rd1, REG_NOT_USED, REG_TEMP_SCRATCH0, funcTempImm&0xfff);
				new_ins->U = 1;
				ADD_LL_NEXT(new_ins, ins);
			}
			else
			{
				if  (funcTempImm < -0xFFF)
				{
					new_ins = newInstrI(ARM_SUB, AL_B, REG_TEMP_SCRATCH0, REG_TEMP_SCRATCH0, REG_NOT_USED, (-funcTempImm)&0xf000);
					ADD_LL_NEXT(new_ins, ins);
				}

				// now store the value at REG_TEMP_SCRATCH0 ( This will be R2 + host base + funcTempImm&0xf000 )
				new_ins = newInstrI(ARM_LDR, AL_B, Rd1, REG_NOT_USED, REG_TEMP_SCRATCH0, (-funcTempImm)&0xfff);
				new_ins->U = 0;
				ADD_LL_NEXT(new_ins, ins);
			}

			ins2 = new_ins = newInstr(INT_BRANCH, AL_B, REG_NOT_USED, REG_NOT_USED, REG_NOT_USED);
			ADD_LL_NEXT(new_ins, ins);

			//new_ins 	= newInstrPUSH(AL, REG_HOST_STM_EABI);
			//ADD_LL_NEXT(new_ins, ins);

			//If this is equal then we have a virtual address
			//TODO not included imm
			new_ins = newInstr(ARM_MOV, AL, REG_HOST_R0, REG_NOT_USED, R1);
			ADD_LL_NEXT(new_ins, ins);

			ins1->branchToThisInstruction = new_ins;

			ins = insertCall_To_C(codeSegment,ins, AL_B, (size_t)&virtual_address, 0);

			//Clear Status
			new_ins = newInstr(ARM_LDR, AL_B, Rd1, REG_NOT_USED, REG_HOST_R0);
			ADD_LL_NEXT(new_ins, ins);

			// Force delay of StoreCachedRegisters
			new_ins = newInstr(NO_OP, AL, Rd1, REG_NOT_USED, REG_NOT_USED);
			ADD_LL_NEXT(new_ins, ins);

			ins2->branchToThisInstruction = new_ins;

			break;
		case LBU: break;
		case LHU: break;
		case LWR: break;
		case LWU: break;
		default: break;
		}

		ins = ins->nextInstruction;
	}
}
