/*
 * Translate.c
 *
 *  Created on: 16 Apr 2014
 *      Author: rjhender
 */

#include "CodeSegments.h"
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

#define ADD_LL_NEXT(x, y) (x)->nextInstruction = (y)->nextInstruction; \
			(y)->nextInstruction = (x); \
			(y) = (y)->nextInstruction;

#define ADD_LL(x, y) (x)->nextInstruction = (y)->nextInstruction; \
			(y)->nextInstruction = (x);

uint32_t bCountSaturates = 0;
uint32_t uiCountFrequency = 40;	// must be less than 128 else may not be able to encode in imm8
uint32_t bMemoryInlineLookup = 0;
uint32_t bMemoryOffsetCheck = 0;
uint32_t bDoDMAonInterrupt = 1;
uint8_t uMemoryBase = 0x80;


//=============================================================

static Instruction_t* insertCall_To_C(code_seg_t* const code_seg, Instruction_t* ins, const Condition_e cond, uint32_t functionAddress)
{
	Instruction_t* newInstruction;
	regID_t base;
	int32_t offset;

	addLiteral(code_seg, &base, &offset, functionAddress);

	newInstruction 		= newInstrI(ARM_LDR_LIT, AL, REG_TEMP_CALL2C, REG_NOT_USED, base, offset);
	ADD_LL_NEXT(newInstruction, ins);

	//push lr
	newInstruction 	= newInstrPUSH(AL, REG_HOST_STM_GENERAL);
	ADD_LL_NEXT(newInstruction, ins);

	//set lr
	newInstruction 	= newInstrI(ADD, AL, REG_HOST_LR, REG_HOST_PC, REG_NOT_USED, 4);
	ADD_LL_NEXT(newInstruction, ins);

	// load function address from [fp + offset] into PC
	newInstruction 	= newInstr(ARM_MOV, cond, REG_HOST_PC, REG_NOT_USED, REG_TEMP_CALL2C);
	ADD_LL_NEXT(newInstruction, ins);

	// pop lr
	newInstruction 	= newInstrPOP(AL, REG_HOST_STM_GENERAL);
	ADD_LL_NEXT(newInstruction, ins);

	return ins;
}

static int32_t FindRegNextUsedAgain(const Instruction_t* const ins, const regID_t Reg)
{
	const Instruction_t* in = ins;
	uint32_t x = 0;

	while (in)
	{
		if (in->R1.regID == Reg || in->R2.regID == Reg || in->R3.regID == Reg)
			return x;
		if (x && (in->Rd1.regID == Reg || in->Rd2.regID == Reg))
			return -1;

		x++;
		in = in->nextInstruction;
	}

	return 0;
}

static void UpdateRegWithReg(Instruction_t* const ins, const regID_t RegFrom, const regID_t RegTo, uint32_t MaxInstructions)
{
	Instruction_t* in = ins;
	uint32_t x = MaxInstructions;

	if (!x) x = 0xffffffff;

#if 0
	if (RegFrom >= REG_HOST)
	{
		if (RegTo >= REG_HOST) 	printf("Reg host %3d => host %3d\n", RegFrom-REG_HOST, RegTo-REG_HOST);
		else					printf("Reg host %3d =>      %3d\n", RegFrom-REG_HOST, RegTo);
	}
	else if (RegFrom >= REG_TEMP)
	{
		if (RegTo >= REG_HOST) 	printf("Reg temp %3d => host %3d\n", RegFrom-REG_TEMP, RegTo-REG_HOST);
		else					printf("Reg temp %3d =>      %3d\n", RegFrom-REG_TEMP, RegTo);
	}
	else if (RegFrom >= REG_CO)
	{
		if (RegTo >= REG_HOST) 	printf("Reg   co %3d => host %3d\n", RegFrom-REG_CO, RegTo-REG_HOST);
		else					printf("Reg   co %3d =>      %3d\n", RegFrom-REG_CO, RegTo);
	}
	else if (RegFrom >= REG_WIDE)
	{
		if (RegTo >= REG_HOST) 	printf("Reg wide %3d => host %3d\n", RegFrom-REG_WIDE, RegTo-REG_HOST);
		else					printf("Reg wide %3d =>      %3d\n", RegFrom-REG_WIDE, RegTo);
	}
	else
	{
		if (RegTo >= REG_HOST) 	printf("Reg      %3d => host %3d\n", RegFrom, RegTo-REG_HOST);
		else					printf("Reg      %3d =>      %3d\n", RegFrom, RegTo);
	}
#endif

	while (x && in)
	{
		if (in->Rd1.regID == RegFrom && in->Rd1.state == RS_REGISTER) in->Rd1.regID = RegTo;
		if (in->Rd2.regID == RegFrom && in->Rd2.state == RS_REGISTER) in->Rd2.regID = RegTo;
		if (in->R1.regID == RegFrom && in->R1.state == RS_REGISTER) in->R1.regID = RegTo;
		if (in->R2.regID == RegFrom && in->R2.state == RS_REGISTER) in->R2.regID = RegTo;
		if (in->R3.regID == RegFrom && in->R3.state == RS_REGISTER) in->R3.regID = RegTo;
		x--;
		in = in->nextInstruction;
	}
}

//=============================================================



#if 0
/*
 * Code to generate Emulation-time Code within 32MB of dynamically compiled code.
 *
 * When a Code segment is written into memory, it may not be known where it branches to. In which case
 * It will be necessary to branch to this Stub code.
 *
 * Requirements:
 * 	The address of the branch instruction to patch
 * 	The tgt address/offset to branch to
 *
 * Method:
 *
 * 	The address of the instruction to patch is LR-4
 *
 * 	If we know what the current segment is then we can lookup the target address/offset from the raw MIPS code
 *
 */
