/*
 * C_Interface.c
 *
 *  Created on: 23 Oct 2014
 *      Author: rjhender
 */

#include "Translate.h"
#include "CodeSegments.h"
#include "InstructionSetMIPS4.h"
#include "InstructionSetARM6hf.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include "memory.h"

#include "Debugger.h"	// RWD
#include "DebugDefines.h"

uint32_t bMemoryInlineLookup= 0;

void cc_interrupt()
{
	uint32_t* regCause = (uint32_t*)(MMAP_FP_BASE) + REG_CAUSE;
	uint32_t* dmaPI = (uint32_t*)(MMAP_PI) ;


	*regCause |= 0x8000;

	printf("cc_interrupt() called\n");

	if (*(dmaPI+2))
	{
		printf("PI DMA 'RAM to Cartridge' detected\n");
		memcpy((void*)*(dmaPI+1),(void*)*(dmaPI+0),*(dmaPI+2));
		*(dmaPI+3) = 0;
	}

	if (*(dmaPI+3))
	{

		printf("PI DMA 'Cartridge to RAM' detected\n");
		memcpy((void*)*(dmaPI+0),(void*)*(dmaPI+1),*(dmaPI+3));
		*(dmaPI+3) = 0;
	}


}

uint32_t virtual_address(unsigned int* addr)
{
	if (!addr)
	{
		printf("virtual_address (nil)\n");
	}
	else
	{
		printf("virtual_address 0x%08x = 0x08x\n",(uint32_t)addr, *addr);
	}

	return 0x80000000 + addr;
}

void p_r_a(uint32_t r0, uint32_t r1, uint32_t r2, uint32_t r3
		, uint32_t r4, uint32_t r5, uint32_t r6, uint32_t r7
		, uint32_t r8, uint32_t r9, uint32_t r10, uint32_t r11
		, uint32_t r12, uint32_t r13, uint32_t r14, uint32_t r15)
{
	printf( "\tr0 0x%08x\t r8  0x%08x\n"
			"\tr1 0x%08x\t r9  0x%08x\n"
			"\tr2 0x%08x\t r10 0x%08x\n"
			"\tr3 0x%08x\t fp  0x%08x\n"
			"\tr4 0x%08x\t ip  0x%08x\n"
			"\tr5 0x%08x\t sp  0x%08x\n"
			"\tr6 0x%08x\t lr  0x%08x\n"
			"\tr7 0x%08x\t pc  0x%08x\n"
			, r0, r8
			, r1, r9
			, r2, r10
			, r3, r11
			, r4, r12
			, r5, r13
			, r6, r14
			, r7, r15);
}
/*
 * Function to Compile MIPS code at 'address' and then return the ARM_PC counter to jump to
 */
size_t branchUnknown(size_t address)
{
	volatile code_seg_t* code_seg 	= segmentData.dbgCurrentSegment;
	uint32_t* 	out 		= code_seg->ARMcode + code_seg->ARMcodeLen -1;

	printf("branchUnknown(0x%08x) called from Segment 0x%08x\n", address, (uint32_t)code_seg);

	code_seg_t* tgtSeg = getSegmentAt(address);
	if (NULL != tgtSeg)
	{
		if (NULL == tgtSeg->ARMEntryPoint) Translate(tgtSeg);
 		return (size_t)tgtSeg->ARMEntryPoint;
	}
	// 1. Need to generate the ARM assembler for target code_segment. Use 'addr' and code Seg map.
	// 2. Then we need to patch the code_segment branch we came from. Do we need it to be a link?
	// 3. return the address to branch to.

	// 1.
	CompileCodeAt((uint32_t*)address);

	// 2.
	Instruction_t 	ins;

	//Get MIPS condition code for branch
	mips_decode(*(code_seg->MIPScode + code_seg->MIPScodeLen -1), &ins);

	//now we can get the target Code Segment ARM Entry Point
	size_t targetAddress = (size_t)getSegmentAt(address)->ARMEntryPoint;

	//Set instruction to ARM_BRANCH for new target
	InstrB(&ins, ins.cond, targetAddress, 1);

	//emit the arm code
	*out = arm_encode(&ins, (size_t)out);

	// 3.
	return targetAddress;
}

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
 *//*
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
	newInstruction 		= newInstr(ARM_ADD, AL, REG_TEMP_GEN1, REG_HOST_R0, REG_HOST_R1);
	code_seg->Intermcode = ins = newInstruction;

	//shift Right so that we have the final Byte
	newInstruction 		= newInstr(SRL, AL, REG_TEMP_MEM2, REG_TEMP_GEN1, REG_NOT_USED);
	newInstruction->I = 1;
	newInstruction->shift = 24;
	ADD_LL_NEXT(newInstruction, ins);

	//If the final Byte is 0x80 then all is good
	newInstruction 		= newInstrI(ARM_CMP, AL, REG_NOT_USED, REG_TEMP_MEM2, REG_NOT_USED, 0x80);
	newInstruction->I = 1;
	ADD_LL_NEXT(newInstruction, ins);

	//Move the address to R0
	newInstruction 		= newInstr(ARM_MOV, EQ, REG_HOST_R0, REG_NOT_USED, REG_TEMP_GEN1);
	ADD_LL_NEXT(newInstruction, ins);

	// Return
	newInstruction 		= newInstr(ARM_MOV, EQ, REG_HOST_PC, REG_NOT_USED, REG_HOST_LR);
	ADD_LL_NEXT(newInstruction, ins);

	//Else is the final byte 0xA0?
	newInstruction 		= newInstrI(ARM_CMP, AL, REG_NOT_USED, REG_TEMP_MEM2, REG_NOT_USED, 0xA0);
	newInstruction->I = 1;
	ADD_LL_NEXT(newInstruction, ins);

	//If so then clear bit 0x40
	newInstruction 		= newInstrI(ARM_BIC, EQ, REG_HOST_R0, REG_TEMP_GEN1, REG_NOT_USED, 0x40);
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
}*/


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

	newInstruction 		= newInstr(ARM_MOV, AL, REG_HOST_LR, REG_NOT_USED, REG_HOST_PC);
	ADD_LL_NEXT(newInstruction, ins);

	newInstruction 		= newInstr(ARM_MOV, AL, REG_HOST_PC, REG_NOT_USED, REG_HOST_R1);
	ADD_LL_NEXT(newInstruction, ins);

	newInstruction 		= newInstrPOP(AL, REG_HOST_STM_EABI2 );
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

