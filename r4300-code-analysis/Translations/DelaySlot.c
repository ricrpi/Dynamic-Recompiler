/*
 * DelaySlot.c
 *
 *  Created on: 23 Oct 2014
 *      Author: rjhender
 */

#include "Translate.h"
#include "InstructionSetMIPS4.h"

/*
 * MIPS4300 executes instruction after a branch or jump if it is not LINK_LIKELY
 * Therefore the instruction after the branch may need to be moved into the segment.
 *
 * I wish to try using a different approach as I do not want to have to handle special cases
 * with delay slot re-shuffling.
 *
 * The recompiler design frees up host registers by the end of a code segment.
 * Therefore we can borrow one of these to record the delay slot status.
 *
 * We cannot use the PSR as it may change if we branch+link away from current segment
 *
 * Example 1: Loop then continue
 *
 * ...								...
 * .L1								.L1
 * ADD 	R1	R2	R3					ADD		R1	R2	R3
 * SUBI	R1	R1	#4					SUMI	R1	R1	#4
 * BNE	R1		.L1
 * ====New Segment====		==>	  + ADDI	R2	R2 	#3		// the condition on the branch
 * ADDI	R2	R2	#3				  + bne		.L1
 * ...							  + bfi		Rx      #y
 * ...							    ====New Segment====		// Will always be a BLOCK_START_CONT Segment
 * ...								tst     Rx      #1		// if its not then don't even need to test.
 * ...                              ADDIeq	R2	R2	#3		// the delaySlot instruction(s)
 * ...								...
 *
 * Example 2:	Always branch/jump then continue
 *
 * ...								...
 * .L1								.L1
 * ADD 	R1	R2	R3					ADD		R1	R2	R3
 * SUBI	R1	R1	#4					SUMI	R1	R1	#4		// x = REG_EMU_FP
 * JAL		.L1					  + ADDI	R2	R2 	#3		// y = REG_EMU_FLAG_DS
 * 								  + bfc		Rx      #y  	// if jumping to a BLOCK_START_CONT add this instruction
 * ====New Segment====		==>	  	JAL				.L1		// do the jump and link
 * ADDI	R2	R2	#3				  + bfi     Rx      #y		// we have already done the next instruction...
 * ...							  	====New Segment====		// Will always be a BLOCK_START_CONT Segment
 *                                  tst     Rx      #(1<<y)	// now test Rx
 * ...							  + ADDIeq	R2	R2	#3		// the delaySlot instruction(s)
 * ...								...
 *
 *
 * Requirements:
 *
 * 1. Segments that have code preceeding it must set the first instruction to be conditional and include a test
 *
 * 2. All segments must set REG_EMU_FLAG_DS flag before jumping to a BLOCK_START_CONT segment.
 *
 * 3. All segments that 'continue' after a branch must set REG_EMU_FLAG_DS flag.
 *    i.e. only segments that don't jump
 *
 * 4. Segments must include the MIPS instruction immediately after the end of the segment MIPS code if they branch.
 *    This could be a no-op in which case we can ignore it.
 *
 *
 */
void Translate_DelaySlot(code_seg_t*  codeSegment)
{
#if defined(USE_INSTRUCTION_COMMENTS)
	currentTranslation = "DelaySlot";
#endif
	Instruction_e ops = ops_type(*(codeSegment->MIPScode + codeSegment->MIPScodeLen -1));

	Instruction_t* delayInstruction 	= NULL;
	Instruction_t* newInstruction 		= NULL;

	Instruction_t* ins 					= codeSegment->Intermcode;

	if (ins == NULL)
		{
			printf("Not initialized this code segment. Please run 'translate intermediate'\n");
			return;
		}

	// if this is a SEG_SANDWICH or SEG_END then we need to be conditional on the first instruction (1)
	// unless first instruction is a no op
	if ( (codeSegment->Type == SEG_SANDWICH || codeSegment->Type == SEG_END)
			&& (ops_type(*(codeSegment->MIPScode -1)) & (OPS_BRANCH | OPS_LINK))
			&& ops_type(*(codeSegment->MIPScode)) > NO_OP)
	{
		newInstruction 	= newInstrI(ARM_TST, AL, REG_NOT_USED, REG_EMU_FLAGS, REG_NOT_USED, 1 << REG_EMU_FLAG_DS);
		newInstruction->nextInstruction = ins;
		codeSegment->Intermcode = newInstruction;

		ins->cond = EQ;
	}

	Instruction_e following_op = ops_type(*(codeSegment->MIPScode + codeSegment->MIPScodeLen));

	// clear the REG_EMU_FLAG_DS flag before branch / jump so the jumped-to segment
	// performs the first MIPS instruction (2)
	if (codeSegment->pBranchNext
			&& (codeSegment->pBranchNext->Type == SEG_SANDWICH
					|| codeSegment->pBranchNext->Type == SEG_END))
	{
		if (NULL == ins->nextInstruction)
		{
			printf("Don't know what to do in Translate.c:%d\n", __LINE__);
			// this would imply the branch/jump is the first instruction.
			// we could change codeSegment->Intermcode but think this
			// is a bigger problem as a jump will be in a delay slot and pulled
			// into previous code segment before its final branch.

			abort();
		}else
		{
			while (ins->nextInstruction->nextInstruction) ins = ins->nextInstruction;
		}
		newInstruction 	= newInstrI(ARM_BIC,AL, REG_EMU_FLAGS, REG_EMU_FLAGS, REG_NOT_USED, REG_EMU_FLAG_DS);
		ADD_LL_NEXT(newInstruction, ins);
	}


	// if the last instruction is not likely and the following instruction is not a NO OP (4)
	if ((ops & (OPS_BRANCH | OPS_JUMP))
			&& !(ops & OPS_LIKELY)
			&& following_op > NO_OP)
	{
		ins = codeSegment->Intermcode;
		delayInstruction = newEmptyInstr();

		//generate the instruction to add.
		mips_decode(*(codeSegment->MIPScode + codeSegment->MIPScodeLen), delayInstruction);

		//Is this is a one instruction segment?
		if (NULL == codeSegment->Intermcode->nextInstruction)
		{
			delayInstruction->nextInstruction = codeSegment->Intermcode;
			codeSegment->Intermcode = delayInstruction;
		}
		else
		{
			while (ins->nextInstruction->nextInstruction) ins = ins->nextInstruction;
			ADD_LL_NEXT(delayInstruction, ins);		//ins will be pointing to newInstruction
		}
	}

	// if the instruction continues then set the REG_EMU_FLAG_DS flag at end of instructions (3)
	if ((ops & OPS_BRANCH)
			&& following_op > NO_OP)
	{
		//goto last instruction (the branch instruction)
		while (ins->nextInstruction) ins = ins->nextInstruction;

		newInstruction 	= newInstrI(ARM_BIC,AL, REG_EMU_FLAGS, REG_EMU_FLAGS, REG_NOT_USED, REG_EMU_FLAG_DS);
		ADD_LL_NEXT(newInstruction, ins);
	}
}