Instruction_t* Generate_BranchStubCode()
{
	Instruction_t* firstInstruction;
	Instruction_t* newInstruction;

	firstInstruction 	= newInstrPUSH(AL, REG_HOST_STM_LR);

	newInstruction 		= newInstr(ADD, AL, REG_HOST_LR, REG_HOST_PC, REG_NOT_USED, 4);

	firstInstruction->nextInstruction = newInstruction;

	newInstruction 		= newInstrPOP(AL, REG_HOST_STM_PC);

	return firstInstruction;

}
#endif


/*
 * Function to generate Emulation-time code for calculating the Hosts memory address to use.
 *
 * Emulation Args:
 * 			R0 base
 * 			R1 offset
 *
 * Emulation Returns:
 *  		R0 Host memory address
 *			Z=1 if !bDoDMAonInterrupt && last byte is 0x5, so run DMA?
 */
code_seg_t* Generate_MemoryTranslationCode(code_segment_data_t* seg_data, pfu1ru1 f)
{
	code_seg_t* 	code_seg 		= newSegment();
	Instruction_t* 	newInstruction;
	Instruction_t* 	ins 			= NULL;

	if (bMemoryInlineLookup){
		printf("Generate_MemoryTranslationCode() and bMemoryInlineLookup\n");
		return NULL;
	}

	//Add the base and offset together
	newInstruction 		= newInstr(ARM_ADD, AL, REG_TEMP_MEM1, REG_HOST_R0, REG_HOST_R1);
	code_seg->Intermcode = ins = newInstruction;

	//shift Right so that we have the final Byte
	newInstruction 		= newInstr(SRL, AL, REG_TEMP_MEM2, REG_TEMP_MEM1, REG_NOT_USED);
	newInstruction->I = 1;
	newInstruction->shift = 24;
	ADD_LL_NEXT(newInstruction, ins);

	//If the final Byte is 0x80 then all is good
	newInstruction 		= newInstrI(ARM_CMP, AL, REG_NOT_USED, REG_TEMP_MEM2, REG_NOT_USED, 0x80);
	newInstruction->I = 1;
	ADD_LL_NEXT(newInstruction, ins);

	//Move the address to R0
	newInstruction 		= newInstr(ARM_MOV, EQ, REG_HOST_R0, REG_NOT_USED, REG_TEMP_MEM1);
	ADD_LL_NEXT(newInstruction, ins);

	// Return
	newInstruction 		= newInstr(ARM_MOV, EQ, REG_HOST_PC, REG_NOT_USED, REG_HOST_LR);
	ADD_LL_NEXT(newInstruction, ins);

	//Else is the final byte 0xA0?
	newInstruction 		= newInstrI(ARM_CMP, AL, REG_NOT_USED, REG_TEMP_MEM2, REG_NOT_USED, 0xA0);
	newInstruction->I = 1;
	ADD_LL_NEXT(newInstruction, ins);

	//If so then clear bit 0x40
	newInstruction 		= newInstrI(ARM_BIC, EQ, REG_HOST_R0, REG_TEMP_MEM1, REG_NOT_USED, 0x40);
	newInstruction->I = 1;
	newInstruction->shift = 24;
	newInstruction->shiftType = LOGICAL_LEFT;
	ADD_LL_NEXT(newInstruction, ins);

	// Return
	newInstruction 		= newInstr(ARM_MOV, EQ, REG_HOST_PC, REG_NOT_USED, REG_HOST_LR);
	ADD_LL_NEXT(newInstruction, ins);

	//The address must be Virtual

	newInstruction 		= newInstrPUSH(AL, REG_HOST_STM_EABI | REG_HOST_STM_LR);
	ADD_LL_NEXT(newInstruction, ins);

	//TODO call C function for Lookup?
	newInstruction 		= newInstr(ARM_MOV, EQ, REG_HOST_LR, REG_NOT_USED, REG_HOST_PC);
	//The literal !!!
	newInstruction 		= newInstr(ARM_LDR_LIT, EQ, REG_HOST_PC, REG_HOST_PC, REG_NOT_USED);

	newInstruction 		= newInstrPOP(AL, REG_HOST_STM_EABI| REG_HOST_STM_LR);
	ADD_LL_NEXT(newInstruction, ins);

	//Return
	newInstruction 		= newInstr(ARM_MOV, AL, REG_HOST_PC, REG_NOT_USED, REG_HOST_LR);
	ADD_LL_NEXT(newInstruction, ins);

	Translate_Registers(code_seg);

	return code_seg;
}


#if defined(TEST_BRANCH_TO_C)
uint32_t test_callCode(uint32_t r0)
{
	return r0 + 0x1000;
}
#endif
/*
 * Function Called to Begin Running Dynamic compiled code.
 */
