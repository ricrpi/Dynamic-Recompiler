/*
 * Memory.c
 *
 *  Created on: 23 Oct 2014
 *      Author: rjhender
 */

#include "Translate.h"
#include "memory.h"

uint8_t uMemoryBase 		= 0x80;
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

	regID_t	funcTempReg;
	int32_t	funcTempImm;
	Instruction_t* new_ins;

	while (ins)
	{
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

		case SB: break;
		case SH: break;
		case SWL: break;
		case SW:
			//TODO optimize for constants
			funcTempReg = ins->R1.regID;
			funcTempImm = ins->immediate;

			//test if raw address or virtual
			ins = InstrI(ins, ARM_TST, AL, REG_NOT_USED, ins->R1.regID, REG_NOT_USED, 0x08 << 24);

			// if address is raw (NE) then add base offset to get to host address
			new_ins = newInstrI(ARM_ADD, NE, REG_TEMP_MEM1, ins->R1.regID, REG_NOT_USED, uMemoryBase << 24);
			ADD_LL_NEXT(new_ins, ins);

			//check immediate is not too large for ARM and if it is then add additional imm
			if (funcTempImm > 0xFFF || funcTempImm < -0xFFF)
			{
				new_ins = newInstrI(ARM_ORR, NE, REG_TEMP_MEM1, REG_TEMP_MEM1, REG_NOT_USED, funcTempImm&0xf000);
				ADD_LL_NEXT(new_ins, ins);
			}

			// now store the value at REG_TEMP_MEM1 ( This will be R2 + host base + funcTempImm&0xf000 )
			new_ins = newInstrI(ARM_STR_LIT, NE, REG_NOT_USED, funcTempReg, REG_TEMP_MEM1, funcTempImm&0xfff);
			// TODO do we need to set ins->U ?
			ADD_LL_NEXT(new_ins, ins);

			new_ins 		= newInstrPUSH(AL, REG_HOST_STM_EABI);
			ADD_LL_NEXT(new_ins, ins);

			new_ins = newInstr(ARM_MOV, AL, REG_HOST_R0, REG_NOT_USED, ins->R1.regID);
			ADD_LL_NEXT(new_ins, ins);

			// now lookup virtual address
			ins = insertCall_To_C(codeSegment, ins, EQ, (uint32_t)&mem_lookup);

			new_ins = newInstrI(ARM_STR_LIT, NE, REG_NOT_USED, funcTempReg, REG_HOST_R0, funcTempImm&0xfff);
			ADD_LL_NEXT(new_ins, ins);

			new_ins 		= newInstrPOP(AL, REG_HOST_STM_EABI);
			ADD_LL_NEXT(new_ins, ins);

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

			funcTempReg = ins->Rd1.regID;
			funcTempImm = ins->immediate;

			ins = InstrI(ins, ARM_TST, AL, REG_NOT_USED, ins->R1.regID, REG_NOT_USED, 0x08 << 24);

			new_ins = newInstrI(ARM_ADD, NE, REG_TEMP_MEM1, ins->R1.regID, REG_NOT_USED, uMemoryBase << 24);
			ADD_LL_NEXT(new_ins, ins);

			if (funcTempImm > 0xFFF || funcTempImm < -0xFFF)
			{
				new_ins = newInstrI(ARM_ORR, NE, REG_TEMP_MEM1, REG_TEMP_MEM1, REG_NOT_USED, funcTempImm&0xf000);
				ADD_LL_NEXT(new_ins, ins);
			}

			new_ins = newInstrI(ARM_LDR_LIT, NE, funcTempReg, REG_NOT_USED, REG_TEMP_MEM1, funcTempImm&0xfff);
			ADD_LL_NEXT(new_ins, ins);

			ins = insertCall_To_C(codeSegment, ins, EQ, (uint32_t)&mem_lookup);

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
