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

#include "memory.h"

uint8_t uMemoryBase 		= 0x80U;


static Instruction_t* insertCheckAddressRaw(Instruction_t* ins, regID_t R1)
{
	// 0x80 = 1000 0000
	// 0xA0 = 1010 0000
	// 0xD0 = 1101 0000

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

static void storeWord(uintptr_t b, uint32_t v)
{
	uint32_t* addr = (uint32_t*)(b);

	if ((((uint32_t)addr)&0xD0000000) == 0x80000000)
	{
		addr = (uint32_t*)((uint32_t)addr&0xDFFFFFFF);
		*addr = v;
	}
	else
	{
		printf("storeWord() virtual address 0x%x\n", (uint32_t)addr);
		abort();
	}
}

static uint32_t loadWord(uintptr_t b)
{
	uint32_t* addr = (uint32_t*)(b);

	if ((((uint32_t)addr)&0xD0000000) == 0x80000000)
	{
		addr = (uint32_t*)((uint32_t)addr&0xDFFFFFFF);
		return *addr;
	}
	else
	{
		printf("loadWord() virtual address 0x%x\n", (uint32_t)addr);
		abort();
	}
	return NULL;
}

static uint64_t C_ldl(uintptr_t b, uint32_t r1, uint32_t r2)
{
	uint32_t* addr = (uint32_t*)(b);
	uint64_t v;
	uint64_t old_v = (uint64_t)r1 << 32U | r2;
	uint64_t mask;

	mask = ((uint64_t)(-1LL)) >> (1 + ((uint32_t)addr&7));

	if ((((uintptr_t)addr)&0xD0000000U) == 0x80000000U)
	{
		addr = (uint32_t*)(((uintptr_t)addr) & 0xDFFFFFFFU);

		// swap words
		v = ((uint64_t)addr[1] << 32U) | addr[0];

		return (v << (7U - (((uintptr_t)addr) & 7U))) | (old_v & mask);
	}
	else
	{
		printf("ldl() virtual address 0x%x\n", (uintptr_t)addr);
		abort();
	}

	return 0;
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

#if USE_INSTRUCTION_COMMENTS
	currentTranslation = "Memory";
#endif

	regID_t Rd1;
	regID_t	R1;
	regID_t R2;
	int32_t	imm;
	Instruction_t* new_ins;
	Instruction_t* ins1, *ins2;

	while (ins)
	{
		Rd1 = ins->Rd1.regID;
		R1 = ins->R1.regID;
		R2 = ins->R2.regID;
		imm = ins->immediate;

		switch (ins->instruction)
		{
		case MIPS_LL:
		case MIPS_LWC1:
		case MIPS_LLD:
		case MIPS_LDC1:
		case MIPS_LD:
			if (imm < -0xFFF)
			{
				InstrI(ins, ARM_SUB, AL, REG_HOST_R0, R1, REG_NOT_USED, (-imm) & 0xFF00U);

				new_ins = newInstrI(ARM_LDR, AL, Rd1, REG_NOT_USED, REG_HOST_R0, ((-imm) & 0x0FFU) + 4U);
				new_ins->U = 0U;
				ADD_LL_NEXT(new_ins, ins);

				new_ins = newInstrI(ARM_LDR, AL, Rd1 | REG_WIDE, REG_NOT_USED, REG_HOST_R0, -(imm & 0xFFU));
				ADD_LL_NEXT(new_ins, ins);
			}
			else if (imm > 0xFFF + 4)
			{
				InstrI(ins, ARM_ADD, AL, REG_HOST_R0, R1, REG_NOT_USED, imm & 0xFF00U);

				new_ins = newInstrI(ARM_LDR, AL, Rd1, REG_NOT_USED, REG_HOST_R0, (imm & 0x0FFU) + 4U);
				ADD_LL_NEXT(new_ins, ins);

				new_ins = newInstrI(ARM_LDR, AL, Rd1 | REG_WIDE, REG_NOT_USED, REG_HOST_R0, (imm & 0xFFU));
				ADD_LL_NEXT(new_ins, ins);
			}
			else
			{
				InstrI(ins, ARM_LDR, AL, Rd1, REG_NOT_USED, R1, imm + 4U);

				new_ins = newInstrI(ARM_LDR, AL, Rd1 | REG_WIDE, REG_NOT_USED, R1, imm);
				ADD_LL_NEXT(new_ins, ins);
			}

			break;
		case MIPS_SC:
		case MIPS_SWC1:
		case MIPS_SCD:
		case MIPS_SDC1:
		case MIPS_SD:
		case MIPS_SB:
			//ins = insertCheckAddressRaw(ins, R1);
		case MIPS_SH:
		case MIPS_SWL:
			TRANSLATE_ABORT();
			break;
		case MIPS_SW:
#if 1
			Instr(ins, ARM_MOV, AL,REG_TEMP_SCRATCH0, REG_NOT_USED, R1);

			if (imm < 0)
			{
				new_ins = newInstrI(ARM_SUB, AL, REG_TEMP_SCRATCH0, REG_TEMP_SCRATCH0, REG_NOT_USED, -imm);
				ADD_LL_NEXT(new_ins, ins);
			}
			else if (imm > 0)
			{
				new_ins = newInstrI(ARM_ADD, AL, REG_TEMP_SCRATCH0, REG_TEMP_SCRATCH0, REG_NOT_USED, imm);
				ADD_LL_NEXT(new_ins, ins);
			}

			new_ins = newInstr(ARM_MOV, AL, REG_TEMP_SCRATCH1, REG_NOT_USED, R2);
			ADD_LL_NEXT(new_ins, ins);

			insertCall_To_C(codeSegment, ins, AL, &storeWord, 0);
#else
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

			if (imm > 0)
			{
				//check immediate is not too large for ARM and if it is then add additional imm
				if (imm > 0xFFF)
				{
					new_ins = newInstrI(ARM_ADD, AL_B, REG_TEMP_SCRATCH0, REG_TEMP_SCRATCH0, REG_NOT_USED, (imm)&0xf000);
					ADD_LL_NEXT(new_ins, ins);
				}
				// now store the value at REG_TEMP_GEN1 ( This will be R2 + host base + funcTempImm&0xf000 )
				new_ins = newInstrI(ARM_STR, AL_B, REG_NOT_USED, R2, REG_TEMP_SCRATCH0, imm&0xfff);
				new_ins->U = 1;
				ADD_LL_NEXT(new_ins, ins);
			}
			else
			{
				if  (imm < -0xFFF)
				{
					new_ins = newInstrI(ARM_SUB, AL_B, REG_TEMP_SCRATCH0, REG_TEMP_SCRATCH0, REG_NOT_USED, (-imm)&0xf000);
					ADD_LL_NEXT(new_ins, ins);
				}

				// now store the value at REG_TEMP_GEN1 ( This will be R2 + host base + funcTempImm&0xf000 )
				new_ins = newInstrI(ARM_STR, AL_B, REG_NOT_USED, R2, REG_TEMP_SCRATCH0, (-imm)&0xfff);
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
#endif
			break;
		case MIPS_SDL:
		case MIPS_SDR:
		case MIPS_SWR:
			TRANSLATE_ABORT();
			break;
		case MIPS_LDL:
		{
			if (imm < 0)
			{
				InstrI(ins, ARM_SUB, AL, REG_HOST_R0, R1, REG_NOT_USED, (-imm)&0xFF);

				if (imm < -0xFF)
				{
					new_ins = newInstrI(ARM_SUB, AL, REG_HOST_R0, REG_NOT_USED, REG_HOST_R0, (-imm)&0xFF00);
					ADD_LL_NEXT(new_ins, ins);
				}
			}
			else
			{
				InstrI(ins, ARM_ADD, AL, REG_HOST_R0, R1, REG_NOT_USED, imm&0xFF);

				if (imm > 0xFF)
				{
					new_ins = newInstrI(ARM_ADD, AL, REG_HOST_R0, REG_NOT_USED, REG_HOST_R0, (imm)&0xFF00);
					ADD_LL_NEXT(new_ins, ins);
				}
			}

			new_ins = newInstr(ARM_ADD, AL, REG_HOST_R2, REG_NOT_USED, Rd1 | REG_WIDE);
			ADD_LL_NEXT(new_ins, ins);

			new_ins = newInstr(ARM_ADD, AL, REG_HOST_R3, REG_NOT_USED, Rd1);
			ADD_LL_NEXT(new_ins, ins);

			ins = insertCall_To_C(codeSegment, ins, AL, (uint32_t)&C_ldl, 0);

			new_ins = newInstr(ARM_MOV, AL, Rd1, REG_NOT_USED, REG_HOST_R0);
			ADD_LL_NEXT(new_ins, ins);

			new_ins = newInstr(ARM_MOV, AL, Rd1 | REG_WIDE, REG_NOT_USED, REG_HOST_R1);
			ADD_LL_NEXT(new_ins, ins);
		}
		break;

		case MIPS_LDR:
			TRANSLATE_ABORT();
			break;
		case MIPS_LB:
			if (imm < -0xFF)
			{
				InstrI(ins, ARM_SUB, AL, REG_HOST_R0, R1, REG_NOT_USED, (-imm) & 0xFF00U);

				new_ins = newInstrI(ARM_LDRSB, AL, Rd1, REG_NOT_USED, REG_HOST_R0, -(imm & 0xFFU));
				ADD_LL_NEXT(new_ins, ins);
			}
			else if (imm > 0xFF)
			{
				InstrI(ins, ARM_ADD, AL, REG_HOST_R0, R1, REG_NOT_USED, imm & 0xFF00U);

				new_ins = newInstrI(ARM_LDRSB, AL, Rd1, REG_NOT_USED, REG_HOST_R0, (imm & 0xFFU));
				ADD_LL_NEXT(new_ins, ins);
			}
			else
			{
				InstrI(ins, ARM_LDRSB, AL, Rd1, REG_NOT_USED, R1, imm);
			}

			// Fix upper 64 bits
			new_ins = newInstrI(ARM_CMP, AL, REG_NOT_USED, Rd1, REG_NOT_USED, 0);
			ADD_LL_NEXT(new_ins, ins);

			new_ins = newInstrI(ARM_MVN, MI, Rd1 | REG_WIDE, REG_NOT_USED, REG_NOT_USED, 0);
			ADD_LL_NEXT(new_ins, ins);

			new_ins = newInstrI(ARM_MOV, PL, Rd1 | REG_WIDE, REG_NOT_USED, REG_NOT_USED, 0);
			ADD_LL_NEXT(new_ins, ins);

			break;
		case MIPS_LH:
		case MIPS_LWL:
			TRANSLATE_ABORT();
			break;
		case MIPS_LW:
#if 1
			Instr(ins, ARM_MOV, AL,REG_TEMP_SCRATCH0, REG_NOT_USED, R1);

			if (imm < 0)
			{
				new_ins = newInstrI(ARM_SUB, AL, REG_TEMP_SCRATCH0, REG_TEMP_SCRATCH0, REG_NOT_USED, -imm);
				ADD_LL_NEXT(new_ins, ins);
			}
			else if (imm > 0)
			{
				new_ins = newInstrI(ARM_ADD, AL, REG_TEMP_SCRATCH0, REG_TEMP_SCRATCH0, REG_NOT_USED, imm);
				ADD_LL_NEXT(new_ins, ins);
			}

			ins = insertCall_To_C(codeSegment, ins, AL, (uint32_t)&loadWord, 0);

			new_ins = newInstr(ARM_MOV, AL, Rd1, REG_NOT_USED, REG_TEMP_SCRATCH0);
			ADD_LL_NEXT(new_ins, ins);

#else
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

			if (imm > 0)
			{
				//check immediate is not too large for ARM and if it is then add additional imm
				if (imm > 0xFFF)
				{
					new_ins = newInstrI(ARM_ADD, AL_B, REG_TEMP_SCRATCH0, REG_TEMP_SCRATCH0, REG_NOT_USED, (imm)&0xf000);
					ADD_LL_NEXT(new_ins, ins);
				}
				// now store the value at REG_TEMP_SCRATCH0 ( This will be R2 + host base + funcTempImm&0xf000 )
				new_ins = newInstrI(ARM_LDR, AL_B, Rd1, REG_NOT_USED, REG_TEMP_SCRATCH0, imm&0xfff);
				new_ins->U = 1;
				ADD_LL_NEXT(new_ins, ins);
			}
			else
			{
				if  (imm < -0xFFF)
				{
					new_ins = newInstrI(ARM_SUB, AL_B, REG_TEMP_SCRATCH0, REG_TEMP_SCRATCH0, REG_NOT_USED, (-imm)&0xf000);
					ADD_LL_NEXT(new_ins, ins);
				}

				// now store the value at REG_TEMP_SCRATCH0 ( This will be R2 + host base + funcTempImm&0xf000 )
				new_ins = newInstrI(ARM_LDR, AL_B, Rd1, REG_NOT_USED, REG_TEMP_SCRATCH0, (-imm)&0xfff);
				new_ins->U = 0U;
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

			ins = insertCall_To_C(codeSegment,ins, AL_B, (size_t)&virtual_address, 0U);

			//Clear Status
			new_ins = newInstr(ARM_LDR, AL_B, Rd1, REG_NOT_USED, REG_HOST_R0);
			ADD_LL_NEXT(new_ins, ins);

			// Force delay of StoreCachedRegisters
			new_ins = newInstr(NO_OP, AL, Rd1, REG_NOT_USED, REG_NOT_USED);
			ADD_LL_NEXT(new_ins, ins);

			ins2->branchToThisInstruction = new_ins;
#endif
			break;
		case MIPS_LBU:
			if (imm < -0xFF)
			{
				InstrI(ins, ARM_SUB, AL, REG_HOST_R0, R1, REG_NOT_USED, (-imm) & 0xFF00U);

				new_ins = newInstrI(ARM_LDRB, AL, Rd1, REG_NOT_USED, REG_HOST_R0, -(imm & 0xFFU));
				ADD_LL_NEXT(new_ins, ins);
			}
			else if (imm > 0xFF)
			{
				InstrI(ins, ARM_ADD, AL, REG_HOST_R0, R1, REG_NOT_USED, imm & 0xFF00U);

				new_ins = newInstrI(ARM_LDRB, AL, Rd1, REG_NOT_USED, REG_HOST_R0, (imm & 0xFFU));
				ADD_LL_NEXT(new_ins, ins);
			}
			else
			{
				InstrI(ins, ARM_LDRB, AL, Rd1, REG_NOT_USED, R1, imm);
			}
			break;
		case MIPS_LHU:
		case MIPS_LWR:
		case MIPS_LWU:
			TRANSLATE_ABORT();
			break;
		default: break;
		}

		ins = ins->nextInstruction;
	}
}