code_seg_t* Generate_CodeStart(code_segment_data_t* seg_data)
{
	code_seg_t* 	code_seg 		= newSegment();
	Instruction_t* 	newInstruction;
	Instruction_t* 	ins 			= NULL;

	seg_data->dbgCurrentSegment = code_seg;

	code_seg->Type = SEG_START;

	newInstruction 		= newInstrPUSH(AL, REG_HOST_STM_EABI2 );
	code_seg->Intermcode = ins = newInstruction;

	regID_t base;
	int32_t offset;

#if defined(TEST_BRANCHING_FORWARD)
	newInstruction 		= newInstrI(ARM_MOV, AL, REG_HOST_R0, REG_NOT_USED, REG_NOT_USED, 0);
	ADD_LL_NEXT(newInstruction, ins);

	newInstruction 		= newInstrI(ARM_B, AL, REG_NOT_USED, REG_NOT_USED, REG_NOT_USED, 3);
	ADD_LL_NEXT(newInstruction, ins);

	newInstruction 		= newInstrI(ARM_ADD, AL, REG_HOST_R0, REG_HOST_R0, REG_NOT_USED,0x1);
	ADD_LL_NEXT(newInstruction, ins);

	newInstruction 		= newInstrI(ARM_ADD, AL, REG_HOST_R0, REG_HOST_R0, REG_NOT_USED,0x2);
	ADD_LL_NEXT(newInstruction, ins);

	newInstruction 		= newInstrI(ARM_ADD, AL, REG_HOST_R0, REG_HOST_R0, REG_NOT_USED,0x4);
	ADD_LL_NEXT(newInstruction, ins);

	newInstruction 		= newInstrI(ARM_ADD, AL, REG_HOST_R0, REG_HOST_R0, REG_NOT_USED,0x8);
	ADD_LL_NEXT(newInstruction, ins);

	newInstruction 		= newInstrI(ARM_ADD, AL, REG_HOST_R0, REG_HOST_R0, REG_NOT_USED,0x10);
	ADD_LL_NEXT(newInstruction, ins);

	newInstruction 		= newInstrI(ARM_ADD, AL, REG_HOST_R0, REG_HOST_R0, REG_NOT_USED,0x20);
	ADD_LL_NEXT(newInstruction, ins);

	newInstruction 		= newInstrI(ARM_ADD, AL, REG_HOST_R0, REG_HOST_R0, REG_NOT_USED,0x40);
	ADD_LL_NEXT(newInstruction, ins);

	// return back to debugger
	newInstruction 		= newInstr(ARM_MOV, AL, REG_HOST_PC, REG_NOT_USED, REG_HOST_LR);
	ADD_LL_NEXT(newInstruction, ins);

#elif defined(TEST_BRANCHING_BACKWARD)

// Jump forwards to the Landing Pad
	newInstruction 		= newInstrI(ARM_MOV, AL, REG_HOST_R0, REG_NOT_USED, REG_NOT_USED, 0);
	ADD_LL_NEXT(newInstruction, ins);

	newInstruction 		= newInstrI(ARM_B, AL, REG_NOT_USED, REG_NOT_USED, REG_NOT_USED, 10);
	ADD_LL_NEXT(newInstruction, ins);

	newInstruction 		= newInstrI(ARM_ADD, AL, REG_HOST_R0, REG_HOST_R0, REG_NOT_USED,0x1);
	ADD_LL_NEXT(newInstruction, ins);

	newInstruction 		= newInstrI(ARM_ADD, AL, REG_HOST_R0, REG_HOST_R0, REG_NOT_USED,0x2);
	ADD_LL_NEXT(newInstruction, ins);

	newInstruction 		= newInstrI(ARM_ADD, AL, REG_HOST_R0, REG_HOST_R0, REG_NOT_USED,0x4);
	ADD_LL_NEXT(newInstruction, ins);

	newInstruction 		= newInstrI(ARM_ADD, AL, REG_HOST_R0, REG_HOST_R0, REG_NOT_USED,0x8);
	ADD_LL_NEXT(newInstruction, ins);

	newInstruction 		= newInstrI(ARM_ADD, AL, REG_HOST_R0, REG_HOST_R0, REG_NOT_USED,0x10);
	ADD_LL_NEXT(newInstruction, ins);

	newInstruction 		= newInstrI(ARM_ADD, AL, REG_HOST_R0, REG_HOST_R0, REG_NOT_USED,0x20);
	ADD_LL_NEXT(newInstruction, ins);

	newInstruction 		= newInstrI(ARM_ADD, AL, REG_HOST_R0, REG_HOST_R0, REG_NOT_USED,0x40);
	ADD_LL_NEXT(newInstruction, ins);

// return back to debugger
	newInstruction 		= newInstr(ARM_MOV, AL, REG_HOST_PC, REG_NOT_USED, REG_HOST_LR);
	ADD_LL_NEXT(newInstruction, ins);

// Landing pad
	newInstruction 		= newInstrI(ARM_ADD, AL, REG_HOST_R0, REG_HOST_R0, REG_NOT_USED,0);
	ADD_LL_NEXT(newInstruction, ins);

	newInstruction 		= newInstrI(ARM_ADD, AL, REG_HOST_R0, REG_HOST_R0, REG_NOT_USED,0);
	ADD_LL_NEXT(newInstruction, ins);

	newInstruction 		= newInstrI(ARM_ADD, AL, REG_HOST_R0, REG_HOST_R0, REG_NOT_USED,0);
	ADD_LL_NEXT(newInstruction, ins);

	newInstruction 		= newInstrI(ARM_ADD, AL, REG_HOST_R0, REG_HOST_R0, REG_NOT_USED,0);
	ADD_LL_NEXT(newInstruction, ins);

// Now jump backwards
	newInstruction 		= newInstrI(ARM_B, AL, REG_NOT_USED, REG_NOT_USED, REG_NOT_USED, -10);
	ADD_LL_NEXT(newInstruction, ins);

#elif defined(TEST_BRANCH_TO_C)

	newInstruction 		= newInstrI(ARM_MOV, AL, REG_HOST_R0, REG_NOT_USED, REG_NOT_USED, 0);
	ADD_LL_NEXT(newInstruction, ins);

	addLiteral(code_seg, &base, &offset,(uint32_t)&test_callCode);
	assert(base == REG_HOST_PC);

	newInstruction 		= newInstrI(ARM_LDR_LIT, AL, REG_HOST_R1, REG_NOT_USED, base, offset);
	ADD_LL_NEXT(newInstruction, ins);

	newInstruction 		= newInstrI(ARM_ADD, AL, REG_HOST_LR, REG_HOST_PC, REG_NOT_USED, 8);
	ADD_LL_NEXT(newInstruction, ins);

	newInstruction 		= newInstr(ARM_MOV, AL, REG_HOST_PC, REG_NOT_USED, REG_HOST_R1);
	ADD_LL_NEXT(newInstruction, ins);

	// Landing Pad
	newInstruction 		= newInstrI(ARM_ADD, AL, REG_HOST_R0, REG_HOST_R0, REG_NOT_USED, 0x1);
	ADD_LL_NEXT(newInstruction, ins);

	newInstruction 		= newInstrI(ARM_ADD, AL, REG_HOST_R0, REG_HOST_R0, REG_NOT_USED, 0x2);
	ADD_LL_NEXT(newInstruction, ins);

	// Return back to Debugger
	newInstruction 		= newInstr(ARM_MOV, AL, REG_HOST_PC, REG_NOT_USED, REG_HOST_LR);
	ADD_LL_NEXT(newInstruction, ins);

#elif defined(TEST_LITERAL)	// Test Literal loading

	addLiteral(code_seg, &base, &offset,(uint32_t)MMAP_FP_BASE);
	assert(base == REG_HOST_PC);

	newInstruction 		= newInstrI(ARM_LDR_LIT, AL, REG_HOST_R0, REG_NOT_USED, base, offset);
	ADD_LL_NEXT(newInstruction, ins);

	// return back to debugger
	newInstruction 		= newInstr(ARM_MOV, AL, REG_HOST_PC, REG_NOT_USED, REG_HOST_LR);
	ADD_LL_NEXT(newInstruction, ins);
#else
	addLiteral(code_seg, &base, &offset,(uint32_t)MMAP_FP_BASE);
	assert(base == REG_HOST_PC);

	// setup the HOST_FP
	newInstruction 		= newInstrI(ARM_LDR_LIT, AL, REG_HOST_FP, REG_NOT_USED, base, offset);
	ADD_LL_NEXT(newInstruction, ins);

	// start executing recompiled code
	newInstruction 		= newInstrI(ARM_LDR_LIT, AL, REG_HOST_PC, REG_NOT_USED, REG_HOST_FP, RECOMPILED_CODE_START);
	ADD_LL_NEXT(newInstruction, ins);
#endif

	Translate_Registers(code_seg);
	Translate_Literals(code_seg);

	return code_seg;
}

