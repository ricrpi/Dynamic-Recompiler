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
#include "InstructionSetARM6hf.h"
#include "memory.h"


static void getTgtAddress(code_seg_t* const codeSegment, int32_t offset, size_t* tgt_address)
{
	code_seg_t* 	BranchToSeg = getSegmentAt((size_t)(codeSegment->MIPScode) + codeSegment->MIPScodeLen * 4 + offset * 4);

	if (BranchToSeg != NULL)
	{
		if (BranchToSeg->ARMEntryPoint != NULL)
		{
			*tgt_address = (size_t)BranchToSeg->ARMEntryPoint;
		}
		else
		{
			*tgt_address = *((size_t*)(MMAP_FP_BASE + FUNC_GEN_BRANCH_UNKNOWN));
		}
	}
	else // No segment Found
	{
		*tgt_address = *((size_t*)(MMAP_FP_BASE + FUNC_GEN_BRANCH_UNKNOWN));
	}
}

static void getTgtJAddress(code_seg_t* const codeSegment, int32_t offset, size_t* tgt_address)
{
	// Need to build the address using the last nibble from the instruction within the delay slot + offset << 2
	code_seg_t* 	BranchToSeg = getSegmentAt((((size_t)(codeSegment->MIPScode) + codeSegment->MIPScodeLen * 4U + 4U)&0xF0000000U) + offset * 4);

	if (BranchToSeg != NULL)
	{
		if (BranchToSeg->ARMEntryPoint != NULL)
		{
			*tgt_address = (size_t)BranchToSeg->ARMEntryPoint;
		}
		else
		{
			*tgt_address = *((size_t*)(MMAP_FP_BASE + FUNC_GEN_BRANCH_UNKNOWN));
		}
	}
	else // No segment Found
	{
		*tgt_address = *((size_t*)(MMAP_FP_BASE + FUNC_GEN_BRANCH_UNKNOWN));
	}
}

static uint32_t FindInstructionIndex(code_seg_t* const codeSegment, const Instruction_t* const find_ins)
{
	uint32_t index = 0U;
	Instruction_t*	ins = codeSegment->Intermcode;

	while ((ins != NULL) && (ins != find_ins))
	{
		index++;
		ins = ins->nextInstruction;
	}

	return index;
}

void Translate_InterCode_Branch(code_seg_t* const codeSegment)
{
	Instruction_t*	ins;
	ins = 			codeSegment->Intermcode;
	uint32_t index = 0U;

#if defined(USE_INSTRUCTION_COMMENTS)
	currentTranslation = "Resolving Intermediate Branches";
#endif

	while (ins)
	{
		switch (ins->instruction)
		{
		case INT_BRANCH:
			ins->instruction = ARM_B;
			ins->offset = FindInstructionIndex(codeSegment, ins->branchToThisInstruction) - index;
			break;

		case ARM_LDR_LIT:
		case ARM_MOV:
			if (ins->Rd1.regID == REG_HOST_LR && ins->branchToThisInstruction)
			{
				int32_t offset;
				offset = (FindInstructionIndex(codeSegment, ins->branchToThisInstruction) - index)*4 - 8;

				if (offset >= 0)
				{
					InstrI(ins, ARM_ADD, ins->cond, REG_HOST_LR, REG_HOST_PC, REG_NOT_USED, offset );
				}
				else
				{
					InstrI(ins, ARM_SUB, ins->cond, REG_HOST_LR, REG_HOST_PC, REG_NOT_USED, -offset );
				}
			}
			break;
		default: break;
		}
		index++;
		ins = ins->nextInstruction;
	}
}