#elif defined(TEST_ROR)

	newInstruction 		= newInstrI(ARM_MOV, AL, REG_HOST_R0, REG_NOT_USED, REG_NOT_USED, 0x00138000);
	ADD_LL_NEXT(newInstruction, ins);

	newInstruction 		= newInstr(ARM_MOV, AL, REG_HOST_PC, REG_NOT_USED, REG_HOST_LR);
	ADD_LL_NEXT(newInstruction, ins);

#else
	addLiteral(code_seg, &base, &offset,(uint32_t)MMAP_FP_BASE);
	assert(base == REG_HOST_PC);

	// setup the HOST_FP
	newInstruction 		= newInstrI(ARM_LDR_LIT, AL, REG_EMU_FP, REG_NOT_USED, base, offset);
	ADD_LL_NEXT(newInstruction, ins);

	// start executing recompiled code
	newInstruction 		= newInstrI(ARM_LDR, AL, REG_HOST_PC, REG_NOT_USED, REG_EMU_FP, RECOMPILED_CODE_START);
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

#if defined (USE_INSTRUCTION_COMMENTS)
	sprintf(newInstruction->comment, "Generate_ISR() segment 0x%08x\n", code_seg);
#endif

	ins = insertCall_To_C(code_seg, ins, AL, (size_t)&cc_interrupt, REG_HOST_STM_EABI);

	// Return
	newInstruction 		= newInstr(ARM_MOV, AL, REG_HOST_PC, REG_NOT_USED, REG_HOST_LR);
	ADD_LL_NEXT(newInstruction, ins);

#if defined(USE_TRANSLATE_DEBUG)
	Translate_Debug(code_seg);
#endif

	Translate_Registers(code_seg);

	return code_seg;
}

code_seg_t* Generate_BranchUnknown(code_segment_data_t* seg_data)
{
	code_seg_t* 	code_seg 		= newSegment();
	Instruction_t* 	newInstruction;
	Instruction_t* 	ins 			= NULL;

	seg_data->dbgCurrentSegment = code_seg;

	/* Don't think we need this instruction as function branchUnknown() can look
	 * up 'seg_data->dbgCurrentSegment' to:
	 * 1. find/compile the branch target
	 * 2. patch the branch instruction that caused us to end up here.
	 */
	newInstruction 		= newInstr(ARM_MOV, AL, REG_HOST_R0, REG_NOT_USED, REG_HOST_R0);
	code_seg->Intermcode = ins = newInstruction;

#if defined (USE_INSTRUCTION_COMMENTS)
	sprintf(newInstruction->comment, "Generate_BranchUnknown() segment 0x%08x\n", code_seg);
#endif

	ins = insertCall_To_C(code_seg, ins, AL, (uint32_t)&branchUnknown, REG_HOST_STM_R1_3);

	// Now jump to the compiled code
	newInstruction 		= newInstrI(ARM_SUB, AL, REG_HOST_PC, REG_HOST_R0, REG_NOT_USED, 0);
	ADD_LL_NEXT(newInstruction, ins);

#if 0 && defined(USE_TRANSLATE_DEBUG)
	Translate_Debug(code_seg);
#endif

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

#if defined(USE_TRANSLATE_DEBUG)
	Translate_Debug(code_seg);
#endif

	Translate_Registers(code_seg);

	return code_seg;
}