code_seg_t* Generate_CodeStop(code_segment_data_t* seg_data)
{
	code_seg_t* 	code_seg 		= newSegment();
	Instruction_t* 	newInstruction;
	Instruction_t* 	ins 			= NULL;

	seg_data->dbgCurrentSegment = code_seg;

	newInstruction 		= newInstrPOP(AL, REG_HOST_STM_EABI2 );
	code_seg->Intermcode = ins = newInstruction;

	newInstruction 		= newInstrI(ARM_MOV, AL, REG_HOST_R0, REG_NOT_USED, REG_NOT_USED, 0);
	ADD_LL_NEXT(newInstruction, ins);

	// Return
	newInstruction 		= newInstr(ARM_MOV, AL, REG_HOST_PC, REG_NOT_USED, REG_HOST_LR);
	ADD_LL_NEXT(newInstruction, ins);

	Translate_Registers(code_seg);

	return code_seg;
}

/*
 * This is equivalent to cc_interupt() in new_dynarec
 *
 */
code_seg_t* Generate_ISR(code_segment_data_t* seg_data)
{
	code_seg_t* 	code_seg 		= newSegment();
	Instruction_t* 	newInstruction;
	Instruction_t* 	ins 			= NULL;

	seg_data->dbgCurrentSegment = code_seg;

	// need to test if interrupts are enabled (Status Register bit 0)
	newInstruction 		= newInstrI(ARM_TST, AL, REG_NOT_USED, REG_STATUS, REG_NOT_USED, 0x01);
	code_seg->Intermcode = ins = newInstruction;

	// Return
	newInstruction 		= newInstr(ARM_MOV, EQ, REG_HOST_PC, REG_NOT_USED, REG_HOST_LR);
	ADD_LL_NEXT(newInstruction, ins);

	// Call interrupt C function
	//insertCall_To_C(ins, AL, FUNC_GEN_INTERRUPT);

	// Return
	newInstruction 		= newInstr(ARM_MOV, AL, REG_HOST_PC, REG_NOT_USED, REG_HOST_LR);
	ADD_LL_NEXT(newInstruction, ins);

	Translate_Registers(code_seg);

	return code_seg;
}

code_seg_t* Generate_BranchUnknown(code_segment_data_t* seg_data)
{
	code_seg_t* 	code_seg 		= newSegment();
	Instruction_t* 	newInstruction;
	Instruction_t* 	ins 			= NULL;

	seg_data->dbgCurrentSegment = code_seg;

	// Return
	newInstruction 		= newInstr(ARM_MOV, AL, REG_HOST_PC, REG_NOT_USED, REG_HOST_LR);
	code_seg->Intermcode = ins = newInstruction;

	Translate_Registers(code_seg);

	return code_seg;
}

code_seg_t* Generate_MIPS_Trap(code_segment_data_t* seg_data)
{
	code_seg_t* 	code_seg 		= newSegment();
	Instruction_t* 	newInstruction;
	Instruction_t* 	ins 			= NULL;

	seg_data->dbgCurrentSegment = code_seg;

	// Return
	newInstruction 		= newInstr(ARM_MOV, AL, REG_HOST_PC, REG_NOT_USED, REG_HOST_LR);
	code_seg->Intermcode = ins = newInstruction;

	Translate_Registers(code_seg);

	return code_seg;
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
		if (0 != *(codeSegment->MIPScode + x))
		{
			newInstruction = newEmptyInstr();

			mips_decode(*(codeSegment->MIPScode + x), newInstruction);

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
		newInstruction 	= newInstrI(ARM_BFC,AL, REG_NOT_USED, REG_NOT_USED, REG_NOT_USED, REG_EMU_FLAG_DS);
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

		newInstruction 	= newInstrI(ARM_BFI,AL, REG_EMU_FLAGS, REG_NOT_USED, REG_NOT_USED, REG_EMU_FLAG_DS);
		ADD_LL_NEXT(newInstruction, ins);
	}
}

