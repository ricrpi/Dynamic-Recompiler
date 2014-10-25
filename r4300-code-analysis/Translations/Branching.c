/*
 * Branching.c
 *
 *  Created on: 23 Oct 2014
 *      Author: rjhender
 */

#include "Translate.h"
#include "InstructionSet_ascii.h"
#include "memory.h"

void Translate_Branch(code_seg_t* const codeSegment)
{
	code_seg_t* 	BranchToSeg;
	Instruction_t*	ins;
	Instruction_t*	new_ins;
	int32_t 		offset;
	size_t 			tgt_address;
	uint8_t 		branchAbsolute 		= 1;
	uint32_t 		instructionCount 	= 0;

	ins = 			codeSegment->Intermcode;

	while (ins)
	{
		switch (ins->instruction)
		{
			case BEQ:
			case BEQL:
				offset = ins->offset;
				if (0 == ins->R2.regID)
				{
					InstrI(ins, ARM_CMP, AL, REG_NOT_USED, ins->R1.regID, REG_NOT_USED, 0);
				}
				else
				{
					Instr(ins, ARM_CMP, AL, REG_NOT_USED, ins->R1.regID, ins->R2.regID);
				}

				BranchToSeg = getSegmentAt(codeSegment->MIPScode + codeSegment->MIPScodeLen + offset);

				if (BranchToSeg)
				{
					if (BranchToSeg == codeSegment)	//loops to self
					{
						tgt_address = -instructionCount -1;
						branchAbsolute = 0;
					}
					else if (BranchToSeg->ARMEntryPoint)
					{
						tgt_address = (size_t)BranchToSeg->ARMEntryPoint;
						branchAbsolute = 1;
					}
					else
					{
						tgt_address = *((size_t*)(MMAP_FP_BASE + FUNC_GEN_BRANCH_UNKNOWN));
						branchAbsolute = 1;
					}
				}
				else // No segment Found
				{
					tgt_address = *((size_t*)(MMAP_FP_BASE + FUNC_GEN_BRANCH_UNKNOWN));
					branchAbsolute = 1;
				}

				new_ins = newInstrB(EQ, tgt_address, branchAbsolute);
				ADD_LL_NEXT(new_ins, ins);

				break;
			case BNE:
			case BNEL:
				offset = ins->offset;
				if (0 == ins->R2.regID)
				{
					InstrI(ins, ARM_CMP, AL, REG_NOT_USED, ins->R1.regID, REG_NOT_USED, 0);
				}
				else
				{
					Instr(ins, ARM_CMP, AL, REG_NOT_USED, ins->R1.regID, ins->R2.regID);
				}

				BranchToSeg = getSegmentAt(codeSegment->MIPScode + codeSegment->MIPScodeLen + offset);

				if (BranchToSeg)
				{
					if (BranchToSeg == codeSegment)	//loops to self
					{
						tgt_address = -instructionCount -1;
						branchAbsolute = 0;
					}
					else if (BranchToSeg->ARMEntryPoint)
					{
						tgt_address = (size_t)BranchToSeg->ARMEntryPoint;
						branchAbsolute = 1;
					}
					else
					{
						tgt_address = *((size_t*)(MMAP_FP_BASE + FUNC_GEN_BRANCH_UNKNOWN));
						branchAbsolute = 1;
					}
				}
				else // No segment Found
				{
					tgt_address = *((size_t*)(MMAP_FP_BASE + FUNC_GEN_BRANCH_UNKNOWN));
					branchAbsolute = 1;
				}

				new_ins = newInstrB(NE, tgt_address, branchAbsolute);
				ADD_LL_NEXT(new_ins, ins);

				break;
			case J:
			case JAL:
				offset = ins->offset;

				BranchToSeg = getSegmentAt(codeSegment->MIPScode + codeSegment->MIPScodeLen + offset);

				if (BranchToSeg)
				{
					if (BranchToSeg == codeSegment)	//loops to self
					{
						tgt_address = -instructionCount -1;
						branchAbsolute = 0;
					}
					else if (BranchToSeg->ARMEntryPoint)
					{
						tgt_address = (size_t)BranchToSeg->ARMEntryPoint;
						branchAbsolute = 1;
					}
					else
					{
						tgt_address = *((size_t*)(MMAP_FP_BASE + FUNC_GEN_BRANCH_UNKNOWN));
						branchAbsolute = 1;
					}
				}
				else // No segment Found
				{
					tgt_address = *((size_t*)(MMAP_FP_BASE + FUNC_GEN_BRANCH_UNKNOWN));
					branchAbsolute = 1;
				}

				new_ins = newInstrB(AL, tgt_address, branchAbsolute);
				ADD_LL_NEXT(new_ins, ins);
				break;
			case JR:
			case JALR:
				// we need to lookup the code segment we should be branching to according to the value in the register
				// work out address in code segment lookup table

				// TODO how do we handle static/dynamic segment lookups?

				//new_ins = newInstrI(ARM_MOV, AL, REG_TEMP_JR1, REG_NOT_USED, MMAP_CODE_SEG_BASE);
				//ADD_LL_NEXT(new_ins, ins);

				//new_ins = newInstrI(ARM_ADD, AL, REG_TEMP_JR1, REG_TEMP_JR1, MMAP_CODE_SEG_BASE);
				//ADD_LL_NEXT(new_ins, ins);


				break;
			case BLTZ:
			case BGEZ:
			case BLEZ:
			case BGTZ:

			case BLTZL:
			case BGEZL:

			case BLEZL:
			case BGTZL:

			case BLTZAL:
			case BGEZAL:
			case BLTZALL:
			case BGEZALL:

			case BC1F:
			case BC1T:
			case BC1FL:
			case BC1TL:
				printf("Cannot handle this type of branch '%s' yet in Translate.c:%d\n", Instruction_ascii[STRIP(ins->instruction)], __LINE__);

				break;

		default:
			break;
		}

		ins = ins->nextInstruction;
		instructionCount++;
	}
}
