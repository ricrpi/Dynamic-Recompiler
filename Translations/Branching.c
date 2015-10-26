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

#include "DebugDefines.h"
#include "Translate.h"
#include "InstructionSet_ascii.h"
#include "InstructionSetARM6hf.h"
#include "memory.h"
#include "mem_state.h"
#include "InstructionSetMIPS4.h"

static void getTgtAddress(code_seg_t* const codeSegment, const int32_t offset, size_t* const tgt_address)
{
	const code_seg_t* const BranchToSeg = getSegmentAt((size_t)(codeSegment->MIPScode) + codeSegment->MIPScodeLen * 4 + offset * 4);

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
	const code_seg_t* const	BranchToSeg = getSegmentAt((((size_t)(codeSegment->MIPScode) + codeSegment->MIPScodeLen * 4U + 4U)&0xF0000000U) + offset * 4);

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

#if USE_INSTRUCTION_COMMENTS
	currentTranslation = "Resolving Intermediate Branches";
#endif

	while (ins)
	{
		switch (ins->instruction)
		{
		case DR_INT_BRANCH:
			ins->instruction = ARM_B;
			ins->offset = FindInstructionIndex(codeSegment, ins->branchToThisInstruction) - index;
			break;
		case DR_INT_BRANCH_LINK:
			ins->instruction = ARM_BL;
			ins->offset = FindInstructionIndex(codeSegment, ins->branchToThisInstruction) - index;
			break;
		case ARM_LDR_LIT:
		case ARM_MOV:
			if (ins->Rd1.regID == REG_HOST_LR && ins->branchToThisInstruction)
			{
				int32_t offset;
				offset = (FindInstructionIndex(codeSegment, ins->branchToThisInstruction) - index)*4 - 8U;

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

/* Description:	Function to translate MIPS Branch and JUMP instructions into ARM equivalent.
 *
 * 				It may be possible to resolve the target branch at translation time	however as the target branch could
 * 				be deleted, the code must still contain instructions to set HOST_R1 and HOST_R2 for calling
 * 				'size_t branchUnknown(code_seg_t* code_seg, size_t* address)'
 *
 * 				Branches that loop to themselves do not need to set HOST_R1 and HOST_R2 as the branch is always known
 *
 * 				branchUnknown() does not 'patch' the ARM branch instruction if the MIPS opcode is a JR or JALR.
 *
 * Parameters:	code_seg_t* const codeSegment		The code segment to translate
 *
 * Returns:		void
 */
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

	Instruction_t* delayInstruction 		= NULL;
	Instruction_t* LikelybranchInstruction 	= NULL;

	ins = 			codeSegment->Intermcode;

#if USE_INSTRUCTION_COMMENTS
	currentTranslation = "Delay Slot";
#endif

	// generate the intermediate delay lot instruction
	delayInstruction = newEmptyInstr();
	mips_decode(*(codeSegment->MIPScode + codeSegment->MIPScodeLen), delayInstruction);

#if USE_INSTRUCTION_COMMENTS
	currentTranslation = "Branching";
#endif

	// if this is a SEG_SANDWICH or SEG_END then we need to conditional branch over the first instruction (1)
	// unless first instruction is a no op
	if ( (codeSegment->Type == SEG_SANDWICH || codeSegment->Type == SEG_END)
			&& ops_type(codeSegment->MIPScode[0]) > DR_NO_OP)
	{
		new_ins 	= newInstrI(ARM_TST, AL, REG_NOT_USED, REG_EMU_FLAGS, REG_NOT_USED, 1U << REG_EMU_FLAG_DS);
		new_ins->nextInstruction = ins;
		codeSegment->Intermcode = ins = new_ins;

		// add a branch to skip over the first instruction
		new_ins 	= newInstrIntB(NE, ins->nextInstruction->nextInstruction);
		ADD_LL_NEXT(new_ins, ins);
	}

	// now we can go to the instruction before the branch for
	// 	(1) insertion of the delay slot,
	// 	(2) clearing the DR flag so that delayslots in branched to segments are executed
	//  (3) translation of the branch itself
	//  (4) setting the DR flag after a branch instruction to stop delay slot instructions being executed
	while (ins && ins->nextInstruction)
	{
		instruction = ins->nextInstruction->instruction;
		regID_t R1 = ins->nextInstruction->R1.regID;
		regID_t R2 = ins->nextInstruction->R2.regID;
		offset = ins->nextInstruction->offset;

		switch (instruction)
		{
		case MIPS_BEQ:

			// (1)
			if (delayInstruction->instruction > DR_NO_OP)
			{
				ADD_LL_NEXT(delayInstruction, ins);
				ins= ins->nextInstruction;
			}

			// (3)
			if (0 == R2)
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

			// (2)
			new_ins 	= newInstrI(ARM_BIC,AL, REG_EMU_FLAGS, REG_EMU_FLAGS, REG_NOT_USED, 1U << REG_EMU_FLAG_DS);
			ADD_LL_NEXT(new_ins, ins);


			// (3) if segment loops on its self
			if (getSegmentAt((size_t)(codeSegment->MIPScode + codeSegment->MIPScodeLen + offset)) == codeSegment)
			{
				new_ins = newInstrIntB(EQ, codeSegment->Intermcode);
			}
			else
			{
				getTgtAddress(codeSegment,offset, &tgt_address);

				// ---- Dynamic Recompiler Branching ABI ----

				addLiteral(codeSegment, &base, &offset, (uint32_t)codeSegment);
				new_ins = newInstrI(ARM_LDR_LIT, AL, REG_HOST_R0, REG_NOT_USED, base, offset);
				ADD_LL_NEXT(new_ins, ins);

				new_ins = newInstrI(ARM_SUB, AL, REG_HOST_R1, REG_HOST_PC, REG_NOT_USED, 4U);
				ADD_LL_NEXT(new_ins, ins);

				// ---- End ---------------------------------

				new_ins = newInstrB(EQ, tgt_address, 1);
			}
			ADD_LL_NEXT(new_ins, ins);

			// (4)
			new_ins 	= newInstrI(ARM_ORR,AL, REG_EMU_FLAGS, REG_EMU_FLAGS, REG_NOT_USED, 1U << REG_EMU_FLAG_DS);
			ADD_LL_NEXT(new_ins, ins);

			return;
		case MIPS_BEQL:
			// Select branch instruction
			ins= ins->nextInstruction;

			// (3)
			if (0 == R2)
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

			LikelybranchInstruction = ins;

			// (1)
			if (delayInstruction->instruction > DR_NO_OP)
			{
				ADD_LL_NEXT(delayInstruction, ins);
			}

			// (2)
			new_ins 	= newInstrI(ARM_BIC,AL, REG_EMU_FLAGS, REG_EMU_FLAGS, REG_NOT_USED, 1U << REG_EMU_FLAG_DS);
			ADD_LL_NEXT(new_ins, ins);

			// (3) if segment loops on its self
			if (getSegmentAt((size_t)(codeSegment->MIPScode + codeSegment->MIPScodeLen + offset)) == codeSegment)
			{
				new_ins = newInstrIntB(AL, codeSegment->Intermcode);
			}
			else
			{
				getTgtAddress(codeSegment,offset, &tgt_address);

				// ---- Dynamic Recompiler Branching ABI ----

				addLiteral(codeSegment, &base, &offset, (uint32_t)codeSegment);
				new_ins = newInstrI(ARM_LDR_LIT, AL, REG_HOST_R0, REG_NOT_USED, base, offset);
				ADD_LL_NEXT(new_ins, ins);

				new_ins = newInstrI(ARM_SUB, AL, REG_HOST_R1, REG_HOST_PC, REG_NOT_USED, 4U);
				ADD_LL_NEXT(new_ins, ins);

				// ---- End ---------------------------------

				new_ins = newInstrB(AL, tgt_address, 1);
			}
			ADD_LL_NEXT(new_ins, ins);

			// (4)
			new_ins 	= newInstrI(ARM_ORR,AL, REG_EMU_FLAGS, REG_EMU_FLAGS, REG_NOT_USED, 1U << REG_EMU_FLAG_DS);
			ADD_LL_NEXT(new_ins, ins);

			if (delayInstruction->instruction > DR_NO_OP)
			{
				// Provide a jump if not branching on LIKELY
				new_ins = newInstrIntB(NE, new_ins);
				ADD_LL_NEXT(new_ins, LikelybranchInstruction);
			}
			return;

		case MIPS_BNE:

			// (1)
			if (delayInstruction->instruction > DR_NO_OP)
			{
				ADD_LL_NEXT(delayInstruction, ins);
				ins= ins->nextInstruction;
			}

			// (3)
			if (0 == R2)
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

			// (2)
			new_ins 	= newInstrI(ARM_BIC,AL, REG_EMU_FLAGS, REG_EMU_FLAGS, REG_NOT_USED, 1U << REG_EMU_FLAG_DS);
			ADD_LL_NEXT(new_ins, ins);

			// (3) if segment loops on its self
			if (getSegmentAt((size_t)(codeSegment->MIPScode + codeSegment->MIPScodeLen + offset)) == codeSegment)
			{
				new_ins = newInstrIntB(NE, codeSegment->Intermcode);
			}
			else
			{
				getTgtAddress(codeSegment,offset, &tgt_address);

				// ---- Dynamic Recompiler Branching ABI ----

				addLiteral(codeSegment, &base, &offset, (uint32_t)codeSegment);
				new_ins = newInstrI(ARM_LDR_LIT, AL, REG_HOST_R0, REG_NOT_USED, base, offset);
				ADD_LL_NEXT(new_ins, ins);

				new_ins = newInstrI(ARM_SUB, AL, REG_HOST_R1, REG_HOST_PC, REG_NOT_USED, 4U);
				ADD_LL_NEXT(new_ins, ins);

				// ---- End ---------------------------------

				new_ins = newInstrB(NE, tgt_address, 1);
			}
			ADD_LL_NEXT(new_ins, ins);

			// (4)
			new_ins 	= newInstrI(ARM_ORR,AL, REG_EMU_FLAGS, REG_EMU_FLAGS, REG_NOT_USED, 1U << REG_EMU_FLAG_DS);
			ADD_LL_NEXT(new_ins, ins);

			return;
		case MIPS_BNEL:
			// Select branch instruction
			ins= ins->nextInstruction;

			// (3)
			if (0 == R2)
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

			LikelybranchInstruction = ins;

			// (1)
			if (delayInstruction->instruction > DR_NO_OP)
			{
				ADD_LL_NEXT(delayInstruction, ins);
			}

			// (2)
			new_ins 	= newInstrI(ARM_BIC,AL, REG_EMU_FLAGS, REG_EMU_FLAGS, REG_NOT_USED, 1U << REG_EMU_FLAG_DS);
			ADD_LL_NEXT(new_ins, ins);

			// (3) if segment loops on its self
			if (getSegmentAt((size_t)(codeSegment->MIPScode + codeSegment->MIPScodeLen + offset)) == codeSegment)
			{
				new_ins = newInstrIntB(AL, codeSegment->Intermcode);
			}
			else
			{
				getTgtAddress(codeSegment,offset, &tgt_address);

				// ---- Dynamic Recompiler Branching ABI ----

				addLiteral(codeSegment, &base, &offset, (uint32_t)codeSegment);
				new_ins = newInstrI(ARM_LDR_LIT, AL, REG_HOST_R0, REG_NOT_USED, base, offset);
				ADD_LL_NEXT(new_ins, ins);

				new_ins = newInstrI(ARM_SUB, AL, REG_HOST_R1, REG_HOST_PC, REG_NOT_USED, 4U);
				ADD_LL_NEXT(new_ins, ins);

				// ---- End ---------------------------------

				new_ins = newInstrB(AL, tgt_address, 1);
			}
			ADD_LL_NEXT(new_ins, ins);

			// (4)
			new_ins 	= newInstrI(ARM_ORR,AL, REG_EMU_FLAGS, REG_EMU_FLAGS, REG_NOT_USED, 1U << REG_EMU_FLAG_DS);
			ADD_LL_NEXT(new_ins, ins);

			if (delayInstruction->instruction > DR_NO_OP)
			{
				// Provide a jump if not branching on LIKELY
				new_ins = newInstrIntB(EQ, new_ins);
				ADD_LL_NEXT(new_ins, LikelybranchInstruction);
			}

			return;
		case MIPS_J:
			getTgtJAddress(codeSegment,offset, &tgt_address);

			// (1)
			if (delayInstruction->instruction > DR_NO_OP)
			{
				ADD_LL_NEXT(delayInstruction, ins);
				ins= ins->nextInstruction;
			}

			// (2)
			InstrI(ins, ARM_BIC, AL, REG_EMU_FLAGS, REG_EMU_FLAGS, REG_NOT_USED, 1U << REG_EMU_FLAG_DS);

			// (3)
			new_ins = newInstrB(AL, tgt_address, 1U);
			ADD_LL_NEXT(new_ins, ins);

			return;
		case MIPS_JAL:
			getTgtJAddress(codeSegment,offset, &tgt_address);

			// (1)
			if (delayInstruction->instruction > DR_NO_OP)
			{
				ADD_LL_NEXT(delayInstruction, ins);
				ins= ins->nextInstruction;
			}

			// (2)
			InstrI(ins, ARM_BIC, AL, REG_EMU_FLAGS, REG_EMU_FLAGS, REG_NOT_USED, 1U << REG_EMU_FLAG_DS);

			// (3)
#if USE_HOST_MANAGED_BRANCHING
			new_ins = newInstrPUSH(AL, REG_HOST_STM_LR);
			ADD_LL_NEXT(new_ins, ins);

			new_ins = newInstrBL(AL, tgt_address, 1U);
			ADD_LL_NEXT(new_ins, ins);

			new_ins = newInstrPOP(AL, REG_HOST_STM_LR);
			ADD_LL_NEXT(new_ins, ins);
#else
			// Store the MIPS PC+8 into REG 31
			addLiteral(codeSegment, &base, &offset, (uint32_t)(codeSegment->MIPScode + codeSegment->MIPScodeLen + 1U));

			// REG 31 is updated regardless of whether the branch is taken
			new_ins = newInstrI(ARM_LDR_LIT, AL, 31U, REG_NOT_USED, base, offset);
			ADD_LL_NEXT(new_ins, ins);

			new_ins = newInstrB(AL, tgt_address, 1U);
			ADD_LL_NEXT(new_ins, ins);
#endif

			// (4)
			new_ins 	= newInstrI(ARM_ORR,AL, REG_EMU_FLAGS, REG_EMU_FLAGS, REG_NOT_USED, 1U << REG_EMU_FLAG_DS);
			ADD_LL_NEXT(new_ins, ins);

			return;
		case MIPS_JR:

			// (1)
			if (delayInstruction->instruction > DR_NO_OP)
			{
				ADD_LL_NEXT(delayInstruction, ins);

			}
			ins= ins->nextInstruction;

#ifdef TEST
			if (R1 == 0)	// this should be an invalid instruction
			{
				InstrB(ins, AL, *((uint32_t*)(MMAP_FP_BASE + FUNC_GEN_STOP)), 1);
			}
			else
#endif
			{
#if USE_HOST_MANAGED_BRANCHING
				if (R1 == 31U)
				{
					Instr(ins, ARM_MOV, AL, REG_HOST_PC, REG_NOT_USED, REG_HOST_LR);
				}
				else
#endif
				{
					Instr(ins, ARM_MOV, AL, REG_HOST_R0, REG_NOT_USED, R1);

					tgt_address = *((size_t*)(MMAP_FP_BASE + FUNC_GEN_BRANCH_UNKNOWN));

					new_ins = newInstrPUSH(AL, REG_HOST_STM_LR);
					ADD_LL_NEXT(new_ins, ins);

					new_ins = newInstrBL(AL, tgt_address, 1U);
					ADD_LL_NEXT(new_ins, ins);

					new_ins = newInstrPOP(AL, REG_HOST_STM_LR);
					ADD_LL_NEXT(new_ins, ins);

					new_ins = newInstr(ARM_BX, AL, REG_NOT_USED, REG_HOST_R0, REG_NOT_USED);
					ADD_LL_NEXT(new_ins, ins);
				}
			}
			break;
		case MIPS_JALR:

			// (1)
			if (delayInstruction->instruction > DR_NO_OP)
			{
				ADD_LL_NEXT(delayInstruction, ins);
				ins= ins->nextInstruction;
			}

			// (2)
			InstrI(ins, ARM_BIC, AL, REG_EMU_FLAGS, REG_EMU_FLAGS, REG_NOT_USED, 1U << REG_EMU_FLAG_DS);

			// Store the MIPS PC+8 into REG 31
			addLiteral(codeSegment, &base, &offset, (uint32_t)(codeSegment->MIPScode + codeSegment->MIPScodeLen + 1U));

			// REG 31
			new_ins = newInstrI(ARM_LDR_LIT, AL, 31U, REG_NOT_USED, base, offset);
			ADD_LL_NEXT(new_ins, ins);

			// we need to lookup the code segment we should be branching to according to the value in the register
			// work out address in code segment lookup table

			// ---- Dynamic Recompiler Branching ABI ----

			addLiteral(codeSegment, &base, &offset, (uint32_t)(codeSegment->MIPScode + codeSegment->MIPScodeLen + offset));
			new_ins = newInstrI(ARM_LDR_LIT, AL, REG_HOST_R0, REG_NOT_USED, base, offset);
			ADD_LL_NEXT(new_ins, ins);

#if USE_HOST_MANAGED_BRANCHING
			new_ins = newInstrI(ARM_ADD, AL, REG_HOST_R1, REG_HOST_PC, REG_NOT_USED, 8U);
			ADD_LL_NEXT(new_ins, ins);
#else
			new_ins = newInstrI(ARM_SUB, AL, REG_HOST_R1, REG_HOST_PC, REG_NOT_USED, 4U);
			ADD_LL_NEXT(new_ins, ins);
#endif
			// ---- End ---------------------------------

			tgt_address = *((size_t*)(MMAP_FP_BASE + FUNC_GEN_BRANCH_UNKNOWN));

			// (3)
			new_ins = newInstrPUSH(AL, REG_HOST_STM_LR);
			ADD_LL_NEXT(new_ins, ins);

			// call branchUnknown() to get arm address in REG_HOST_R0
			new_ins = newInstrBL(AL, tgt_address, 1U);
			ADD_LL_NEXT(new_ins, ins);

#if USE_HOST_MANAGED_BRANCHING
			// now branch to target segment
			new_ins = newInstr(ARM_BLX, AL, REG_NOT_USED, REG_HOST_R0, REG_NOT_USED);
			ADD_LL_NEXT(new_ins, ins);

			new_ins = newInstrPOP(AL, REG_HOST_STM_LR);
			ADD_LL_NEXT(new_ins, ins);
#else
			new_ins = newInstrPOP(AL, REG_HOST_STM_LR);
			ADD_LL_NEXT(new_ins, ins);

			new_ins = newInstr(ARM_BX, AL, REG_NOT_USED, REG_TEMP_SCRATCH0, REG_NOT_USED);
			ADD_LL_NEXT(new_ins, ins);
#endif
			return;

		case MIPS_BLTZ:

			// (1)
			if (delayInstruction->instruction > DR_NO_OP)
			{
				ADD_LL_NEXT(delayInstruction, ins);
				ins= ins->nextInstruction;
			}

			// (3)
			InstrI(ins, ARM_CMP, AL, REG_NOT_USED, R1 | REG_WIDE, REG_NOT_USED, 0);

			new_ins = newInstrI(ARM_CMP, MI, REG_NOT_USED, R1 , REG_NOT_USED, 0); // if not minus
			ADD_LL_NEXT(new_ins, ins);

			// (2)
			new_ins 	= newInstrI(ARM_BIC,AL, REG_EMU_FLAGS, REG_EMU_FLAGS, REG_NOT_USED, 1U << REG_EMU_FLAG_DS);
			ADD_LL_NEXT(new_ins, ins);

			// (3) if segment loops on its self
			if (getSegmentAt((size_t)(codeSegment->MIPScode + codeSegment->MIPScodeLen + offset)) == codeSegment)
			{
				new_ins = newInstrIntB(MI, codeSegment->Intermcode);
				ADD_LL_NEXT(new_ins, ins);
			}
			else
			{
				getTgtAddress(codeSegment, offset, &tgt_address);

				// ---- Dynamic Recompiler Branching ABI ----

				addLiteral(codeSegment, &base, &offset, (uint32_t)codeSegment);
				new_ins = newInstrI(ARM_LDR_LIT, AL, REG_HOST_R0, REG_NOT_USED, base, offset);
				ADD_LL_NEXT(new_ins, ins);

				new_ins = newInstrI(ARM_SUB, AL, REG_HOST_R1, REG_HOST_PC, REG_NOT_USED, 4U);
				ADD_LL_NEXT(new_ins, ins);

				// ---- End ---------------------------------

				new_ins = newInstrB(MI, tgt_address, 1);
				ADD_LL_NEXT(new_ins, ins);
			}

			// (4)
			new_ins 	= newInstrI(ARM_ORR,AL, REG_EMU_FLAGS, REG_EMU_FLAGS, REG_NOT_USED, 1U << REG_EMU_FLAG_DS);
			ADD_LL_NEXT(new_ins, ins);

			return;
		case MIPS_BLTZL:
			ins= ins->nextInstruction;

			InstrI(ins, ARM_CMP, AL, REG_NOT_USED, R1 | REG_WIDE, REG_NOT_USED, 0);

			//new_ins = newInstrI(ARM_CMP, MI, REG_NOT_USED, R1 , REG_NOT_USED, 0); // if not minus
			//ADD_LL_NEXT(new_ins, ins);

			LikelybranchInstruction = ins;

			// (1)
			if (delayInstruction->instruction > DR_NO_OP)
			{
				ADD_LL_NEXT(delayInstruction, ins);
			}

			// (2)
			new_ins 	= newInstrI(ARM_BIC,AL, REG_EMU_FLAGS, REG_EMU_FLAGS, REG_NOT_USED, 1U << REG_EMU_FLAG_DS);
			ADD_LL_NEXT(new_ins, ins);

			// (3) if segment loops on its self
			if (getSegmentAt((size_t)(codeSegment->MIPScode + codeSegment->MIPScodeLen + offset)) == codeSegment)
			{
				new_ins = newInstrIntB(AL, codeSegment->Intermcode);
				ADD_LL_NEXT(new_ins, ins);
			}
			else
			{
				getTgtAddress(codeSegment, offset, &tgt_address);

				// ---- Dynamic Recompiler Branching ABI ----

				addLiteral(codeSegment, &base, &offset, (uint32_t)codeSegment);
				new_ins = newInstrI(ARM_LDR_LIT, AL, REG_HOST_R0, REG_NOT_USED, base, offset);
				ADD_LL_NEXT(new_ins, ins);

				new_ins = newInstrI(ARM_SUB, AL, REG_HOST_R1, REG_HOST_PC, REG_NOT_USED, 4U);
				ADD_LL_NEXT(new_ins, ins);

				// ---- End ---------------------------------

				new_ins = newInstrB(AL, tgt_address, 1);
				ADD_LL_NEXT(new_ins, ins);
			}

			// (4)
			new_ins 	= newInstrI(ARM_ORR,AL, REG_EMU_FLAGS, REG_EMU_FLAGS, REG_NOT_USED, 1U << REG_EMU_FLAG_DS);
			ADD_LL_NEXT(new_ins, ins);

			//if (delayInstruction->instruction > DR_NO_OP)
			{
				// Provide a jump if not branching on LIKELY
				new_ins = newInstrIntB(PL, new_ins);
				ADD_LL_NEXT(new_ins, LikelybranchInstruction);
			}

			return;
		case MIPS_BLTZAL:
			// (1)
			if (delayInstruction->instruction > DR_NO_OP)
			{
				ADD_LL_NEXT(delayInstruction, ins);
				ins= ins->nextInstruction;
			}

			//change the MIPS_BLTZAL instruction
			InstrI(ins, ARM_CMP, AL, REG_NOT_USED, R1 | REG_WIDE, REG_NOT_USED, 0);

			new_ins = newInstrI(ARM_CMP, MI, REG_NOT_USED, R1 , REG_NOT_USED, 0); // if not minus
			ADD_LL_NEXT(new_ins, ins);

			// (2)
			new_ins 	= newInstrI(ARM_BIC,AL, REG_EMU_FLAGS, REG_EMU_FLAGS, REG_NOT_USED, 1U << REG_EMU_FLAG_DS);
			ADD_LL_NEXT(new_ins, ins);

			// Store the MIPS PC+8 into REG 31
			addLiteral(codeSegment, &base, &offset, (uint32_t)(codeSegment->MIPScode + codeSegment->MIPScodeLen + 1U));

			// REG 31 is updated regardless of whether the branch is taken
			new_ins = newInstrI(ARM_LDR_LIT, AL, 31U, REG_NOT_USED, base, offset);
			ADD_LL_NEXT(new_ins, ins);

			// (3) if segment loops on its self
			if (getSegmentAt((size_t)(codeSegment->MIPScode + codeSegment->MIPScodeLen + offset)) == codeSegment)
			{
				new_ins = newInstrPUSH(AL, REG_HOST_STM_LR);
				ADD_LL_NEXT(new_ins, ins);

				new_ins = newInstrIntBL(MI, codeSegment->Intermcode);

				new_ins = newInstrPOP(AL, REG_HOST_STM_LR);
				ADD_LL_NEXT(new_ins, ins);
			}
			else
			{
				getTgtAddress(codeSegment, offset, &tgt_address);

				// ---- Dynamic Recompiler Branching ABI ----

				addLiteral(codeSegment, &base, &offset, (uint32_t)codeSegment);
				new_ins = newInstrI(ARM_LDR_LIT, AL, REG_HOST_R0, REG_NOT_USED, base, offset);
				ADD_LL_NEXT(new_ins, ins);

#if USE_HOST_MANAGED_BRANCHING
				new_ins = newInstrI(ARM_ADD, AL, REG_HOST_R1, REG_HOST_PC, REG_NOT_USED, 0U);
				ADD_LL_NEXT(new_ins, ins);
#else
				new_ins = newInstrI(ARM_SUB, AL, REG_HOST_R1, REG_HOST_PC, REG_NOT_USED, 4U);
				ADD_LL_NEXT(new_ins, ins);
#endif
				// ---- End ---------------------------------

#if USE_HOST_MANAGED_BRANCHING
				new_ins = newInstrPUSH(AL, REG_HOST_STM_LR);
				ADD_LL_NEXT(new_ins, ins);

				new_ins = newInstrBL(MI, tgt_address, 1);
				ADD_LL_NEXT(new_ins, ins);

				new_ins = newInstrPOP(AL, REG_HOST_STM_LR);
				ADD_LL_NEXT(new_ins, ins);
#else
				new_ins = newInstrB(MI, tgt_address, 1);
				ADD_LL_NEXT(new_ins, ins);
#endif
			}

			// (4)
			new_ins 	= newInstrI(ARM_ORR,AL, REG_EMU_FLAGS, REG_EMU_FLAGS, REG_NOT_USED, 1U << REG_EMU_FLAG_DS);
			ADD_LL_NEXT(new_ins, ins);

			return;
		case MIPS_BLTZALL:
			ins= ins->nextInstruction;

			InstrI(ins, ARM_CMP, AL, REG_NOT_USED, R1 | REG_WIDE, REG_NOT_USED, 0);

			new_ins = newInstrI(ARM_CMP, MI, REG_NOT_USED, R1 , REG_NOT_USED, 0); // if not minus
			ADD_LL_NEXT(new_ins, ins);

			// Store the MIPS PC+8 into REG 31
			addLiteral(codeSegment, &base, &offset, (uint32_t)(codeSegment->MIPScode + codeSegment->MIPScodeLen + 1U));

			// REG 31 is updated regardless of whether the branch is taken
			new_ins = newInstrI(ARM_LDR_LIT, AL, 31, REG_NOT_USED, base, offset);
			ADD_LL_NEXT(new_ins, ins);

			LikelybranchInstruction = ins;

			// (1)
			if (delayInstruction->instruction > DR_NO_OP)
			{
				ADD_LL_NEXT(delayInstruction, ins);
			}

			//if segment loops on its self
			if (getSegmentAt((size_t)(codeSegment->MIPScode + codeSegment->MIPScodeLen + offset)) == codeSegment)
			{
				new_ins = newInstrPUSH(AL, REG_HOST_STM_LR);
				ADD_LL_NEXT(new_ins, ins);

				new_ins = newInstrIntBL(AL, codeSegment->Intermcode);
				ADD_LL_NEXT(new_ins, ins);

				new_ins = newInstrPOP(AL, REG_HOST_STM_LR);
				ADD_LL_NEXT(new_ins, ins);
			}
			else
			{
				getTgtAddress(codeSegment, offset, &tgt_address);

				// ---- Dynamic Recompiler Branching ABI ----

				addLiteral(codeSegment, &base, &offset, (uint32_t)codeSegment);
				new_ins = newInstrI(ARM_LDR_LIT, AL, REG_HOST_R0, REG_NOT_USED, base, offset);
				ADD_LL_NEXT(new_ins, ins);

#if USE_HOST_MANAGED_BRANCHING
				new_ins = newInstrI(ARM_ADD, AL, REG_HOST_R1, REG_HOST_PC, REG_NOT_USED, 0U);
				ADD_LL_NEXT(new_ins, ins);
#else
				new_ins = newInstrI(ARM_SUB, AL, REG_HOST_R1, REG_HOST_PC, REG_NOT_USED, 4U);
				ADD_LL_NEXT(new_ins, ins);
#endif
				// ---- End ---------------------------------

#if USE_HOST_MANAGED_BRANCHING
				new_ins = newInstrPUSH(AL, REG_HOST_STM_LR);
				ADD_LL_NEXT(new_ins, ins);

				new_ins = newInstrBL(AL, tgt_address, 1);
				ADD_LL_NEXT(new_ins, ins);

				new_ins = newInstrPOP(AL, REG_HOST_STM_LR);
				ADD_LL_NEXT(new_ins, ins);
#else
				new_ins = newInstrB(MI, tgt_address, 1);
				ADD_LL_NEXT(new_ins, ins);
#endif
			}

			// (4)
			new_ins 	= newInstrI(ARM_ORR,AL, REG_EMU_FLAGS, REG_EMU_FLAGS, REG_NOT_USED, 1U << REG_EMU_FLAG_DS);
			ADD_LL_NEXT(new_ins, ins);

			//if (delayInstruction->instruction > DR_NO_OP)
			{
				// Provide a jump if not branching on LIKELY
				new_ins = newInstrIntB(PL, new_ins);
				ADD_LL_NEXT(new_ins, LikelybranchInstruction);
			}

			return;
		case MIPS_BGEZ:

			// (1)
			if (delayInstruction->instruction > DR_NO_OP)
			{
				ADD_LL_NEXT(delayInstruction, ins);
				ins= ins->nextInstruction;
			}

			InstrI(ins, ARM_CMP, AL, REG_NOT_USED, R1 | REG_WIDE, REG_NOT_USED, 0);

			new_ins = newInstrI(ARM_CMP, PL, REG_NOT_USED, R1 , REG_NOT_USED, 0); // if not minus
			ADD_LL_NEXT(new_ins, ins);

			// (2)
			new_ins 	= newInstrI(ARM_BIC,AL, REG_EMU_FLAGS, REG_EMU_FLAGS, REG_NOT_USED, 1U << REG_EMU_FLAG_DS);
			ADD_LL_NEXT(new_ins, ins);

			// (3) if segment loops on its self
			if (getSegmentAt((size_t)(codeSegment->MIPScode + codeSegment->MIPScodeLen + offset)) == codeSegment)
			{
				new_ins = newInstrIntB(GEZ, codeSegment->Intermcode);
				ADD_LL_NEXT(new_ins, ins);
			}
			else
			{
				getTgtAddress(codeSegment,offset, &tgt_address);

				// ---- Dynamic Recompiler Branching ABI ----

				addLiteral(codeSegment, &base, &offset, (uint32_t)codeSegment);
				new_ins = newInstrI(ARM_LDR_LIT, AL, REG_HOST_R0, REG_NOT_USED, base, offset);
				ADD_LL_NEXT(new_ins, ins);

				new_ins = newInstrI(ARM_SUB, AL, REG_HOST_R1, REG_HOST_PC, REG_NOT_USED, 4U);
				ADD_LL_NEXT(new_ins, ins);

				// ---- End ---------------------------------

				new_ins = newInstrB(GEZ, tgt_address, 1);
				ADD_LL_NEXT(new_ins, ins);
			}

			// (4)
			new_ins 	= newInstrI(ARM_ORR,AL, REG_EMU_FLAGS, REG_EMU_FLAGS, REG_NOT_USED, 1U << REG_EMU_FLAG_DS);
			ADD_LL_NEXT(new_ins, ins);

			return;
		case MIPS_BGEZL:
			ins= ins->nextInstruction;

			InstrI(ins, ARM_CMP, AL, REG_NOT_USED, R1 | REG_WIDE, REG_NOT_USED, 0);

			new_ins = newInstrI(ARM_CMP, PL, REG_NOT_USED, R1 , REG_NOT_USED, 0); // if not minus
			ADD_LL_NEXT(new_ins, ins);

			LikelybranchInstruction = ins;

			// (1)
			if (delayInstruction->instruction > DR_NO_OP)
			{
				ADD_LL_NEXT(delayInstruction, ins);
			}

			// (3) if segment loops on its self
			if (getSegmentAt((size_t)(codeSegment->MIPScode + codeSegment->MIPScodeLen + offset)) == codeSegment)
			{
				new_ins = newInstrIntB(AL, codeSegment->Intermcode);
				ADD_LL_NEXT(new_ins, ins);
			}
			else
			{
				getTgtAddress(codeSegment,offset, &tgt_address);

				// ---- Dynamic Recompiler Branching ABI ----

				addLiteral(codeSegment, &base, &offset, (uint32_t)codeSegment);
				new_ins = newInstrI(ARM_LDR_LIT, AL, REG_HOST_R0, REG_NOT_USED, base, offset);
				ADD_LL_NEXT(new_ins, ins);

				new_ins = newInstrI(ARM_SUB, AL, REG_HOST_R1, REG_HOST_PC, REG_NOT_USED, 4U);
				ADD_LL_NEXT(new_ins, ins);

				// ---- End ---------------------------------

				new_ins = newInstrB(AL, tgt_address, 1);
				ADD_LL_NEXT(new_ins, ins);
			}

			// (4)
			new_ins 	= newInstrI(ARM_ORR,AL, REG_EMU_FLAGS, REG_EMU_FLAGS, REG_NOT_USED, 1U << REG_EMU_FLAG_DS);
			ADD_LL_NEXT(new_ins, ins);

			//if (delayInstruction->instruction > DR_NO_OP)
			{
				// Provide a jump if not branching on LIKELY
				new_ins = newInstrIntB(LT, new_ins);
				ADD_LL_NEXT(new_ins, LikelybranchInstruction);
			}

			return;
		case MIPS_BGEZAL:
			// (1)
			if (delayInstruction->instruction > DR_NO_OP)
			{
				ADD_LL_NEXT(delayInstruction, ins);
				ins= ins->nextInstruction;
			}

			InstrI(ins, ARM_CMP, AL, REG_NOT_USED, R1 | REG_WIDE, REG_NOT_USED, 0);

			new_ins = newInstrI(ARM_CMP, PL, REG_NOT_USED, R1 , REG_NOT_USED, 0); // if not minus
			ADD_LL_NEXT(new_ins, ins);

			// (2)
			new_ins 	= newInstrI(ARM_BIC,AL, REG_EMU_FLAGS, REG_EMU_FLAGS, REG_NOT_USED, 1U << REG_EMU_FLAG_DS);
			ADD_LL_NEXT(new_ins, ins);

			// Store the MIPS PC+8 into REG 31
			addLiteral(codeSegment, &base, &offset, (uint32_t)(codeSegment->MIPScode + codeSegment->MIPScodeLen + 1U));

			// REG 31 is updated regardless of whether the branch is taken
			new_ins = newInstrI(ARM_LDR_LIT, AL, 31U, REG_NOT_USED, base, offset);
			ADD_LL_NEXT(new_ins, ins);

			// (3) if segment loops on its self
			if (getSegmentAt((size_t)(codeSegment->MIPScode + codeSegment->MIPScodeLen + offset)) == codeSegment)
			{
				new_ins = newInstrPUSH(AL, REG_HOST_STM_LR);
				ADD_LL_NEXT(new_ins, ins);

				new_ins = newInstrIntBL(GEZ, codeSegment->Intermcode);

				new_ins = newInstrPOP(AL, REG_HOST_STM_LR);
				ADD_LL_NEXT(new_ins, ins);
			}
			else
			{
				getTgtAddress(codeSegment, offset, &tgt_address);

				// ---- Dynamic Recompiler Branching ABI ----

				addLiteral(codeSegment, &base, &offset, (uint32_t)codeSegment);
				new_ins = newInstrI(ARM_LDR_LIT, AL, REG_HOST_R0, REG_NOT_USED, base, offset);
				ADD_LL_NEXT(new_ins, ins);

#if USE_HOST_MANAGED_BRANCHING
				new_ins = newInstrI(ARM_ADD, AL, REG_HOST_R1, REG_HOST_PC, REG_NOT_USED, 0U);
				ADD_LL_NEXT(new_ins, ins);
#else
				new_ins = newInstrI(ARM_SUB, AL, REG_HOST_R1, REG_HOST_PC, REG_NOT_USED, 4U);
				ADD_LL_NEXT(new_ins, ins);
#endif

				// ---- End ---------------------------------

#if USE_HOST_MANAGED_BRANCHING
				new_ins = newInstrPUSH(AL, REG_HOST_STM_LR);
				ADD_LL_NEXT(new_ins, ins);

				new_ins = newInstrBL(GEZ, tgt_address, 1);
				ADD_LL_NEXT(new_ins, ins);

				new_ins = newInstrPOP(AL, REG_HOST_STM_LR);
				ADD_LL_NEXT(new_ins, ins);
#else
				new_ins = newInstrB(GEZ, tgt_address, 1);
				ADD_LL_NEXT(new_ins, ins);
#endif
			}

			// (4)
			new_ins 	= newInstrI(ARM_ORR,AL, REG_EMU_FLAGS, REG_EMU_FLAGS, REG_NOT_USED, 1U << REG_EMU_FLAG_DS);
			ADD_LL_NEXT(new_ins, ins);

			return;
		case MIPS_BGEZALL:
			ins= ins->nextInstruction;

			InstrI(ins, ARM_CMP, AL, REG_NOT_USED, R1 | REG_WIDE, REG_NOT_USED, 0);

			new_ins = newInstrI(ARM_CMP, PL, REG_NOT_USED, R1 , REG_NOT_USED, 0); // if not minus
			ADD_LL_NEXT(new_ins, ins);

			// Store the MIPS PC+8 into REG 31
			addLiteral(codeSegment, &base, &offset, (uint32_t)(codeSegment->MIPScode + codeSegment->MIPScodeLen + 1U));

			// REG 31 is updated regardless of whether the branch is taken
			new_ins = newInstrI(ARM_LDR_LIT, AL, 31, REG_NOT_USED, base, offset);
			ADD_LL_NEXT(new_ins, ins);
			LikelybranchInstruction = ins;

			// (1)
			if (delayInstruction->instruction > DR_NO_OP)
			{
				ADD_LL_NEXT(delayInstruction, ins);
			}

			// (2)
			new_ins 	= newInstrI(ARM_BIC,AL, REG_EMU_FLAGS, REG_EMU_FLAGS, REG_NOT_USED, 1U << REG_EMU_FLAG_DS);
			ADD_LL_NEXT(new_ins, ins);

			//if segment loops on its self
			if (getSegmentAt((size_t)(codeSegment->MIPScode + codeSegment->MIPScodeLen + offset)) == codeSegment)
			{
#if USE_HOST_MANAGED_BRANCHING
				new_ins = newInstrPUSH(AL, REG_HOST_STM_LR);
				ADD_LL_NEXT(new_ins, ins);

				new_ins = newInstrIntBL(AL, codeSegment->Intermcode);
				ADD_LL_NEXT(new_ins, ins);

				new_ins = newInstrPOP(AL, REG_HOST_STM_LR);
				ADD_LL_NEXT(new_ins, ins);
#else
				new_ins = newInstrIntB(AL, codeSegment->Intermcode);
				ADD_LL_NEXT(new_ins, ins);
#endif
			}
			else
			{
				getTgtAddress(codeSegment, offset, &tgt_address);

				// ---- Dynamic Recompiler Branching ABI ----

				addLiteral(codeSegment, &base, &offset, (uint32_t)codeSegment);
				new_ins = newInstrI(ARM_LDR_LIT, AL, REG_HOST_R0, REG_NOT_USED, base, offset);
				ADD_LL_NEXT(new_ins, ins);

#if USE_HOST_MANAGED_BRANCHING
				new_ins = newInstrI(ARM_ADD, AL, REG_HOST_R1, REG_HOST_PC, REG_NOT_USED, 0U);
				ADD_LL_NEXT(new_ins, ins);
#else
				new_ins = newInstrI(ARM_SUB, AL, REG_HOST_R1, REG_HOST_PC, REG_NOT_USED, 4U);
				ADD_LL_NEXT(new_ins, ins);
#endif

				// ---- End ---------------------------------

#if USE_HOST_MANAGED_BRANCHING
				new_ins = newInstrPUSH(AL, REG_HOST_STM_LR);
				ADD_LL_NEXT(new_ins, ins);

				new_ins = newInstrBL(AL, tgt_address, 1);
				ADD_LL_NEXT(new_ins, ins);

				new_ins = newInstrPOP(AL, REG_HOST_STM_LR);
				ADD_LL_NEXT(new_ins, ins);
#else
				new_ins = newInstrB(GEZ, tgt_address, 1);
				ADD_LL_NEXT(new_ins, ins);
#endif
			}

			// (4)
			new_ins 	= newInstrI(ARM_ORR,AL, REG_EMU_FLAGS, REG_EMU_FLAGS, REG_NOT_USED, 1U << REG_EMU_FLAG_DS);
			ADD_LL_NEXT(new_ins, ins);

			//if (delayInstruction->instruction > DR_NO_OP)
			{
				// Provide a jump if not branching on LIKELY
				new_ins = newInstrIntB(LT, new_ins);
				ADD_LL_NEXT(new_ins, LikelybranchInstruction);
			}
			return;

		case MIPS_BLEZ:
			// (1)
			if (delayInstruction->instruction > DR_NO_OP)
			{
				ADD_LL_NEXT(delayInstruction, ins);
				ins= ins->nextInstruction;
			}

			// (3)
			InstrI(ins, ARM_CMP, AL, REG_NOT_USED, R1 | REG_WIDE, REG_NOT_USED, 0);

			new_ins = newInstrI(ARM_CMP, EQ, REG_NOT_USED, R1 , REG_NOT_USED, 0); // if upper 64 bit is EQ then check lower
			ADD_LL_NEXT(new_ins, ins);

			// (2)
			new_ins 	= newInstrI(ARM_BIC,AL, REG_EMU_FLAGS, REG_EMU_FLAGS, REG_NOT_USED, 1U << REG_EMU_FLAG_DS);
			ADD_LL_NEXT(new_ins, ins);

			// (3) if segment loops on its self
			if (getSegmentAt((size_t)(codeSegment->MIPScode + codeSegment->MIPScodeLen + offset)) == codeSegment)
			{
				new_ins = newInstrIntB(LEZ, codeSegment->Intermcode);
				ADD_LL_NEXT(new_ins, ins);
			}
			else
			{
				getTgtAddress(codeSegment, offset, &tgt_address);

				// ---- Dynamic Recompiler Branching ABI ----

				addLiteral(codeSegment, &base, &offset, (uint32_t)codeSegment);
				new_ins = newInstrI(ARM_LDR_LIT, AL, REG_HOST_R0, REG_NOT_USED, base, offset);
				ADD_LL_NEXT(new_ins, ins);

				new_ins = newInstrI(ARM_SUB, AL, REG_HOST_R1, REG_HOST_PC, REG_NOT_USED, 4U);
				ADD_LL_NEXT(new_ins, ins);

				// ---- End ---------------------------------

				new_ins = newInstrB(LEZ, tgt_address, branchAbsolute);
				ADD_LL_NEXT(new_ins, ins);
			}

			// (4)
			new_ins 	= newInstrI(ARM_ORR,AL, REG_EMU_FLAGS, REG_EMU_FLAGS, REG_NOT_USED, 1U << REG_EMU_FLAG_DS);
			ADD_LL_NEXT(new_ins, ins);

			return;
		case MIPS_BLEZL:
			// Select branch instruction
			ins= ins->nextInstruction;

			InstrI(ins, ARM_CMP, AL, REG_NOT_USED, R1 | REG_WIDE, REG_NOT_USED, 0);

			new_ins = newInstrI(ARM_CMP, EQ, REG_NOT_USED, R1 , REG_NOT_USED, 0); // if upper 64 bit is EQ then check lower
			ADD_LL_NEXT(new_ins, ins);

			LikelybranchInstruction = ins;

			// (1)
			if (delayInstruction->instruction > DR_NO_OP)
			{
				ADD_LL_NEXT(delayInstruction, ins);
			}

			// (2)
			new_ins 	= newInstrI(ARM_BIC,AL, REG_EMU_FLAGS, REG_EMU_FLAGS, REG_NOT_USED, 1U << REG_EMU_FLAG_DS);
			ADD_LL_NEXT(new_ins, ins);

			// (3) if segment loops on its self
			if (getSegmentAt((size_t)(codeSegment->MIPScode + codeSegment->MIPScodeLen + offset)) == codeSegment)
			{
				new_ins = newInstrIntB(LEZ, codeSegment->Intermcode);
				ADD_LL_NEXT(new_ins, ins);
			}
			else
			{
				getTgtAddress(codeSegment, offset, &tgt_address);

				// ---- Dynamic Recompiler Branching ABI ----

				addLiteral(codeSegment, &base, &offset, (uint32_t)codeSegment);
				new_ins = newInstrI(ARM_LDR_LIT, AL, REG_HOST_R0, REG_NOT_USED, base, offset);
				ADD_LL_NEXT(new_ins, ins);

				new_ins = newInstrI(ARM_SUB, AL, REG_HOST_R1, REG_HOST_PC, REG_NOT_USED, 4U);
				ADD_LL_NEXT(new_ins, ins);

				// ---- End ---------------------------------

				new_ins = newInstrB(LEZ, tgt_address, branchAbsolute);
				ADD_LL_NEXT(new_ins, ins);
			}

			// (4)
			new_ins 	= newInstrI(ARM_ORR,AL, REG_EMU_FLAGS, REG_EMU_FLAGS, REG_NOT_USED, 1U << REG_EMU_FLAG_DS);
			ADD_LL_NEXT(new_ins, ins);

			//if (delayInstruction->instruction > DR_NO_OP)
			{
				// Provide a jump if not branching on LIKELY
				new_ins = newInstrIntB(GT, new_ins);
				ADD_LL_NEXT(new_ins, LikelybranchInstruction);
			}
			return;

		case MIPS_BGTZ:
			// (1)
			if (delayInstruction->instruction > DR_NO_OP)
			{
				ADD_LL_NEXT(delayInstruction, ins);
				ins= ins->nextInstruction;
			}

			InstrI(ins, ARM_CMP, AL, REG_NOT_USED, R1 | REG_WIDE, REG_NOT_USED, 0);

			new_ins = newInstrI(ARM_CMP, EQ, REG_NOT_USED, R1 , REG_NOT_USED, 0); // upper is 0 what about lower
			ADD_LL_NEXT(new_ins, ins);

			// (2)
			new_ins 	= newInstrI(ARM_BIC,AL, REG_EMU_FLAGS, REG_EMU_FLAGS, REG_NOT_USED, 1U << REG_EMU_FLAG_DS);
			ADD_LL_NEXT(new_ins, ins);

			// (3) if segment loops on its self
			if (getSegmentAt((size_t)(codeSegment->MIPScode + codeSegment->MIPScodeLen + offset)) == codeSegment)
			{
				new_ins = newInstrIntB(GT, codeSegment->Intermcode);
			}
			else
			{
				getTgtAddress(codeSegment, offset, &tgt_address);

				// ---- Dynamic Recompiler Branching ABI ----

				addLiteral(codeSegment, &base, &offset, (uint32_t)codeSegment);
				new_ins = newInstrI(ARM_LDR_LIT, AL, REG_HOST_R0, REG_NOT_USED, base, offset);
				ADD_LL_NEXT(new_ins, ins);

				new_ins = newInstrI(ARM_SUB, AL, REG_HOST_R1, REG_HOST_PC, REG_NOT_USED, 4U);
				ADD_LL_NEXT(new_ins, ins);

				// ---- End ---------------------------------

				new_ins = newInstrB(GT, tgt_address, branchAbsolute);
			}
			ADD_LL_NEXT(new_ins, ins);

			// (4)
			new_ins 	= newInstrI(ARM_ORR,AL, REG_EMU_FLAGS, REG_EMU_FLAGS, REG_NOT_USED, 1U << REG_EMU_FLAG_DS);
			ADD_LL_NEXT(new_ins, ins);

			break;
		case MIPS_BGTZL:
			// Select branch instruction
			ins= ins->nextInstruction;

			InstrI(ins, ARM_CMP, AL, REG_NOT_USED, R1 | REG_WIDE, REG_NOT_USED, 0);

			new_ins = newInstrI(ARM_CMP, EQ, REG_NOT_USED, R1 , REG_NOT_USED, 0); // upper is 0 what about lower
			ADD_LL_NEXT(new_ins, ins);

			LikelybranchInstruction = ins;

			// (1)
			if (delayInstruction->instruction > DR_NO_OP)
			{
				ADD_LL_NEXT(delayInstruction, ins);
			}

			// (2)
			new_ins 	= newInstrI(ARM_BIC,AL, REG_EMU_FLAGS, REG_EMU_FLAGS, REG_NOT_USED, 1U << REG_EMU_FLAG_DS);
			ADD_LL_NEXT(new_ins, ins);

			// (3) if segment loops on its self
			if (getSegmentAt((size_t)(codeSegment->MIPScode + codeSegment->MIPScodeLen + offset)) == codeSegment)
			{
				new_ins = newInstrIntB(GT, codeSegment->Intermcode);
			}
			else
			{
				getTgtAddress(codeSegment, offset, &tgt_address);

				// ---- Dynamic Recompiler Branching ABI ----

				addLiteral(codeSegment, &base, &offset, (uint32_t)codeSegment);
				new_ins = newInstrI(ARM_LDR_LIT, AL, REG_HOST_R0, REG_NOT_USED, base, offset);
				ADD_LL_NEXT(new_ins, ins);

				new_ins = newInstrI(ARM_SUB, AL, REG_HOST_R1, REG_HOST_PC, REG_NOT_USED, 4U);
				ADD_LL_NEXT(new_ins, ins);

				// ---- End ---------------------------------

				new_ins = newInstrB(GT, tgt_address, branchAbsolute);
			}
			ADD_LL_NEXT(new_ins, ins);

			// (4)
			new_ins 	= newInstrI(ARM_ORR,AL, REG_EMU_FLAGS, REG_EMU_FLAGS, REG_NOT_USED, 1U << REG_EMU_FLAG_DS);
			ADD_LL_NEXT(new_ins, ins);

			//if (delayInstruction->instruction > DR_NO_OP)
			{
				// Provide a jump if not branching on LIKELY
				new_ins = newInstrIntB(LE, new_ins);
				ADD_LL_NEXT(new_ins, LikelybranchInstruction);
			}

			return;

		case MIPS_BC1F:
		case MIPS_BC1T:
		case MIPS_BC1FL:
		case MIPS_BC1TL:
			TRANSLATE_ABORT();
			break;

		default:
			break;
		}

		ins = ins->nextInstruction;
		instructionCount++;
	}
}