/*
 * MIPS4300 has a COUNT register that is decremented every instruction
 * when it underflows, an interrupt is triggered.
 */
void Translate_CountRegister(code_seg_t* const codeSegment)
{
	Instruction_t*ins = codeSegment->Intermcode;
	uint32_t instrCount =0;
	uint32_t instrCountRemaining = codeSegment->MIPScodeLen;

	if (ins == NULL)
	{
		printf("Not initialized this code segment. Please run 'optimize intermediate'\n");
		return;
	}

	if (bCountSaturates)
	{
		printf("Optimize_CountRegister failed. Not implemented QADD \n");
		abort();
	}

#if 0
	//loop through the instructions and update COUNT every countFrequency

	while (ins->nextInstruction->nextInstruction)
	{
		instrCount++;

		if (instrCount >= uiCountFrequency && instrCountRemaining >= uiCountFrequency)
		{
			//add COUNT update
			Instruction_t* newInstruction 	= newEmptyInstr();

			if (bCountSaturates)
			{
				//TODO QADD
			}
			else
			{
				newInstruction = newInstrIS(ARM_ADD,AL,REG_COUNT,REG_COUNT,REG_NOT_USED, uiCountFrequency);
				ADD_LL_NEXT(newInstruction, ins);

				instrCountRemaining -= uiCountFrequency;

				ins = insertCall_To_C(ins, PL, FUNC_GEN_INTERRUPT);
				instrCount = 0;
			}
		}

		ins = ins->nextInstruction;
	}
	//now add a final update before end of function
#endif

	if (instrCount && instrCountRemaining)
	{
		Instruction_t* newInstruction 	= newEmptyInstr();

		//create COUNT update instructions
		if (bCountSaturates)
		{
			//TODO QSUB
		}
		else
		{
			newInstruction = newInstrI(ARM_ADD, AL, REG_COUNT, REG_COUNT, REG_NOT_USED, instrCountRemaining&0xff);
			newInstruction->nextInstruction = ins->nextInstruction;
			ADD_LL_NEXT(newInstruction, ins);

			if (instrCountRemaining > 255)
			{
				newInstruction = newInstrI(ARM_ADD, AL, REG_COUNT, REG_COUNT, REG_NOT_USED, instrCountRemaining&0xff00);
				ADD_LL_NEXT(newInstruction, ins);
			}

			newInstruction = newInstrS(ARM_CMP, AL, REG_NOT_USED, REG_COMPARE, REG_COUNT);
			ADD_LL_NEXT(newInstruction, ins);

			// We need to set IP7 of the Cause Register and call cc_interrupt()
			newInstruction = newInstrI(ARM_ORR, AL, REG_CAUSE, REG_CAUSE, REG_NOT_USED, 0x8000);
			ADD_LL_NEXT(newInstruction, ins);

			newInstruction = newInstrI(ARM_B, PL, REG_NOT_USED, REG_NOT_USED, REG_NOT_USED, MMAP_FP_BASE + FUNC_GEN_INTERRUPT);
			newInstruction->Ln = 1;
			newInstruction->I = 1;
			ADD_LL_NEXT(newInstruction, ins);
			return;
		}
	}
}

/*
 *
 */
void Translate_Constants(code_seg_t* const codeSegment)
{
	Instruction_t*ins = codeSegment->Intermcode;

	//First off r0 is ALWAYS 0 so lets do that first
	while (ins)
	{
		if ((ins->Rd1.regID & ~REG_WIDE) == 0)
		{
			ins->Rd1.state = RS_CONSTANT_U8;
			ins->Rd1.u8 = 0;
		}

		if ((ins->Rd2.regID & ~REG_WIDE) == 0)
		{
			ins->Rd2.state = RS_CONSTANT_U8;
			ins->Rd2.u8 = 0;
		}

		if ((ins->R1.regID & ~REG_WIDE) == 0)
		{
			ins->R1.state = RS_CONSTANT_U8;
			ins->R1.u8 = 0;
		}

		if ((ins->R2.regID & ~REG_WIDE) == 0)
		{
			ins->R2.state = RS_CONSTANT_U8;
			ins->R2.u8 = 0;
		}

		if ((ins->R3.regID & ~REG_WIDE) == 0)
		{
			ins->R3.state = RS_CONSTANT_U8;
			ins->R3.u8 = 0;
		}

		ins = ins->nextInstruction;
	}

	ins = codeSegment->Intermcode;

	while (ins)
	{
		switch (ins->instruction)
		{

		case LUI:
			ins->Rd1.state = RS_CONSTANT_I8;
			if (ins->immediate < 0)
			{
				ins->Rd1.i8 = 0xffffffff00000000ll | (ins->immediate << 16);
			}else
			{
			ins->Rd1.i8 = ins->immediate;
			}			break;
		default: break;
		}
		ins = ins->nextInstruction;
	}
}

/*
 * Function to turn 64bit registers into multiple 32-bit registers
 *
 */