void Translate_Branch(code_seg_t* const codeSegment)
{
	Instruction_t*	ins;
	Instruction_t*	new_ins;
	Instruction_e	instruction;
	int32_t 		offset;
	size_t 			tgt_address;
	uint8_t 		branchAbsolute 		= 1U;
	uint32_t 		instructionCount 	= 0U;

	regID_t base;

	ins = 			codeSegment->Intermcode;

#if defined(USE_INSTRUCTION_COMMENTS)
	currentTranslation = "Branching";
#endif

	while (ins)
	{
		instruction = ins->instruction;
		regID_t R1 = ins->R1.regID;
		regID_t R2 = ins->R2.regID;
		regID_t Rd1 = ins->Rd1.regID;

		switch (instruction)
		{
		case BEQ:
		case BEQL:
			offset = ins->offset;
			if (0 == ins->R2.regID)
			{
				InstrI(ins, ARM_CMP, AL, REG_NOT_USED, R1 | REG_WIDE, REG_NOT_USED, 0);

				new_ins = newInstrI(ARM_CMP, EQ, REG_NOT_USED, R1 , REG_NOT_USED, 0);
				ADD_LL_NEXT(new_ins, ins);
			}
			else
			{
				Instr(ins, ARM_CMP, AL, REG_NOT_USED, R1 | REG_WIDE, R2 | REG_WIDE);

				new_ins = newInstr(ARM_CMP, EQ, REG_NOT_USED, R1 , ins->R2.regID);
				ADD_LL_NEXT(new_ins, ins);
			}

			//if segment loops on its self
			if (getSegmentAt((size_t)(codeSegment->MIPScode + codeSegment->MIPScodeLen + offset)) == codeSegment)
			{
				new_ins = newInstrIntB(EQ, codeSegment->Intermcode);
			}
			else
			{
				getTgtAddress(codeSegment,offset, &tgt_address);
				if (tgt_address == *((size_t*)(MMAP_FP_BASE + FUNC_GEN_BRANCH_UNKNOWN)))
				{
					addLiteral(codeSegment, &base, &offset, (codeSegment->MIPScode + codeSegment->MIPScodeLen + offset));
					new_ins = newInstrI(ARM_LDR_LIT, AL, REG_HOST_R0, REG_NOT_USED, base, offset);
					ADD_LL_NEXT(new_ins, ins);
				}
				new_ins = newInstrB(EQ, tgt_address, 1);
			}

			if (instruction & OPS_LINK)
			{
				new_ins->Ln = 1;
			}
			ADD_LL_NEXT(new_ins, ins);

			break;

		case BNE:
		case BNEL:

			offset = ins->offset;
			if (0 == ins->R2.regID)
			{
				InstrI(ins, ARM_CMP, AL, REG_NOT_USED, R1 | REG_WIDE, REG_NOT_USED, 0);

				new_ins = newInstrI(ARM_CMP, EQ, REG_NOT_USED, R1 , REG_NOT_USED, 0);
				ADD_LL_NEXT(new_ins, ins);
			}
			else
			{
				Instr(ins, ARM_CMP, AL, REG_NOT_USED, R1 | REG_WIDE, R2 | REG_WIDE);

				new_ins = newInstr(ARM_CMP, EQ, REG_NOT_USED, R1 , R2);
				ADD_LL_NEXT(new_ins, ins);
			}

			//if segment loops on its self
			if (getSegmentAt((size_t)(codeSegment->MIPScode + codeSegment->MIPScodeLen + offset)) == codeSegment)
			{
				new_ins = newInstrIntB(NE, codeSegment->Intermcode);
			}
			else
			{
				getTgtAddress(codeSegment,offset, &tgt_address);

				if (tgt_address == *((size_t*)(MMAP_FP_BASE + FUNC_GEN_BRANCH_UNKNOWN)))
				{
					addLiteral(codeSegment, &base, &offset, (codeSegment->MIPScode + codeSegment->MIPScodeLen + offset));
					new_ins = newInstrI(ARM_LDR_LIT, AL, REG_HOST_R0, REG_NOT_USED, base, offset);
					ADD_LL_NEXT(new_ins, ins);
				}
				new_ins = newInstrB(NE, tgt_address, 1);
			}

			if (instruction & OPS_LINK) new_ins->Ln = 1;
			ADD_LL_NEXT(new_ins, ins);

			break;

		case J:
			offset = ins->offset;
			getTgtJAddress(codeSegment,offset, &tgt_address);
			InstrB(ins, AL, tgt_address, 1U);
			break;
		case JAL:
			offset = ins->offset;
			getTgtJAddress(codeSegment,offset, &tgt_address);

			InstrPUSH(ins, AL, REG_HOST_STM_LR);


			new_ins = newInstrB(AL, tgt_address, 1U);
			ins->Ln = 1U;
			ADD_LL_NEXT(new_ins, ins);

			new_ins = newInstrPOP(AL, REG_HOST_STM_LR);
			ADD_LL_NEXT(new_ins, ins);
			break;
		case JR:
#ifdef TEST
			if (ins->R1.regID == 0)	// this is an invalid instruction
			{
				InstrB(ins, AL, *((uint32_t*)(MMAP_FP_BASE + FUNC_GEN_STOP)), 1);
			}
			else
#endif
			{
				Instr(ins, ARM_MOV, AL, REG_HOST_R0, REG_NOT_USED, ins->R1.regID);

				tgt_address = *((size_t*)(MMAP_FP_BASE + FUNC_GEN_BRANCH_UNKNOWN));

				new_ins = newInstrB(AL, tgt_address, 1U);
				new_ins->Ln = 1U;
				ADD_LL_NEXT(new_ins, ins);

				new_ins = newInstr(ARM_BX, AL, REG_NOT_USED, REG_TEMP_SCRATCH0, REG_NOT_USED);
				ADD_LL_NEXT(new_ins, ins);
			}
			break;
		case JALR:
			// we need to lookup the code segment we should be branching to according to the value in the register
			// work out address in code segment lookup table

			Instr(ins, ARM_MOV, AL, REG_HOST_R0, REG_NOT_USED, ins->R1.regID);

			tgt_address = *((size_t*)(MMAP_FP_BASE + FUNC_GEN_BRANCH_UNKNOWN));

			new_ins = newInstrPUSH(AL, REG_HOST_STM_LR);
			ADD_LL_NEXT(new_ins, ins);

			new_ins = newInstrB(AL, tgt_address, 1U);
			new_ins->Ln = 1U;
			ADD_LL_NEXT(new_ins, ins);

			new_ins = newInstr(ARM_BX, AL, REG_NOT_USED, REG_TEMP_SCRATCH0, REG_NOT_USED);
			new_ins->Ln = 1U;
			ADD_LL_NEXT(new_ins, ins);

			new_ins = newInstrPOP(AL, REG_HOST_STM_LR);
			ADD_LL_NEXT(new_ins, ins);
			break;

		case BLTZ:
		case BLTZL:
		case BLTZAL:
		case BLTZALL:
			offset = ins->offset;
			if (0 == ins->R2.regID)
			{
				InstrI(ins, ARM_CMP, AL, REG_NOT_USED, R1 | REG_WIDE, REG_NOT_USED, 0);

				//new_ins = newInstrI(ARM_CMP, PL, REG_NOT_USED, ins->R1.regID , REG_NOT_USED, 0);
				//ADD_LL_NEXT(new_ins, ins);
			}
			else
			{
				Instr(ins, ARM_CMP, AL, REG_NOT_USED, R1 | REG_WIDE, R2 | REG_WIDE);

				//new_ins = newInstrI(ARM_CMP, PL, REG_NOT_USED, ins->R1.regID , ins->R2.regID);
				//ADD_LL_NEXT(new_ins, ins);
			}

			//if segment loops on its self
			if (getSegmentAt((size_t)(codeSegment->MIPScode + codeSegment->MIPScodeLen + offset)) == codeSegment)
			{
				new_ins = newInstrIntB(MI, codeSegment->Intermcode);
			}
			else
			{
				getTgtAddress(codeSegment, offset, &tgt_address);

				if (tgt_address == *((size_t*)(MMAP_FP_BASE + FUNC_GEN_BRANCH_UNKNOWN)))
				{
					addLiteral(codeSegment, &base, &offset, (codeSegment->MIPScode + codeSegment->MIPScodeLen + offset));
					new_ins = newInstrI(ARM_LDR_LIT, AL, REG_HOST_R0, REG_NOT_USED, base, offset);
					ADD_LL_NEXT(new_ins, ins);
				}
				new_ins = newInstrB(MI, tgt_address, 1);
			}

			if (instruction & OPS_LINK) new_ins->Ln = 1;
			ADD_LL_NEXT(new_ins, ins);


			break;

		case BGEZ:
		case BGEZL:
		case BGEZAL:
		case BGEZALL:

			offset = ins->offset;

			InstrI(ins, ARM_CMP, AL, REG_NOT_USED, R1 | REG_WIDE, REG_NOT_USED, 0);

			new_ins = newInstrI(ARM_CMP, PL, REG_NOT_USED, R1 , REG_NOT_USED, 0); // if not minus
			ADD_LL_NEXT(new_ins, ins);

			//if segment loops on its self
			if (getSegmentAt((size_t)(codeSegment->MIPScode + codeSegment->MIPScodeLen + offset)) == codeSegment)
			{
				new_ins = newInstrIntB(GEZ, codeSegment->Intermcode);
			}
			else
			{
				getTgtAddress(codeSegment,offset, &tgt_address);
				if (tgt_address == *((size_t*)(MMAP_FP_BASE + FUNC_GEN_BRANCH_UNKNOWN)))
				{
					addLiteral(codeSegment, &base, &offset, (codeSegment->MIPScode + codeSegment->MIPScodeLen + offset));
					new_ins = newInstrI(ARM_LDR_LIT, AL, REG_HOST_R0, REG_NOT_USED, base, offset);
					ADD_LL_NEXT(new_ins, ins);
				}
				new_ins = newInstrB(GEZ, tgt_address, 1);
			}

			if (instruction & OPS_LINK) new_ins->Ln = 1;
			ADD_LL_NEXT(new_ins, ins);

			break;

		case BLEZ:
		case BLEZL:

			offset = ins->offset;

			InstrI(ins, ARM_CMP, AL, REG_NOT_USED, R1 | REG_WIDE, REG_NOT_USED, 0);

			new_ins = newInstrI(ARM_CMP, EQ, REG_NOT_USED, R1 , REG_NOT_USED, 0); // if upper 64 bit is EQ then check lower
			ADD_LL_NEXT(new_ins, ins);

			//if segment loops on its self
			if (getSegmentAt((size_t)(codeSegment->MIPScode + codeSegment->MIPScodeLen + offset)) == codeSegment)
			{
				new_ins = newInstrIntB(LEZ, codeSegment->Intermcode);
			}
			else
			{
				getTgtAddress(codeSegment, offset, &tgt_address);
				if (tgt_address == *((size_t*)(MMAP_FP_BASE + FUNC_GEN_BRANCH_UNKNOWN)))
				{
					addLiteral(codeSegment, &base, &offset, (codeSegment->MIPScode + codeSegment->MIPScodeLen + offset));
					new_ins = newInstrI(ARM_LDR_LIT, AL, REG_HOST_R0, REG_NOT_USED, base, offset);
					ADD_LL_NEXT(new_ins, ins);
				}
				new_ins = newInstrB(LEZ, tgt_address, branchAbsolute);
			}

			if (instruction & OPS_LINK) new_ins->Ln = 1;
			ADD_LL_NEXT(new_ins, ins);

			break;

		case BGTZ:
		case BGTZL:

			offset = ins->offset;

			InstrI(ins, ARM_CMP, AL, REG_NOT_USED, R1 | REG_WIDE, REG_NOT_USED, 0);

			new_ins = newInstrI(ARM_CMP, EQ, REG_NOT_USED, R1 , REG_NOT_USED, 0); // upper is 0 what about lower
			ADD_LL_NEXT(new_ins, ins);

			//if segment loops on its self
			if (getSegmentAt((size_t)(codeSegment->MIPScode + codeSegment->MIPScodeLen + offset)) == codeSegment)
			{
				new_ins = newInstrIntB(HI, codeSegment->Intermcode);
			}
			else
			{
				getTgtAddress(codeSegment, offset, &tgt_address);
				if (tgt_address == *((size_t*)(MMAP_FP_BASE + FUNC_GEN_BRANCH_UNKNOWN)))
				{
					addLiteral(codeSegment, &base, &offset, (codeSegment->MIPScode + codeSegment->MIPScodeLen + offset));
					new_ins = newInstrI(ARM_LDR_LIT, AL, REG_HOST_R0, REG_NOT_USED, base, offset);
					ADD_LL_NEXT(new_ins, ins);
				}
				new_ins = newInstrB(HI, tgt_address, branchAbsolute);
			}

			if (instruction & OPS_LINK) new_ins->Ln = 1U;
			ADD_LL_NEXT(new_ins, ins);

			break;

		case BC1F:
		case BC1T:
		case BC1FL:
		case BC1TL:
			TRANSLATE_ABORT();
			break;

		default:
			break;
		}

		ins = ins->nextInstruction;
		instructionCount++;
	}
}