void Translate_32BitRegisters(code_seg_t* const codeSegment)
{
	Instruction_t*ins;
	Instruction_t*new_ins;
	ins = codeSegment->Intermcode;

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
			case DSLLV:
				/*
				 *		Rd1 W        Rd1            R1 W           R1               R2 W          R2
				 * [FF FF FF FF | FF FF FF FE] = [FF FF FF FF | FF FF FF FF] << [00 00 00 00 | 00 00 00 3F]
				 *
				 *
				 */

				// 1. Work out lower Word
				new_ins = newInstr(ARM_MOV, AL, ins->Rd1.regID , REG_NOT_USED, ins->R1.regID);
				new_ins->shiftType = LOGICAL_LEFT;
				new_ins->R3.regID = ins->R2.regID;
				ADD_LL_NEXT(new_ins, ins);

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
				new_ins = newInstr(ARM_MOV, AL, ins->Rd1.regID, REG_NOT_USED, ins->R1.regID);
				new_ins->shiftType = LOGICAL_RIGHT;
				new_ins->R3.regID = ins->R2.regID;
				ADD_LL_NEXT(new_ins, ins);

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

void Translate_Generic(code_seg_t* const codeSegment)
{
	Instruction_t*ins;
	Instruction_t*new_ins;
	ins = codeSegment->Intermcode;

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
			case ORI:
				if (ins->immediate > 255)
				{
					new_ins = newInstrI(ARM_ORR, AL, ins->Rd1.regID, ins->Rd1.regID, REG_NOT_USED, ins->immediate&0xFF00);
					ins->immediate = ins->immediate & 0xFF;
					ADD_LL_NEXT(new_ins, ins);
				}
				break;
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

void Translate_FPU(code_seg_t* const codeSegment)
{
	Instruction_t*ins;
	Instruction_t*new_ins;
	ins = codeSegment->Intermcode;

	while (ins)
	{
		switch (ins->instruction)
		{
			case MFC0: break;
			case MTC0: break;
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
		default: break;
		}

		ins = ins->nextInstruction;
	}
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
			ins = InstrI(ins, ARM_TST, AL, REG_NOT_USED, ins->R2.regID, REG_NOT_USED, 0x08 << 24);

			// if address is raw (NE) then add base offset to get to host address
			new_ins = newInstrI(ARM_ADD, NE, REG_TEMP_MEM1, ins->R2.regID, REG_NOT_USED, uMemoryBase << 24);
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

			// now lookup virtual address
			//ins = insertCall_To_C(ins, EQ, FUNC_GEN_LOOKUP_VIRTUAL_ADDRESS);
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

			//ins = insertCall_To_C(ins, EQ, FUNC_GEN_LOOKUP_VIRTUAL_ADDRESS);

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

void Translate_LoadStoreWriteBack(code_seg_t* const codeSegment)
{
	Instruction_t*ins;
	ins = codeSegment->Intermcode;

	while (ins)
	{
		ins = ins->nextInstruction;
	}
}

void Translate_LoadCachedRegisters(code_seg_t* const codeSegment)
{
	Instruction_t*ins = codeSegment->Intermcode;
	Instruction_t*prev_ins = NULL;

	Instruction_t*new_ins;
	uint8_t RegUsed[64];

	memset(RegUsed,0,sizeof(RegUsed));

	while (ins)
	{
		if (ins->R1.state == RS_REGISTER
				&& ins->R1.regID < REG_TEMP
				&& !RegUsed[ins->R1.regID])
		{
			RegUsed[ins->R1.regID]++;
			new_ins = newInstrI(ARM_LDR_LIT, AL, ins->R1.regID, REG_NOT_USED, REG_HOST_FP, ins->R1.regID * 4);
			new_ins->nextInstruction = ins;

			if (!prev_ins)
			{
				codeSegment->Intermcode = new_ins;
			}
			else
			{
				prev_ins->nextInstruction = new_ins;
			}
		}

		if (ins->R2.state == RS_REGISTER
				&& ins->R2.regID < REG_TEMP
				&& !RegUsed[ins->R2.regID])
		{
			RegUsed[ins->R2.regID]++;
			new_ins = newInstrI(ARM_LDR_LIT, AL, ins->R2.regID, REG_NOT_USED, REG_HOST_FP, ins->R2.regID * 4);
			new_ins->nextInstruction = ins;

			if (!prev_ins)
			{
				codeSegment->Intermcode = new_ins;
			}
			else
			{
				prev_ins->nextInstruction = new_ins;
			}
		}

		if (ins->R3.state == RS_REGISTER
				&& ins->R3.regID < REG_TEMP
				&& !RegUsed[ins->R3.regID])
		{
			RegUsed[ins->R3.regID]++;
			new_ins = newInstrI(ARM_LDR_LIT, AL, ins->R3.regID, REG_NOT_USED, REG_HOST_FP, ins->R3.regID * 4);
			new_ins->nextInstruction = ins;

			if (!prev_ins)
			{
				codeSegment->Intermcode = new_ins;
			}
			else
			{
				prev_ins->nextInstruction = new_ins;
			}
		}

		prev_ins = ins;
		ins = ins->nextInstruction;
	}
}

static void getNextRegister(Instruction_t* ins, uint32_t* uiCurrentRegister)
{
	uint32_t uiLastRegister = *uiCurrentRegister;

	while ((FindRegNextUsedAgain(ins, REG_HOST + *uiCurrentRegister) > 0))
	{
		(*uiCurrentRegister)++;
		if (*uiCurrentRegister > 10) *uiCurrentRegister = 0;

		// Have we looped round all registers?
		if (uiLastRegister == *uiCurrentRegister){
			abort();
		}
	}
}

/*
 * Function to re-number / reduce the number of registers so that they fit the HOST
 *
 * This function will need to scan a segment and when more than the number of spare
 * HOST registers is exceeded, choose to either save register(s) into the emulated registers
 * referenced by the Frame Pointer or push them onto the stack for later use.
 *
 * Pushing onto the stack may make it easier to use LDM/SDM where 32/64 bit is not compatible
 * with the layout of the emulated register space.
 *
 */
void Translate_Registers(code_seg_t* const codeSegment)
{
	Instruction_t* ins;
	//Instruction_t*insSearch;

	uint32_t x;
	uint32_t NumberRegUsed = 0;

	uint16_t counts[REG_T_SIZE];
	memset(counts,0,sizeof(counts));

	ins = codeSegment->Intermcode;
	while (ins)
	{
		if (ins->Rd1.regID != REG_NOT_USED && ins->Rd1.state == RS_REGISTER) counts[ins->Rd1.regID]++;
		if (ins->Rd2.regID != REG_NOT_USED && ins->Rd2.state == RS_REGISTER) counts[ins->Rd2.regID]++;
		if (ins->R1.regID != REG_NOT_USED && ins->R1.state == RS_REGISTER) counts[ins->R1.regID]++;
		if (ins->R2.regID != REG_NOT_USED && ins->R2.state == RS_REGISTER) counts[ins->R2.regID]++;
		if (ins->R3.regID != REG_NOT_USED && ins->R3.state == RS_REGISTER) counts[ins->R3.regID]++;

		ins = ins->nextInstruction;
	}

	for (x=0; x < REG_HOST + 11; x++)
	{
		if (counts[x]) NumberRegUsed++;
	}

	//printf("Segment 0x%x uses %d registers\n",(uint32_t)codeSegment, NumberRegUsed);
	
	if (NumberRegUsed <= 11)
	{
		ins = codeSegment->Intermcode;
		uint32_t uiCurrentRegister = 0;

		while (counts[REG_HOST + uiCurrentRegister])
			uiCurrentRegister++; // Find the next free HOST register

		for (x = 0; x < REG_HOST; x++ )
		{
			if (counts[x])
			{
				UpdateRegWithReg(ins,(regID_t)x, REG_HOST + uiCurrentRegister, 0);
				uiCurrentRegister++;
				while (counts[REG_HOST + uiCurrentRegister]) uiCurrentRegister++; // Find the next free HOST register
			}
		}
	}
	else
	{
		ins = codeSegment->Intermcode;

		//we should do this in the 'instruction' domain so that non-overlapping register usage can be 'flattened'

		uint32_t uiCurrentRegister = 0;

		getNextRegister(ins, &uiCurrentRegister);

		while (ins)
		{
			if (ins->Rd1.regID != REG_NOT_USED  && ins->Rd1.state == RS_REGISTER && ins->Rd1.regID < REG_HOST){
				UpdateRegWithReg(ins,ins->Rd1.regID, REG_HOST + uiCurrentRegister, 0);
				getNextRegister(ins, &uiCurrentRegister);
			}

			if (ins->Rd2.regID != REG_NOT_USED && ins->Rd2.state == RS_REGISTER && ins->Rd2.regID < REG_HOST){
				UpdateRegWithReg(ins,ins->Rd2.regID, REG_HOST + uiCurrentRegister, 0);
				getNextRegister(ins, &uiCurrentRegister);
			}

			if (ins->R1.regID != REG_NOT_USED && ins->R1.state == RS_REGISTER && ins->R1.regID < REG_HOST){
				UpdateRegWithReg(ins,ins->R1.regID, REG_HOST + uiCurrentRegister, 0);
				getNextRegister(ins, &uiCurrentRegister);
			}

			if (ins->R2.regID != REG_NOT_USED && ins->R2.state == RS_REGISTER && ins->R2.regID < REG_HOST){
				UpdateRegWithReg(ins,ins->R2.regID, REG_HOST + uiCurrentRegister, 0);
				getNextRegister(ins, &uiCurrentRegister);
			}

			if (ins->R3.regID != REG_NOT_USED && ins->R3.state == RS_REGISTER && ins->R3.regID < REG_HOST){
				UpdateRegWithReg(ins,ins->R3.regID, REG_HOST + uiCurrentRegister, 0);
				getNextRegister(ins, &uiCurrentRegister);
			}

			ins = ins->nextInstruction;
		}
	}

#if 1
	//Strip HOST flag from register ID leaving ARM register ID ready for writing
	ins = codeSegment->Intermcode;
	while (ins)
	{
		if (ins->Rd1.regID != REG_NOT_USED && ins->Rd1.state == RS_REGISTER) ins->Rd1.regID &= ~REG_HOST;
		if (ins->Rd2.regID != REG_NOT_USED && ins->Rd2.state == RS_REGISTER) ins->Rd2.regID &= ~REG_HOST;
		if (ins->R1.regID != REG_NOT_USED && ins->R1.state == RS_REGISTER) ins->R1.regID &= ~REG_HOST;
		if (ins->R2.regID != REG_NOT_USED && ins->R2.state == RS_REGISTER) ins->R2.regID &= ~REG_HOST;
		if (ins->R3.regID != REG_NOT_USED && ins->R3.state == RS_REGISTER) ins->R3.regID &= ~REG_HOST;

		ins = ins->nextInstruction;
	}
#endif

	// ------------ sanity check --------------

#ifndef NDEBUG
	ins = codeSegment->Intermcode;

	while (ins)
	{
		assert( ins->Rd1.state != RS_REGISTER || (ins->Rd1.regID & ~REG_HOST) < 16 || ins->Rd1.regID == REG_NOT_USED);
		assert( ins->Rd2.state != RS_REGISTER || (ins->Rd2.regID & ~REG_HOST) < 16 || ins->Rd2.regID == REG_NOT_USED);
		assert( ins->R1.state != RS_REGISTER || (ins->R1.regID & ~REG_HOST) < 16 || ins->R1.regID == REG_NOT_USED);
		assert( ins->R2.state != RS_REGISTER || (ins->R2.regID & ~REG_HOST) < 16 || ins->R2.regID == REG_NOT_USED);
		assert( ins->R3.state != RS_REGISTER || (ins->R3.regID & ~REG_HOST) < 16 || ins->R3.regID == REG_NOT_USED);
		ins = ins->nextInstruction;
	}
#endif
	return;
}

void Translate_StoreCachedRegisters(code_seg_t* const codeSegment)
{
		Instruction_t*ins = codeSegment->Intermcode;

		Instruction_t*new_ins;

		while (ins)
		{
			if (ins->Rd1.regID < REG_TEMP
					&& ins->Rd1.state == RS_REGISTER
				)
			{
				int32_t nextUsed = FindRegNextUsedAgain(ins, ins->Rd1.regID);

				//Register will be over-written before next use so don't bother saving
				if (nextUsed == -1)
				{

				}
				else if (nextUsed == 0)
				{
					new_ins = newInstrI(ARM_STR_LIT, AL, REG_NOT_USED, ins->Rd1.regID, REG_HOST_FP, ins->Rd1.regID * 4);
					new_ins->nextInstruction = ins->nextInstruction;
					ins->nextInstruction = new_ins;
					ins = ins->nextInstruction;
				}
			}
			else if (ins->Rd1.regID < REG_TEMP)
			{
				//TODO depending on literal, we could do a ARM_MOV

				regID_t regBase;
				int32_t offset;

				addLiteral(codeSegment,&regBase,&offset,ins->Rd1.u4);

				new_ins = newInstrI(ARM_LDR_LIT, AL, REG_TEMP_STR_CONST, REG_NOT_USED, regBase, offset);
				ADD_LL_NEXT(new_ins, ins);

				new_ins = newInstrI(ARM_STR_LIT, AL, REG_NOT_USED, REG_TEMP_STR_CONST, REG_HOST_FP, ins->Rd1.regID * 4);
				ADD_LL_NEXT(new_ins, ins);
			}

			if (ins->Rd2.state == RS_REGISTER
					&& ins->Rd2.regID < REG_TEMP)
			{
				int32_t nextUsed = FindRegNextUsedAgain(ins, ins->Rd2.regID);

				//Register will be over-written before next use so don't bother saving
				if (nextUsed == -1)
				{

				}
				else if (nextUsed == 0)
				{
					new_ins = newInstrI(ARM_STR_LIT, AL, REG_NOT_USED, ins->Rd2.regID, REG_HOST_FP, ins->Rd2.regID * 4);
					new_ins->nextInstruction = ins->nextInstruction;
					ins->nextInstruction = new_ins;
					ins = ins->nextInstruction;
				}
			}
			else if (ins->Rd2.regID < REG_TEMP)
			{
				abort();
			}

			ins = ins->nextInstruction;
		}
}

void Translate_Trap(code_seg_t* const codeSegment)
{
	Instruction_t*ins;
	Instruction_t*new_ins;
	ins = codeSegment->Intermcode;

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
				ins->instruction = ARM_ADD;
				ins->S = 1;
				//insertCall_To_C(codeSegment, ins, VS, MMAP_FP_BASE + FUNC_GEN_TRAP);
				break;
			case ADDIU:
				ins->instruction = ARM_ADD;
				break;
			case SLTI: break;
			case SLTIU: break;
			case ANDI: break;
			case ORI: break;
			case XORI: break;
			case LUI: break;
			case MFC0: break;
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

				new_ins = newInstr(ARM_B, EQ, REG_NOT_USED,REG_NOT_USED,REG_NOT_USED);

				new_ins->offset = tgt_address;
				new_ins->I = branchAbsolute;

				new_ins->nextInstruction = ins->nextInstruction;
				ins->nextInstruction = new_ins;
				ins = ins->nextInstruction;


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

				new_ins = newInstr(ARM_B, NE, REG_NOT_USED,REG_NOT_USED,REG_NOT_USED);

				new_ins->offset = tgt_address;
				new_ins->I = branchAbsolute;

				new_ins->nextInstruction = ins->nextInstruction;
				ins->nextInstruction = new_ins;
				ins = ins->nextInstruction;

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

				new_ins = newInstr(ARM_B, AL, REG_NOT_USED,REG_NOT_USED,REG_NOT_USED);

				new_ins->offset = tgt_address;
				new_ins->I = branchAbsolute;

				new_ins->nextInstruction = ins->nextInstruction;
				ins->nextInstruction = new_ins;
				ins = ins->nextInstruction;
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
/*
 * Function to correct the offset to be applied for literal store/loading
 */
void Translate_Literals(const code_seg_t* const codeSegment)
{
	Instruction_t*ins;
	ins = codeSegment->Intermcode;
	uint32_t x = 0;

	uint32_t InterimCodeLen = 0;

	while (ins)
	{
		ins = ins->nextInstruction;
		InterimCodeLen++;
	}

	ins = codeSegment->Intermcode;

	while (ins)
	{
		switch (ins->instruction)
		{
		case ARM_STR_LIT:
		case ARM_LDR_LIT:
			if (codeSegment->Type == SEG_START)
			{
				ins->offset = ins->offset - x - 12;
			}
			else if (codeSegment->Type == SEG_END)
			{
				ins->offset = InterimCodeLen * 4 - x + ins->offset - 4;
			}

			break;
		default:
			break;
		}
		ins = ins->nextInstruction;
		x+=4;
	}
}

void Translate(code_seg_t* const codeSegment)
{
	Translate_init(codeSegment);

	Translate_DelaySlot(codeSegment);
	Translate_CountRegister(codeSegment);
	Translate_Constants(codeSegment);
	Translate_32BitRegisters(codeSegment);

	Translate_Generic(codeSegment);
	Translate_FPU(codeSegment);

	Translate_Trap(codeSegment);
	Translate_Memory(codeSegment);

	Translate_LoadStoreWriteBack(codeSegment);
	Translate_LoadCachedRegisters(codeSegment);
	Translate_StoreCachedRegisters(codeSegment);

	Translate_Branch(codeSegment);
	Translate_Registers(codeSegment);
	Translate_Literals(codeSegment);
}
