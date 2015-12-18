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

#include <stdint.h>
#include <assert.h>
#include <stdio.h>

#include "Debugger.h"
#include "CodeSegments.h"
#include "Translate.h"
#include "memory.h"
#include "InstructionSetMIPS4.h"
#include "InstructionSetARM6hf.h"

#define TEST_BREAK		(0)
#define TEST_PREF 		(0)
#define TEST_SYNC	 	(0)
#define TEST_SYSCALL 	(0)
#define TEST_TRAP 		(0)
#define TEST_COPROCESSOR (0)

#define EXPECT_EQ(ACTUAL, EXPECTED) \
		if ((ACTUAL) != (EXPECTED)){ \
			printf("%s:%d\n\t" #EXPECTED " != " #ACTUAL "\n\tExpected %lld (0x%llx), Actual %lld (0x%llx)\n" , __FUNCTION__, __LINE__, (uint64_t)(EXPECTED), (uint64_t)(EXPECTED), (uint64_t)(ACTUAL), (uint64_t)(ACTUAL)); \
			CodeSeg_print(seg);\
			if (seg->pContinueNext != NULL){ printf("------------ seg->pContinueNext (0x%x) ------------\n", (uint32_t)seg->pContinueNext); CodeSeg_print(seg->pContinueNext);} \
			if (seg->pBranchNext != NULL)  { printf("------------ seg->pBranchNext (0x%x) --------------\n", (uint32_t)seg->pBranchNext);   CodeSeg_print(seg->pBranchNext);} \
		}
#define ASSERT_EQ(ACTUAL, EXPECTED) \
		if ((ACTUAL) != (EXPECTED)){ \
			printf("%s:%d\n\t" #EXPECTED " != " #ACTUAL "\n\tExpected %lld (0x%llx), Actual %lld (0x%llx)\n" , __FUNCTION__, __LINE__, (uint64_t)(EXPECTED), (uint64_t)(EXPECTED), (uint64_t)(ACTUAL), (uint64_t)(ACTUAL)); \
			CodeSeg_print(seg);\
			if (seg->pContinueNext != NULL){ printf("------------ seg->pContinueNext (0x%x) ------------\n", (uint32_t)seg->pContinueNext); CodeSeg_print(seg->pContinueNext);} \
			if (seg->pBranchNext != NULL)  { printf("------------ seg->pBranchNext (0x%x) --------------\n", (uint32_t)seg->pBranchNext);   CodeSeg_print(seg->pBranchNext);} \
			while (Debugger_start(segmentData, NULL, NULL));\
			return; \
		}

// provide a function pointer to the START routine
static pfvru1 run = NULL;

// provide a pointer to the emulated N64 registers
static volatile uint64_t* reg = NULL;

// pointer to area where to write raw MIPS code for testing
static uint32_t* mipsCode;

static void TranslationTest_ADD(code_segment_data_t* segmentData)
{
	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;

	uint8_t rs = 1U;
	uint8_t rt = 2U;
	uint8_t rd = 3U;

	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;

	// MIPS ADD Instruction
	mipsCode[0] = 0x00000020 + (rs << 21) + (rt << 16) + (rd << 11);
	mipsCode[1] = 0x00000008;	// JR to Rs(0)
	mipsCode[2] = 0x00000000;	// Delay Slot

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -1000;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	reg[rs] = 0x01234567U;
	reg[rt] = 0x07654321U;

	run();

	ASSERT_EQ((0x01234567U + 0x07654321U), reg[rd]);
	ASSERT_EQ(0x01234567U, reg[rs]);
	ASSERT_EQ(0x07654321U, reg[rt]);

	//---------------------------------------
	// Test for overflow trap

	reg[rs] = 0x70000000U;
	reg[rt] = 0x10000000U;
	reg[rd] = 0U;

	run();

	ASSERT_EQ(reg[rd], 0U);
	ASSERT_EQ(0x70000000U, reg[rs]);

	//---------------------------------------
	// Test writing back to input register

	mipsCode[0] = 0x00000020 + (1 << 21) + (2 << 16) + (1 << 11);

	Translate(seg);
	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	reg[1] = 0x01234567U;
	reg[2] = 0x07654321U;

	run();

	ASSERT_EQ(reg[1], (0x01234567U + 0x07654321U));

	//---------------------------------------
	// Test writing back to input register

	mipsCode[0] = 0x00000020 + (2 << 21) + (1 << 16) + (1 << 11);

	Translate(seg);
	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	reg[1] = 0x01234567U;
	reg[2] = 0x07654321U;

	run();

	ASSERT_EQ(reg[1], (0x01234567U + 0x07654321U));

	//---------------------------------------

	delSegment(seg);

	printf("ADD     translated successfully\n");
}

static void TranslationTest_ADDI(code_segment_data_t* segmentData)
{
	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;

	uint8_t rs = 1U;
	uint8_t rt = 2U;

	uint32_t imm = 0x0021U;

	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;

	// MIPS ADDI Instruction
	mipsCode[0] = 0x20000000 + (rs << 21U) + (rt << 16U) + imm;
	mipsCode[1] = 0x00000008;	// JR to Rs(0)
	mipsCode[2] = 0x00000000;	// Delay Slot

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	reg[rs] = 0x11111111U;

	run();

	ASSERT_EQ((0x11111111U + imm), reg[rt]);
	ASSERT_EQ((0x11111111U), reg[rs]);

	//---------------------------------------

	imm = 0x4321U;

	// MIPS ADDI Instruction
	mipsCode[0] = 0x20000000 + (rs << 21U) + (rt << 16U) + imm;

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	reg[rs] = 0x11111111U;
	reg[rt] = 0x00000000U;

	run();

	ASSERT_EQ((0x11111111U + imm), reg[rt]);
	ASSERT_EQ((0x11111111U), reg[rs]);

	//---------------------------------------

	imm = 0x8000U;
	mipsCode[0] = 0x20000000 + (rs << 21U) + (rt << 16U) + imm;
	Translate(seg);
	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	reg[rs] = 0x11111111U;

	run();

	ASSERT_EQ((0x11111111U - imm), reg[rt]);
	ASSERT_EQ((0x11111111U), reg[rs]);

	//---------------------------------------

	mipsCode[0] = 0x20000000 + (1 << 21U) + (1 << 16U) + 2U;
	Translate(seg);
	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	reg[1] = 0x00000001U;

	run();

	ASSERT_EQ(reg[1], 3U);

	//---------------------------------------

	// Test writing back to input register
	imm = 0x3;

	mipsCode[0] = 0x20000000 + (1 << 21U) + (1 << 16U) + imm;

	Translate(seg);
	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	reg[1] = 0x01234567U;

	run();

	ASSERT_EQ(reg[1], 0x01234567U + imm);

	//---------------------------------------

	delSegment(seg);
	printf("ADDI    translated successfully\n");
}

static void TranslationTest_ADDIU(code_segment_data_t* segmentData)
{
	// Description: rt ← rs + immediate
	// To add a constant to a 32-bit integer.
	// The 16-bit signed immediate is added to the 32-bit value in GPR rs and the 32-bit
	// arithmetic result is placed into GPR rt.
	// No Integer Overflow exception occurs under any circumstances.

	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;

	uint8_t rs = 1;
	uint8_t rt = 2;
	int16_t imm = 0x1111;

	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;

	// MIPS ADDIU Instruction
	mipsCode[0] = 0x24000000 + (rs << 21) + (rt << 16) + (uint16_t)imm ;
	mipsCode[1] = 0x00000008;	// JR to Rs(0)
	mipsCode[2] = 0x00000000;	// Delay Slot

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -1000;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	reg[rs] = 0x11110000;
	reg[rt] = 0x0;

	run();

	ASSERT_EQ(reg[rs] + imm, reg[rt]);

	//---------------------------------------

	imm = -1;

	// MIPS ADDIU Instruction
	mipsCode[0] = 0x24000000 + (rs << 21) + (rt << 16) + (uint16_t)imm ;

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -1000;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	reg[rs] = 0x11110000;
	reg[rt] = 0x0;

	run();

	ASSERT_EQ(0x1110FFFF, reg[rt]);

	//---------------------------------------
	// register writeback

	mipsCode[0] = 0x24000000 + (1 << 21) + (1 << 16) + (uint16_t)imm ;
	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	reg[1] = 0x11110000;

	run();

	ASSERT_EQ(0x1110FFFF, reg[rt]);

	//---------------------------------------

	delSegment(seg);
	printf("ADDIU   translated successfully\n");
}

static void TranslationTest_ADDU(code_segment_data_t* segmentData)
{
	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;

	uint8_t rs = 1;
	uint8_t rt = 2;
	uint8_t rd = 3;

	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;

	// MIPS ADDU Instruction
	mipsCode[0] = 0x00000021U + (rs << 21U) + (rt << 16U) + (rd << 11U);
	mipsCode[1] = 0x00000008U;	// JR to Rs(0)
	mipsCode[2] = 0x00000000U;	// Delay Slot

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -1000;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;


	//---------------------------------------

	reg[rs] = 0x56780000U;
	reg[rt] = 0x32101111U;

	run();

	ASSERT_EQ(reg[rd], reg[rs] + reg[rt]);

	//---------------------------------------
	// register writeback

	mipsCode[0] = 0x00000021 + (2 << 21) + (1 << 16) + (2 << 11);
	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	reg[1] = 0x56780000U;
	reg[2] = 0x32101111U;

	run();

	ASSERT_EQ(reg[2], 0x56780000U + 0x32101111U);

	//---------------------------------------
	// register writeback

	mipsCode[0] = 0x00000021 + (1 << 21) + (2 << 16) + (2 << 11);
	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	reg[1] = 0x56780000U;
	reg[2] = 0x32101111U;

	run();

	ASSERT_EQ(reg[2], 0x56780000U + 0x32101111U);

	//---------------------------------------

	delSegment(seg);
	printf("ADDU    translated successfully\n");
}

static void TranslationTest_AND(code_segment_data_t* segmentData)
{
	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;

	uint8_t rs = 1;
	uint8_t rt = 2;
	uint8_t rd = 3;

	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;

	// MIPS ADDU Instruction
	mipsCode[0] = 0x00000024 + (rs << 21) + (rt << 16) + (rd << 11);
	mipsCode[1] = 0x00000008;	// JR to Rs(0)
	mipsCode[2] = 0x00000000;	// Delay Slot

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -1000;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	reg[rs] = 0x0000101000001010;
	reg[rt] = 0x0000110000001100;

	run();

	ASSERT_EQ((reg[rs] & reg[rt]), reg[rd]);

	//---------------------------------------
	// register writeback

	mipsCode[0] = 0x00000024 + (2 << 21) + (1 << 16) + (2 << 11);
	Translate(seg);
	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	reg[1] = 0x0000101000001010;
	reg[2] = 0x0000110000001100;

	run();

	ASSERT_EQ((0x0000101000001010 & 0x0000110000001100), reg[2]);

	//---------------------------------------
	// register writeback

	mipsCode[0] = 0x00000024 + (1 << 21) + (2 << 16) + (2 << 11);
	Translate(seg);
	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	reg[1] = 0x0000101000001010;
	reg[2] = 0x0000110000001100;

	run();

	ASSERT_EQ((0x0000101000001010 & 0x0000110000001100), reg[2]);

	//---------------------------------------
	delSegment(seg);
	printf("AND     translated successfully\n");
}

static void TranslationTest_ANDI(code_segment_data_t* segmentData)
{
	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;

	uint8_t rs = 1U;
	uint8_t rt = 2U;

	uint16_t imm = 0x10U;

	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;

	// MIPS ANDI Instruction
	mipsCode[0] = 0x30000000U + (rs << 21U) + (rt << 16U) + (uint16_t)imm;
	mipsCode[1] = 0x00000008U;	// JR to Rs(0)
	mipsCode[2] = 0x00000000U;	// Delay Slot

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	reg[rs] = 0x1111111111111111U;

	run();

	ASSERT_EQ((reg[rs] & imm), reg[rt]);


	//---------------------------------------

	imm = 0xFFFFU;
	mipsCode[0] = 0x30000000U + (rs << 21U) + (rt << 16U) + (uint16_t)imm;
	Translate(seg);
	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	reg[rs] = 0x1111111111111111U;

	run();

	ASSERT_EQ(reg[rt], (reg[rs] & (uint64_t)(imm)));

	//---------------------------------------
	// register writeback

	imm = 0xFFFFU;
	mipsCode[0] = 0x30000000U + (1 << 21U) + (1 << 16U) + (uint16_t)imm;
	Translate(seg);
	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	reg[1] = 0x1111111111111111U;

	run();

	ASSERT_EQ(reg[1], (0x1111111111111111U & (uint64_t)(imm)));

	//---------------------------------------

	delSegment(seg);
	printf("ANDI    translated successfully\n");
}

static void TranslationTest_BEQ(code_segment_data_t* segmentData)
{
	mipsCode[0] = 0x00000000U; // Delay Slot
	mipsCode[1] = 0x10000000U + (2U << 21U) + (1 << 16U) + 5U;	// BEQ
	mipsCode[2] = 0x20000000U + (4U << 16U) + 3U;	// Delay Slot

	mipsCode[3] = 0x20000000U + (3U << 16U) + 1U;	// R3 = 1 // Branch not taken
	mipsCode[4] = 0x00000008U;	// JR to Rs(0)
	mipsCode[5] = 0x00000000U;	// Delay Slot

	mipsCode[6] = 0x20000000U + (3U << 16U) + 2U;	// R3 = 2 // Branch taken, Landing Address

	mipsCode[7] = 0x00000008U;	// JR to Rs(0)
	mipsCode[8] = 0x00000000U;	// Delay Slot

	code_seg_t* seg = CompileCodeAt(mipsCode);

	segmentData->dbgCurrentSegment = seg;

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;
	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	// test branching when R1 == R2

	reg[1] = 1U;
	reg[2] = 1U;
	reg[3] = 0U;
	reg[4] = 0U;

	run();

	ASSERT_EQ(reg[1], 1U);
	ASSERT_EQ(reg[3], 2U);
	ASSERT_EQ(reg[4], 3U);

	//---------------------------------------
	// test not branching

	reg[1] = 1U;
	reg[2] = 0U;
	reg[3] = 0U;
	reg[4] = 0U;

	run();

	ASSERT_EQ(reg[1], 1U);
	ASSERT_EQ(reg[3], 1U);
	ASSERT_EQ(reg[4], 3U);

	//---------------------------------------
	// Test looping

	//---------------------------------------


	if (seg->pBranchNext != NULL)
	{
		delSegment(seg->pBranchNext);
	}

	if (seg->pContinueNext != NULL)
	{
		delSegment(seg->pContinueNext);
	}

	//------------- BEQ with NO-OP delay slot--------------------------

	mipsCode[2] = 0U;

	seg = CompileCodeAt(mipsCode);

	segmentData->dbgCurrentSegment = seg;

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;
	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	// test branching when R1 == R2

	reg[1] = 1U;
	reg[2] = 1U;
	reg[3] = 0U;

	run();

	ASSERT_EQ(reg[1], 1U);
	ASSERT_EQ(reg[3], 2U);

	//---------------------------------------
	// test not branching

	reg[1] = 1U;
	reg[2] = 0U;
	reg[3] = 0U;

	run();

	ASSERT_EQ(reg[1], 1U);
	ASSERT_EQ(reg[3], 1U);

	//---------------------------------------

	if (seg->pBranchNext != NULL)
	{
		delSegment(seg->pBranchNext);
	}

	if (seg->pContinueNext != NULL)
	{
		delSegment(seg->pContinueNext);
	}

	printf("BEQ     translated successfully\n");
}

static void TranslationTest_BEQL(code_segment_data_t* segmentData)
{
	mipsCode[0] = 0x00000000U; // Delay Slot
	mipsCode[1] = 0x50000000U + (2U << 21U) + (1 << 16U) + 5U;	// BEQL
	mipsCode[2] = 0x20000000U + (4U << 16U) + 3U;	// Delay Slot

	mipsCode[3] = 0x20000000U + (3U << 16U) + 1U;	// R3 = 1 // BEQ not taken
	mipsCode[4] = 0x00000008U;	// JR to Rs(0)
	mipsCode[5] = 0x00000000U;	// Delay Slot

	mipsCode[6] = 0x20000000U + (3U << 16U) + 2U;	// R3 = 2 // BEQ taken, Landing Address

	mipsCode[7] = 0x00000008U;	// JR to Rs(0)
	mipsCode[8] = 0x00000000U;	// Delay Slot

	code_seg_t* seg = CompileCodeAt(mipsCode);

	segmentData->dbgCurrentSegment = seg;

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;
	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	// test branching when R1 == R2

	reg[1] = 1U;
	reg[2] = 1U;
	reg[3] = 0U;
	reg[4] = 3U;

	run();

	ASSERT_EQ(reg[1], 1U);
	ASSERT_EQ(reg[3], 2U);
	ASSERT_EQ(reg[4], 3U);

	//---------------------------------------
	// test not branching

	reg[1] = 1U;
	reg[2] = 0U;
	reg[3] = 0U;
	reg[4] = 0U;

	run();

	ASSERT_EQ(reg[1], 1U);
	ASSERT_EQ(reg[3], 1U);
	ASSERT_EQ(reg[4], 0U);

	//---------------------------------------
	// Test looping

	//---------------------------------------

	if (seg->pBranchNext != NULL)
	{
		delSegment(seg->pBranchNext);
	}

	if (seg->pContinueNext != NULL)
	{
		delSegment(seg->pContinueNext);
	}

	printf("BEQL    translated successfully\n");
}

static void TranslationTest_BGEZ(code_segment_data_t* segmentData)
{
	mipsCode[0] = 0x00000000U; 						// Delay Slot
	mipsCode[1] = 0x04010000U + (2U << 21U) + 5U;	// BGEZ
	mipsCode[2] = 0x20000000U + (4U << 16U) + 3U;	// Delay Slot

	mipsCode[3] = 0x20000000U + (3U << 16U) + 1U;	// R3 = 1 // BGEZ not taken
	mipsCode[4] = 0x00000008U;	// JR to Rs(0)
	mipsCode[5] = 0x00000000U;	// Delay Slot

	mipsCode[6] = 0x20000000U + (3U << 16U) + 2U;	// R3 = 2 // BGEZ taken, Landing Address

	mipsCode[7] = 0x00000008U;	// JR to Rs(0)
	mipsCode[8] = 0x00000000U;	// Delay Slot

	code_seg_t* seg = CompileCodeAt(mipsCode);

	segmentData->dbgCurrentSegment = seg;

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;
	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	// test branching when Rs == 0

	reg[2] = 0U;
	reg[3] = 0U;
	reg[4] = 0U;

	run();

	ASSERT_EQ(reg[3], 2U);
	ASSERT_EQ(reg[4], 3U);

	//---------------------------------------

	// test branching when Rs > 0

	reg[2] = 1U;
	reg[3] = 0U;
	reg[4] = 0U;

	run();

	ASSERT_EQ(reg[3], 2U);
	ASSERT_EQ(reg[4], 3U);

	//---------------------------------------
	// test not branching Rs < 0

	reg[2] = (uint64_t)-1;
	reg[3] = 0U;
	reg[4] = 0U;

	run();

	ASSERT_EQ(reg[3], 1U);
	ASSERT_EQ(reg[4], 3U);

	//---------------------------------------
	// Test looping

	//---------------------------------------

	if (seg->pBranchNext != NULL)
	{
		delSegment(seg->pBranchNext);
	}

	if (seg->pContinueNext != NULL)
	{
		delSegment(seg->pContinueNext);
	}

	printf("BGEZ    translated successfully\n");
}

static void TranslationTest_BGEZAL(code_segment_data_t* segmentData)
{
	mipsCode[0] = 0x00000000U; 						// Delay Slot
	mipsCode[1] = 0x04110000U + (2U << 21U) + 5U;	// BGEZAL
	mipsCode[2] = 0x20000000U + (4U << 16U) + 3U;	// Delay Slot

	mipsCode[3] = 0x20000000U + (3U << 16U) + (3U << 21U) + 1U;	// R3 += 1
	mipsCode[4] = 0x00000008U;	// JR to Rs(0)
	mipsCode[5] = 0x00000000U;	// Delay Slot

	mipsCode[6] = 0x20000000U + (3U << 16U) + (3U << 21U) + 2U;	// R3 += 2

	mipsCode[7] = 0x00000008U + (31U << 21U);		// PC = R31 (to Return)
	mipsCode[8] = 0x20000000U + (5U << 16U) + 1U;	// Delay Slot

	code_seg_t* seg = CompileCodeAt(mipsCode);

	segmentData->dbgCurrentSegment = seg;

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;
	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	// test branching when Rs == 0

	reg[2] = 0U;
	reg[3] = 0U;
	reg[4] = 0U;
	reg[5] = 0U;
	reg[31] = 0U;

	run();

	ASSERT_EQ(reg[3], 3U);
	ASSERT_EQ(reg[4], 3U);
	ASSERT_EQ(reg[5], 1U);
	ASSERT_EQ(reg[31], (size_t)&mipsCode[1] + 8U);

	//---------------------------------------

	// test branching when Rs > 0

	reg[2] = 1U;
	reg[3] = 0U;
	reg[4] = 0U;
	reg[5] = 0U;
	reg[31] = 0U;

	run();

	ASSERT_EQ(reg[3], 3U);
	ASSERT_EQ(reg[4], 3U);
	ASSERT_EQ(reg[5], 1U);
	ASSERT_EQ(reg[31], (size_t)&mipsCode[1] + 8U);

	//---------------------------------------
	// test not branching Rs < 0

	reg[2] = (uint64_t)-1;
	reg[3] = 0U;
	reg[4] = 0U;
	reg[5] = 0U;
	reg[31] = 0U;

	run();

	ASSERT_EQ(reg[3], 1U);
	ASSERT_EQ(reg[4], 3U);
	ASSERT_EQ(reg[5], 0U);
	ASSERT_EQ(reg[31], (size_t)&mipsCode[1] + 8U);

	//---------------------------------------
	// Test looping

	//---------------------------------------

	if (seg->pBranchNext != NULL)
	{
		delSegment(seg->pBranchNext);
	}

	if (seg->pContinueNext != NULL)
	{
		delSegment(seg->pContinueNext);
	}

	printf("BGEZAL  translated successfully\n");
}

static void TranslationTest_BGEZALL(code_segment_data_t* segmentData)
{
	mipsCode[0] = 0x00000000U; 						// Delay Slot
	mipsCode[1] = 0x04130000U + (2U << 21U) + 5U;	// BGEZALL
	mipsCode[2] = 0x20000000U + (4U << 16U) + 3U;	// Delay Slot

	mipsCode[3] = 0x20000000U + (3U << 16U) + (3U << 21U) + 1U;	// R3 += 1
	mipsCode[4] = 0x00000008U;	// JR to Rs(0)
	mipsCode[5] = 0x00000000U;	// Delay Slot

	mipsCode[6] = 0x20000000U + (3U << 16U) + (3U << 21U) + 2U;	// R3 += 2
	mipsCode[7] = 0x20000000U + (4U << 16U) + (4U << 21U) + 1U;	// R4 += 1

	mipsCode[8] = 0x00000008U + (31U << 21U);		// PC = R31 (to Return)
	mipsCode[9] = 0x20000000U + (5U << 16U) + 1U;	// Delay Slot

	code_seg_t* seg = CompileCodeAt(mipsCode);

	segmentData->dbgCurrentSegment = seg;

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;
	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	// test branching when Rs == 0

	reg[2] = 0U;
	reg[3] = 0U;
	reg[4] = 0U;
	reg[5] = 0U;
	reg[31] = 0U;

	run();

	ASSERT_EQ(reg[3], 3U);
	ASSERT_EQ(reg[4], 4U);
	ASSERT_EQ(reg[5], 1U);
	ASSERT_EQ(reg[31], (size_t)&mipsCode[1] + 8U);

	//---------------------------------------

	// test branching when Rs > 0

	reg[2] = 1U;
	reg[3] = 0U;
	reg[4] = 0U;
	reg[5] = 0U;
	reg[31] = 0U;

	run();

	ASSERT_EQ(reg[3], 3U);
	ASSERT_EQ(reg[4], 4U);
	ASSERT_EQ(reg[5], 1U);
	ASSERT_EQ(reg[31], (size_t)&mipsCode[1] + 8U);

	//---------------------------------------
	// test not branching Rs < 0

	reg[2] = (uint64_t)-1;
	reg[3] = 0U;
	reg[4] = 0U;
	reg[5] = 0U;
	reg[31] = 0U;

	run();

	ASSERT_EQ(reg[3], 1U);
	ASSERT_EQ(reg[4], 0U);
	ASSERT_EQ(reg[5], 0U);
	ASSERT_EQ(reg[31], (size_t)&mipsCode[1] + 8U);

	//---------------------------------------
	// Test looping

	//---------------------------------------

	if (seg->pBranchNext != NULL)
	{
		delSegment(seg->pBranchNext);
	}

	if (seg->pContinueNext != NULL)
	{
		delSegment(seg->pContinueNext);
	}

	printf("BGEZALL translated successfully\n");
}

static void TranslationTest_BGEZL(code_segment_data_t* segmentData)
{
	mipsCode[0] = 0x00000000U; 						// Delay Slot
	mipsCode[1] = 0x04030000U + (2U << 21U) + 5U;	// BGEZL
	mipsCode[2] = 0x20000000U + (4U << 16U) + 3U;	// Delay Slot

	mipsCode[3] = 0x20000000U + (3U << 16U) + 1U;	// R3 = 1 // BGEZ not taken
	mipsCode[4] = 0x00000008U;	// JR to Rs(0)
	mipsCode[5] = 0x00000000U;	// Delay Slot

	mipsCode[6] = 0x20000000U + (3U << 16U) + 2U;	// R3 = 2 // BGEZ taken, Landing Address

	mipsCode[7] = 0x00000008U;	// JR to Rs(0)
	mipsCode[8] = 0x00000000U;	// Delay Slot

	code_seg_t* seg = CompileCodeAt(mipsCode);

	segmentData->dbgCurrentSegment = seg;

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;
	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	// test branching when Rs == 0

	reg[2] = 0U;
	reg[3] = 0U;
	reg[4] = 0U;

	run();

	ASSERT_EQ(reg[3], 2U);
	ASSERT_EQ(reg[4], 3U);

	//---------------------------------------

	// test branching when Rs > 0

	reg[2] = 1U;
	reg[3] = 0U;
	reg[4] = 0U;

	run();

	ASSERT_EQ(reg[3], 2U);
	ASSERT_EQ(reg[4], 3U);

	//---------------------------------------
	// test not branching Rs < 0

	reg[2] = (uint64_t)-1;
	reg[3] = 0U;
	reg[4] = 0U;

	run();

	ASSERT_EQ(reg[3], 1U);
	ASSERT_EQ(reg[4], 0U);

	//---------------------------------------
	// Test looping

	//---------------------------------------

	if (seg->pBranchNext)
	{
		delSegment(seg->pBranchNext);
	}

	if (seg->pContinueNext)
	{
		delSegment(seg->pContinueNext);
	}

	printf("BGEZL   translated successfully\n");
}

static void TranslationTest_BGTZ(code_segment_data_t* segmentData)
{
	mipsCode[0] = 0x00000000U; 						// Delay Slot
	mipsCode[1] = 0x1C000000U + (2U << 21U) + 5U;	// BGTZ
	mipsCode[2] = 0x20000000U + (4U << 16U) + 3U;	// Delay Slot

	mipsCode[3] = 0x20000000U + (3U << 16U) + 1U;	// R3 = 1 // Branch not taken
	mipsCode[4] = 0x00000008U;	// JR to Rs(0)
	mipsCode[5] = 0x00000000U;	// Delay Slot

	mipsCode[6] = 0x20000000U + (3U << 16U) + 2U;	// R3 = 2 // Branch taken, Landing Address

	mipsCode[7] = 0x00000008U;	// JR to Rs(0)
	mipsCode[8] = 0x00000000U;	// Delay Slot

	code_seg_t* seg = CompileCodeAt(mipsCode);

	segmentData->dbgCurrentSegment = seg;

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;
	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	// test not branching when Rs == 0

	reg[2] = 0U;
	reg[3] = 0U;
	reg[4] = 0U;

	run();

	ASSERT_EQ(reg[3], 1U);
	ASSERT_EQ(reg[4], 3U);

	//---------------------------------------

	// test branching when Rs > 0

	reg[2] = 1U;
	reg[3] = 0U;
	reg[4] = 0U;

	run();

	ASSERT_EQ(reg[3], 2U);
	ASSERT_EQ(reg[4], 3U);

	//---------------------------------------
	// test not branching Rs < 0

	reg[2] = (uint64_t)-1;
	reg[3] = 0U;
	reg[4] = 0U;

	run();

	ASSERT_EQ(reg[3], 1U);
	ASSERT_EQ(reg[4], 3U);

	//---------------------------------------
	// Test looping

	//---------------------------------------

	if (seg->pBranchNext)
	{
		delSegment(seg->pBranchNext);
	}

	if (seg->pContinueNext)
	{
		delSegment(seg->pContinueNext);
	}

	printf("BGTZ    translated successfully\n");
}

static void TranslationTest_BGTZL(code_segment_data_t* segmentData)
{
	mipsCode[0] = 0x00000000U; 						// Delay Slot
	mipsCode[1] = 0x5C000000U + (2U << 21U) + 5U;	// BGTZL
	mipsCode[2] = 0x20000000U + (4U << 16U) + 3U;	// Delay Slot

	mipsCode[3] = 0x20000000U + (3U << 16U) + 1U;	// R3 = 1 // Branch not taken
	mipsCode[4] = 0x00000008U;	// JR to Rs(0)
	mipsCode[5] = 0x00000000U;	// Delay Slot

	mipsCode[6] = 0x20000000U + (3U << 16U) + 2U;	// R3 = 2 // Branch taken, Landing Address

	mipsCode[7] = 0x00000008U;	// JR to Rs(0)
	mipsCode[8] = 0x00000000U;	// Delay Slot

	code_seg_t* seg = CompileCodeAt(mipsCode);

	segmentData->dbgCurrentSegment = seg;

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;
	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	// test not branching when Rs == 0

	reg[2] = 0U;
	reg[3] = 0U;
	reg[4] = 0U;

	run();

	ASSERT_EQ(reg[3], 1U);
	ASSERT_EQ(reg[4], 0U);

	//---------------------------------------

	// test branching when Rs > 0

	reg[2] = 1U;
	reg[3] = 0U;
	reg[4] = 0U;

	run();

	ASSERT_EQ(reg[3], 2U);
	ASSERT_EQ(reg[4], 3U);

	//---------------------------------------
	// test not branching Rs < 0

	reg[2] = (uint64_t)-1;
	reg[3] = 0U;
	reg[4] = 0U;

	run();

	ASSERT_EQ(reg[3], 1U);
	ASSERT_EQ(reg[4], 0U);

	//---------------------------------------
	// Test looping

	//---------------------------------------

	if (seg->pBranchNext)
	{
		delSegment(seg->pBranchNext);
	}

	if (seg->pContinueNext)
	{
		delSegment(seg->pContinueNext);
	}

	printf("BGTZL   translated successfully\n");
}

static void TranslationTest_BLEZ(code_segment_data_t* segmentData)
{
	mipsCode[0] = 0x00000000U; 						// Delay Slot
	mipsCode[1] = 0x18000000U + (2U << 21U) + 5U;	// BLEZ
	mipsCode[2] = 0x20000000U + (4U << 16U) + 3U;	// Delay Slot

	mipsCode[3] = 0x20000000U + (3U << 16U) + 1U;	// R3 = 1 // Branch not taken
	mipsCode[4] = 0x00000008U;	// JR to Rs(0)
	mipsCode[5] = 0x00000000U;	// Delay Slot

	mipsCode[6] = 0x20000000U + (3U << 16U) + 2U;	// R3 = 2 // Branch taken, Landing Address

	mipsCode[7] = 0x00000008U;	// JR to Rs(0)
	mipsCode[8] = 0x00000000U;	// Delay Slot

	code_seg_t* seg = CompileCodeAt(mipsCode);

	segmentData->dbgCurrentSegment = seg;

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;
	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	// test branching when Rs == 0

	reg[2] = 0U;
	reg[3] = 0U;
	reg[4] = 0U;

	run();

	ASSERT_EQ(reg[3], 2U);
	ASSERT_EQ(reg[4], 3U);

	//---------------------------------------

	// test not branching when Rs > 0

	reg[2] = 1U;
	reg[3] = 0U;
	reg[4] = 0U;

	run();

	ASSERT_EQ(reg[3], 1U);
	ASSERT_EQ(reg[4], 3U);

	//---------------------------------------
	// test branching Rs < 0

	reg[2] = (uint64_t)-1;
	reg[3] = 0U;
	reg[4] = 0U;

	run();

	ASSERT_EQ(reg[3], 2U);
	ASSERT_EQ(reg[4], 3U);

	//---------------------------------------
	// Test looping

	//---------------------------------------

	if (seg->pBranchNext)
	{
		delSegment(seg->pBranchNext);
	}

	if (seg->pContinueNext)
	{
		delSegment(seg->pContinueNext);
	}

	printf("BLEZ    translated successfully\n");
}

static void TranslationTest_BLEZL(code_segment_data_t* segmentData)
{
	mipsCode[0] = 0x00000000U; 						// Delay Slot
	mipsCode[1] = 0x58000000U + (2U << 21U) + 5U;	// BLEZL
	mipsCode[2] = 0x20000000U + (4U << 16U) + 3U;	// Delay Slot

	mipsCode[3] = 0x20000000U + (3U << 16U) + 1U;	// R3 = 1 // Branch not taken
	mipsCode[4] = 0x00000008U;	// JR to Rs(0)
	mipsCode[5] = 0x00000000U;	// Delay Slot

	mipsCode[6] = 0x20000000U + (3U << 16U) + 2U;	// R3 = 2 // Branch taken, Landing Address

	mipsCode[7] = 0x00000008U;	// JR to Rs(0)
	mipsCode[8] = 0x00000000U;	// Delay Slot

	code_seg_t* seg = CompileCodeAt(mipsCode);

	segmentData->dbgCurrentSegment = seg;

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;
	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	// test branching when Rs == 0

	reg[2] = 0U;
	reg[3] = 0U;
	reg[4] = 0U;

	run();

	ASSERT_EQ(reg[3], 2U);
	ASSERT_EQ(reg[4], 3U);

	//---------------------------------------

	// test not branching when Rs > 0

	reg[2] = 1U;
	reg[3] = 0U;
	reg[4] = 0U;

	run();

	ASSERT_EQ(reg[3], 1U);
	ASSERT_EQ(reg[4], 0U);

	//---------------------------------------
	// test branching Rs < 0

	reg[2] = (uint64_t)-1;
	reg[3] = 0U;
	reg[4] = 0U;

	run();

	ASSERT_EQ(reg[3], 2U);
	ASSERT_EQ(reg[4], 3U);

	//---------------------------------------
	// Test looping

	//---------------------------------------

	if (seg->pBranchNext)
	{
		delSegment(seg->pBranchNext);
	}

	if (seg->pContinueNext)
	{
		delSegment(seg->pContinueNext);
	}

	printf("BLEZL   translated successfully\n");
}

static void TranslationTest_BLTZ(code_segment_data_t* segmentData)
{
	mipsCode[0] = 0x00000000U; 						// Delay Slot
	mipsCode[1] = 0x04000000U + (2U << 21U) + 5U;	// BLTZ
	mipsCode[2] = 0x20000000U + (4U << 16U) + 3U;	// Delay Slot

	mipsCode[3] = 0x20000000U + (3U << 16U) + 1U;	// R3 = 1 // Branch not taken
	mipsCode[4] = 0x00000008U;	// JR to Rs(0)
	mipsCode[5] = 0x00000000U;	// Delay Slot

	mipsCode[6] = 0x20000000U + (3U << 16U) + 2U;	// R3 = 2 // Branch taken, Landing Address

	mipsCode[7] = 0x00000008U;	// JR to Rs(0)
	mipsCode[8] = 0x00000000U;	// Delay Slot

	code_seg_t* seg = CompileCodeAt(mipsCode);

	segmentData->dbgCurrentSegment = seg;

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;
	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	// test not branching when Rs == 0

	reg[2] = 0U;
	reg[3] = 0U;
	reg[4] = 0U;

	run();

	ASSERT_EQ(reg[3], 1U);
	ASSERT_EQ(reg[4], 3U);

	//---------------------------------------

	// test not branching when Rs > 0

	reg[2] = 1U;
	reg[3] = 0U;
	reg[4] = 0U;

	run();

	ASSERT_EQ(reg[3], 1U);
	ASSERT_EQ(reg[4], 3U);

	//---------------------------------------
	// test branching Rs < 0

	reg[2] = (uint64_t)-1;
	reg[3] = 0U;
	reg[4] = 0U;

	run();

	ASSERT_EQ(reg[3], 2U);
	ASSERT_EQ(reg[4], 3U);

	//---------------------------------------
	// Test looping

	//---------------------------------------

	if (seg->pBranchNext)
	{
		delSegment(seg->pBranchNext);
	}

	if (seg->pContinueNext)
	{
		delSegment(seg->pContinueNext);
	}

	printf("BLTZ    translated successfully\n");
}

static void TranslationTest_BLTZAL(code_segment_data_t* segmentData)
{
	mipsCode[0] = 0x00000000U; 						// Delay Slot
	mipsCode[1] = 0x04100000U + (2U << 21U) + 5U;	// BLTZAL
	mipsCode[2] = 0x20000000U + (4U << 16U) + 3U;	// Delay Slot

	mipsCode[3] = 0x20000000U + (3U << 16U) + (3U << 21U) + 1U;	// R3 += 1
	mipsCode[4] = 0x00000008U;	// JR to Rs(0)
	mipsCode[5] = 0x00000000U;	// Delay Slot

	mipsCode[6] = 0x20000000U + (3U << 16U) + (3U << 21U) + 2U;	// R3 += 2

	mipsCode[7] = 0x00000008U + (31U << 21U);		// PC = R31 (to Return)
	mipsCode[8] = 0x20000000U + (5U << 16U) + 1U;	// Delay Slot

	code_seg_t* seg = CompileCodeAt(mipsCode);

	segmentData->dbgCurrentSegment = seg;

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;
	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	// test not branching when Rs == 0

	reg[2] = 0U;
	reg[3] = 0U;
	reg[4] = 0U;
	reg[5] = 0U;
	reg[31] = 0U;

	run();

	ASSERT_EQ(reg[3], 1U);
	ASSERT_EQ(reg[4], 3U);
	ASSERT_EQ(reg[5], 0U);
	ASSERT_EQ(reg[31], (size_t)&mipsCode[1] + 8U);

	//---------------------------------------

	// test not branching when Rs > 0

	reg[2] = 1U;
	reg[3] = 0U;
	reg[4] = 0U;
	reg[5] = 0U;
	reg[31] = 0U;

	run();

	ASSERT_EQ(reg[3], 1U);
	ASSERT_EQ(reg[4], 3U);
	ASSERT_EQ(reg[5], 0U);
	ASSERT_EQ(reg[31], (size_t)&mipsCode[1] + 8U);

	//---------------------------------------
	// test not branching Rs < 0

	reg[2] = (uint64_t)-1;
	reg[3] = 0U;
	reg[4] = 0U;
	reg[5] = 0U;
	reg[31] = 0U;

	run();

	ASSERT_EQ(reg[3], 3U);
	ASSERT_EQ(reg[4], 3U);
	ASSERT_EQ(reg[5], 1U);
	ASSERT_EQ(reg[31], (size_t)&mipsCode[1] + 8U);

	//---------------------------------------
	// Test looping

	//---------------------------------------

	if (seg->pBranchNext != NULL)
	{
		delSegment(seg->pBranchNext);
	}

	if (seg->pContinueNext != NULL)
	{
		delSegment(seg->pContinueNext);
	}

	printf("BLTZAL  translated successfully\n");
}

static void TranslationTest_BLTZALL(code_segment_data_t* segmentData)
{
	mipsCode[0] = 0x00000000U; 						// Delay Slot
	mipsCode[1] = 0x04120000U + (2U << 21U) + 5U;	// BLTZALL
	mipsCode[2] = 0x20000000U + (4U << 16U) + 3U;	// Delay Slot

	mipsCode[3] = 0x20000000U + (3U << 16U) + (3U << 21U) + 1U;	// R3 += 1
	mipsCode[4] = 0x00000008U;	// JR to Rs(0)
	mipsCode[5] = 0x00000000U;	// Delay Slot

	mipsCode[6] = 0x20000000U + (3U << 16U) + (3U << 21U) + 2U;	// R3 += 2
	mipsCode[7] = 0x20000000U + (4U << 16U) + (4U << 21U) + 1U;	// R4 += 1

	mipsCode[8] = 0x00000008U + (31U << 21U);		// PC = R31 (to Return)
	mipsCode[9] = 0x20000000U + (5U << 16U) + 1U;	// Delay Slot

	code_seg_t* seg = CompileCodeAt(mipsCode);

	segmentData->dbgCurrentSegment = seg;

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;
	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	// test not branching when Rs == 0

	reg[2] = 0U;
	reg[3] = 0U;
	reg[4] = 0U;
	reg[5] = 0U;
	reg[31] = 0U;

	run();

	ASSERT_EQ(reg[3], 1U);
	ASSERT_EQ(reg[4], 0U);
	ASSERT_EQ(reg[5], 0U);
	ASSERT_EQ(reg[31], (size_t)&mipsCode[1] + 8U);

	//---------------------------------------

	// test not branching when Rs > 0

	reg[2] = 1U;
	reg[3] = 0U;
	reg[4] = 0U;
	reg[5] = 0U;
	reg[31] = 0U;

	run();

	ASSERT_EQ(reg[3], 1U);
	ASSERT_EQ(reg[4], 0U);
	ASSERT_EQ(reg[5], 0U);
	ASSERT_EQ(reg[31], (size_t)&mipsCode[1] + 8U);

	//---------------------------------------
	// test branching Rs < 0

	reg[2] = (uint64_t)-1;
	reg[3] = 0U;
	reg[4] = 0U;
	reg[5] = 0U;
	reg[31] = 0U;

	run();

	ASSERT_EQ(reg[3], 3U);
	ASSERT_EQ(reg[4], 4U);
	ASSERT_EQ(reg[5], 1U);
	ASSERT_EQ(reg[31], (size_t)&mipsCode[1] + 8U);

	//---------------------------------------
	// Test looping

	//---------------------------------------

	if (seg->pBranchNext != NULL)
	{
		delSegment(seg->pBranchNext);
	}

	if (seg->pContinueNext != NULL)
	{
		delSegment(seg->pContinueNext);
	}

	printf("BLTZALL translated successfully\n");
}

static void TranslationTest_BLTZL(code_segment_data_t* segmentData)
{
	mipsCode[0] = 0x00000000U; 						// Delay Slot
	mipsCode[1] = 0x04020000U + (2U << 21U) + 5U;	// BLTZL
	mipsCode[2] = 0x20000000U + (4U << 16U) + 3U;	// Delay Slot

	mipsCode[3] = 0x20000000U + (3U << 16U) + 1U;	// R3 = 1 // Branch not taken
	mipsCode[4] = 0x00000008U;	// JR to Rs(0)
	mipsCode[5] = 0x00000000U;	// Delay Slot

	mipsCode[6] = 0x20000000U + (3U << 16U) + 2U;	// R3 = 2 // Branch taken, Landing Address

	mipsCode[7] = 0x00000008U;	// JR to Rs(0)
	mipsCode[8] = 0x00000000U;	// Delay Slot

	code_seg_t* seg = CompileCodeAt(mipsCode);

	segmentData->dbgCurrentSegment = seg;

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;
	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	// test not branching when Rs == 0

	reg[2] = 0U;
	reg[3] = 0U;
	reg[4] = 0U;

	run();

	ASSERT_EQ(reg[3], 1U);
	ASSERT_EQ(reg[4], 0U);

	//---------------------------------------

	// test not branching when Rs > 0

	reg[2] = 1U;
	reg[3] = 0U;
	reg[4] = 0U;

	run();

	ASSERT_EQ(reg[3], 1U);
	ASSERT_EQ(reg[4], 0U);

	//---------------------------------------
	// test branching Rs < 0

	reg[2] = (uint64_t)-1;
	reg[3] = 0U;
	reg[4] = 0U;

	run();

	ASSERT_EQ(reg[3], 2U);
	ASSERT_EQ(reg[4], 3U);

	//---------------------------------------
	// Test looping

	//---------------------------------------

	if (seg->pBranchNext)
	{
		delSegment(seg->pBranchNext);
	}

	if (seg->pContinueNext)
	{
		delSegment(seg->pContinueNext);
	}

	printf("BLTZL   translated successfully\n");
}

static void TranslationTest_BNE(code_segment_data_t* segmentData)
{
	mipsCode[0] = 0x00000000U; 						// Delay Slot
	mipsCode[1] = 0x14000000U + (2U << 21U) + (1 << 16U) + 5U;	// BNE
	mipsCode[2] = 0x20000000U + (4U << 16U) + 3U;	// Delay Slot

	mipsCode[3] = 0x20000000U + (3U << 16U) + 1U;	// R3 = 1 // Branch not taken
	mipsCode[4] = 0x00000008U;	// JR to Rs(0)
	mipsCode[5] = 0x00000000U;	// Delay Slot

	mipsCode[6] = 0x20000000U + (3U << 16U) + 2U;	// R3 = 2 // Branch taken, Landing Address

	mipsCode[7] = 0x00000008U;	// JR to Rs(0)
	mipsCode[8] = 0x00000000U;	// Delay Slot

	code_seg_t* seg = CompileCodeAt(mipsCode);

	segmentData->dbgCurrentSegment = seg;

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;
	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	// test not branching when R1 == R2

	reg[1] = 1U;
	reg[2] = 1U;
	reg[3] = 0U;
	reg[4] = 0U;

	run();

	ASSERT_EQ(reg[1], 1U);
	ASSERT_EQ(reg[3], 1U);
	ASSERT_EQ(reg[4], 3U);

	//---------------------------------------
	// test branching

	reg[1] = 1U;
	reg[2] = 0U;
	reg[3] = 0U;
	reg[4] = 0U;

	run();

	ASSERT_EQ(reg[1], 1U);
	ASSERT_EQ(reg[3], 2U);
	ASSERT_EQ(reg[4], 3U);

	//---------------------------------------
	// Test looping

	//---------------------------------------


	if (seg->pBranchNext)
	{
		delSegment(seg->pBranchNext);
	}

	if (seg->pContinueNext)
	{
		delSegment(seg->pContinueNext);
	}

	printf("BNE     translated successfully\n");
}

static void TranslationTest_BNEL(code_segment_data_t* segmentData)
{
	mipsCode[0] = 0x00000000U; 						// Delay Slot
	mipsCode[1] = 0x54000000U + (2U << 21U) + (1 << 16U) + 5U;	// BNEL
	mipsCode[2] = 0x20000000U + (4U << 16U) + 3U;	// Delay Slot

	mipsCode[3] = 0x20000000U + (3U << 16U) + 1U;	// R3 = 1 // Branch not taken
	mipsCode[4] = 0x00000008U;	// JR to Rs(0)
	mipsCode[5] = 0x00000000U;	// Delay Slot

	mipsCode[6] = 0x20000000U + (3U << 16U) + 2U;	// R3 = 2 // Branch taken, Landing Address

	mipsCode[7] = 0x00000008U;	// JR to Rs(0)
	mipsCode[8] = 0x00000000U;	// Delay Slot

	code_seg_t* seg = CompileCodeAt(mipsCode);

	segmentData->dbgCurrentSegment = seg;

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;
	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	// test not branching when R1 == R2

	reg[1] = 1U;
	reg[2] = 1U;
	reg[3] = 0U;
	reg[4] = 0U;

	run();

	ASSERT_EQ(reg[1], 1U);
	ASSERT_EQ(reg[3], 1U);
	ASSERT_EQ(reg[4], 0U);

	//---------------------------------------
	// test branching

	reg[1] = 1U;
	reg[2] = 0U;
	reg[3] = 0U;
	reg[4] = 0U;

	run();

	ASSERT_EQ(reg[1], 1U);
	ASSERT_EQ(reg[3], 2U);
	ASSERT_EQ(reg[4], 3U);

	//---------------------------------------
	// Test looping

	//---------------------------------------

	if (seg->pBranchNext)
	{
		delSegment(seg->pBranchNext);
	}

	if (seg->pContinueNext)
	{
		delSegment(seg->pContinueNext);
	}

	printf("BNEL    translated successfully\n");
}

#if TEST_BREAK
static void TranslationTest_BREAK(code_segment_data_t* segmentData)
{
	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;

	uint8_t rs = 1;
	uint8_t rt = 2;

	uint16_t imm = 0x10U;

	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;

	// MIPS ANDI Instruction
	mipsCode[0] = 0x0000000D + (imm << 6U);
	mipsCode[1] = 0x00000008;	// JR to Rs(0)
	mipsCode[2] = 0x00000000;	// Delay Slot

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	run();

	//---------------------------------------

	delSegment(seg);
	printf("BREAK   translated successfully\n");
}
#endif

static void TranslationTest_DADD(code_segment_data_t* segmentData)
{
	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;

	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;

	// MIPS DADD Instruction
	mipsCode[0] = 0x0000002C + (3U << 21U) + (2U << 16U) + (1U << 11U);
	mipsCode[1] = 0x00000008;	// JR to Rs(0)
	mipsCode[2] = 0x00000000;	// Delay Slot

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -1000;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	reg[1] = 0x0U;
	reg[2] = 0x0000000100000001U;
	reg[3] = 0x0000000300000003U;

	run();

	ASSERT_EQ(reg[1], 0x0000000400000004U);

	//---------------------------------------
	// Test for overflow trap

	reg[1] = 0U;
	reg[2] = 0x7000000000000000U;
	reg[3] = 0x1000000000000000U;

	run();

	ASSERT_EQ(reg[1], 0U);
	ASSERT_EQ(reg[2], 0x7000000000000000U);
	ASSERT_EQ(reg[3], 0x1000000000000000U);
	//---------------------------------------
	// register writeback

	mipsCode[0] = 0x0000002C + (1U << 21U) + (2U << 16U) + (2U << 11U);
	Translate(seg);
	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	reg[1] = 0x0000000100000001U;
	reg[2] = 0x0000000300000003U;

	run();

	ASSERT_EQ(reg[2], 0x0000000400000004U);

	//---------------------------------------
	// register writeback

	mipsCode[0] = 0x0000002C + (2U << 21U) + (1U << 16U) + (2U << 11U);
	Translate(seg);
	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	reg[1] = 0x0000000100000001U;
	reg[2] = 0x0000000300000003U;

	run();

	ASSERT_EQ(reg[2], 0x0000000400000004U);

	//---------------------------------------

	delSegment(seg);
	printf("DADD    translated successfully\n");
}

static void TranslationTest_DADDI(code_segment_data_t* segmentData)
{
	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;

	uint8_t rs = 1U;
	uint8_t rt = 2U;

	uint32_t imm = 0x0021U;

	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;

	// MIPS DADDI Instruction
	mipsCode[0] = 0x60000000 + (rs << 21U) + (rt << 16U) + imm;
	mipsCode[1] = 0x00000008;	// JR to Rs(0)
	mipsCode[2] = 0x00000000;	// Delay Slot

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	reg[rs] = 0x111111111111U;

	run();

	ASSERT_EQ((0x111111111111U + imm), reg[rt]);
	ASSERT_EQ((0x111111111111U), reg[rs]);

	//---------------------------------------

	imm = 0x4321U;

	// MIPS DADDI Instruction
	mipsCode[0] = 0x60000000 + (rs << 21U) + (rt << 16U) + imm;

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	reg[rs] = 0x111111111111U;
	reg[rt] = 0x00000000U;

	run();

	ASSERT_EQ((0x111111111111U + imm), reg[rt]);
	ASSERT_EQ((0x111111111111U), reg[rs]);

	//---------------------------------------

	imm = 0x8000U;
	mipsCode[0] = 0x60000000 + (rs << 21U) + (rt << 16U) + imm;
	Translate(seg);
	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	reg[rs] = 0x111111111111U;

	run();

	ASSERT_EQ((0x111111111111U - imm), reg[rt]);
	ASSERT_EQ((0x111111111111U), reg[rs]);

	//---------------------------------------
	// register writeback

	imm = 0x8000U;
	mipsCode[0] = 0x60000000 + (1 << 21U) + (1 << 16U) + imm;
	Translate(seg);
	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	reg[1] = 0x111111111111U;

	run();

	ASSERT_EQ((0x111111111111U - imm), reg[1]);

	//---------------------------------------

	delSegment(seg);
	printf("DADDI   translated successfully\n");
}

static void TranslationTest_DADDIU(code_segment_data_t* segmentData)
{
	// Description: rt ← rs + immediate
	// To add a constant to a 64-bit integer.
	// The 16-bit signed immediate is added to the 64-bit value in GPR rs and the 64-bit
	// arithmetic result is placed into GPR rt.
	// No Integer Overflow exception occurs under any circumstances.

	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;

	uint8_t rs = 1;
	uint8_t rt = 2;
	int64_t imm = 0x1111;


	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;

	// MIPS DADDIU Instruction
	mipsCode[0] = 0x64000000 + (rs << 21) + (rt << 16) + (uint16_t)imm ;
	mipsCode[1] = 0x00000008;	// JR to Rs(0)
	mipsCode[2] = 0x00000000;	// Delay Slot

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	reg[rs] = 0x111111110000U;
	reg[rt] = 0x0U;

	run();

	ASSERT_EQ(0x111111110000U + imm, reg[rt]);

	//---------------------------------------

	imm = -1;

	// MIPS DADDIU Instruction
	mipsCode[0] = 0x64000000 + (rs << 21) + (rt << 16) + (uint16_t)imm ;

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	reg[rs] = 0x111111110000U;
	reg[rt] = 0x0U;

	run();

	ASSERT_EQ(reg[rs] + imm, reg[rt]);

	//---------------------------------------
	// register writeback

	imm = -1;

	// MIPS DADDIU Instruction
	mipsCode[0] = 0x64000000 + (1 << 21) + (1 << 16) + (uint16_t)imm ;

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	reg[1] = 0x111111110000U;

	run();

	ASSERT_EQ(0x111111110000U + imm, reg[1]);

	//---------------------------------------

	delSegment(seg);
	printf("DADDIU  translated successfully\n");
}

static void TranslationTest_DADDU(code_segment_data_t* segmentData)
{
	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;

	uint8_t rs = 1;
	uint8_t rt = 2;
	uint8_t rd = 3;

	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;

	// MIPS DADDU Instruction
	mipsCode[0] = 0x0000002D + (rs << 21) + (rt << 16) + (rd << 11);
	mipsCode[1] = 0x00000008;	// JR to Rs(0)
	mipsCode[2] = 0x00000000;	// Delay Slot

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;


	//---------------------------------------

	reg[rs] = 0x123456780000;
	reg[rt] = 0x765432101111;

	run();

	ASSERT_EQ(((reg[rs] + reg[rt])), reg[rd]);

	//---------------------------------------
	// register writeback

	mipsCode[0] = 0x0000002D + (1 << 21) + (2 << 16) + (2 << 11);
	Translate(seg);
	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	reg[1] = 0x123456780000;
	reg[2] = 0x765432101111;

	run();

	ASSERT_EQ(((0x123456780000 + 0x765432101111)), reg[2]);

	//---------------------------------------
	// register writeback

	mipsCode[0] = 0x0000002D + (2 << 21) + (1 << 16) + (2 << 11);
	Translate(seg);
	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	reg[1] = 0x123456780000;
	reg[2] = 0x765432101111;

	run();

	ASSERT_EQ(((0x123456780000 + 0x765432101111)), reg[2]);

	//---------------------------------------

	delSegment(seg);
	printf("DADDU   translated successfully\n");
}

static void TranslationTest_DDIV(code_segment_data_t* segmentData)
{
	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;

	uint8_t rs = 1U;
	uint8_t rt = 2U;

	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;

	// MIPS DDIV Instruction
	mipsCode[0] = 0x0000001E + (rs << 21U) + (rt << 16U);
	mipsCode[1] = 0x00000008;	// JR to Rs(0)
	mipsCode[2] = 0x00000000;	// Delay Slot

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	reg[rs] = 0x1111111111111111U;
	reg[rt] = 0x3U;

	run();

	ASSERT_EQ(multiLo, (0x1111111111111111ULL / 3ULL));
	ASSERT_EQ(multiHi, (0x1111111111111111ULL % 3ULL));

	//---------------------------------------

	reg[rs] = 0x1111111111111111U;
	reg[rt] = (uint64_t)(-3);

	run();

	ASSERT_EQ(multiLo, (int64_t)(0x1111111111111111LL) / (int64_t)-3 );
	ASSERT_EQ(multiHi, (int64_t)(0x1111111111111111LL) % (int64_t)-3 );

	//---------------------------------------

	delSegment(seg);
	printf("DDIV    translated successfully\n");
}

static void TranslationTest_DDIVU(code_segment_data_t* segmentData)
{
	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;

	uint8_t rs = 1U;
	uint8_t rt = 2U;

	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;

	// MIPS DDIVU Instruction
	mipsCode[0] = 0x0000001F + (rs << 21U) + (rt << 16U);
	mipsCode[1] = 0x00000008;	// JR to Rs(0)
	mipsCode[2] = 0x00000000;	// Delay Slot

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	reg[rs] = 0x1111111111111111U;
	reg[rt] = 0x3U;

	run();

	ASSERT_EQ(multiLo, (0x1111111111111111ULL / 3ULL));
	ASSERT_EQ(multiHi, (0x1111111111111111ULL % 3ULL));
	//---------------------------------------

	reg[rs] = 0x1111111111111111U;
	reg[rt] = (uint64_t)(-3);

	run();

	ASSERT_EQ(multiLo, (0x1111111111111111ULL) / ((uint64_t)-3) );
	ASSERT_EQ(multiHi, (0x1111111111111111ULL) % ((uint64_t)-3) );
	//---------------------------------------

	delSegment(seg);
	printf("DDIVU   translated successfully\n");
}

static void TranslationTest_DIV(code_segment_data_t* segmentData)
{
	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;

	uint8_t rs = 1;
	uint8_t rt = 2;

	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;

	// MIPS DIV Instruction
	mipsCode[0] = 0x0000001A + (rs << 21U) + (rt << 16U);
	mipsCode[1] = 0x00000008;	// JR to Rs(0)
	mipsCode[2] = 0x00000000;	// Delay Slot

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	reg[rs] = 0x11111111U;
	reg[rt] = 0x3U;

	run();

	ASSERT_EQ(multiLo, (0x11111111U / 3U));
	ASSERT_EQ(multiHi, (0x11111111U % 3U));

	//---------------------------------------

	delSegment(seg);
	printf("DIV     translated successfully\n");
}

static void TranslationTest_DIVU(code_segment_data_t* segmentData)
{
	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;

	uint8_t rs = 1;
	uint8_t rt = 2;

	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;

	// MIPS DIVU Instruction
	mipsCode[0] = 0x0000001B + (rs << 21U) + (rt << 16U);
	mipsCode[1] = 0x00000008;	// JR to Rs(0)
	mipsCode[2] = 0x00000000;	// Delay Slot

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	reg[rs] = 0x11111111U;
	reg[rt] = 0x3U;
	run();

	ASSERT_EQ(multiLo, (0x11111111U / 3U));
	ASSERT_EQ(multiHi, (0x11111111U % 3U));

	//---------------------------------------

	delSegment(seg);
	printf("DIVU    translated successfully\n");
}

static void TranslationTest_DMULT(code_segment_data_t* segmentData)
{
	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;

	uint8_t rs = 1U;
	uint8_t rt = 2U;

	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;

	// MIPS DMULT Instruction
	mipsCode[0] = 0x0000001CU + (rs << 21U) + (rt << 16U);
	mipsCode[1] = 0x00000008U;	// JR to Rs(0)
	mipsCode[2] = 0x00000000U;	// Delay Slot

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	reg[rs] = 0x0000000000000002U;
	reg[rt] = 0x0000000000000002U;

	run();

	ASSERT_EQ(multiLo, 4U);
	ASSERT_EQ(multiHi, 0U);

	//---------------------------------------

	reg[rs] = 0x0000000000000002U;
	reg[rt] = 0x0000000200000000U;

	run();

	ASSERT_EQ(multiLo, 0x400000000U);
	ASSERT_EQ(multiHi, 0U);

	//---------------------------------------

	reg[rs] = 0x0000000200000000U;
	reg[rt] = 0x0000000000000002U;

	run();

	ASSERT_EQ(multiLo, 0x400000000U);
	ASSERT_EQ(multiHi, 0);

	//---------------------------------------

	reg[rs] = 0x0000000200000000U;
	reg[rt] = 0x0000000200000000U;

	run();

	ASSERT_EQ(multiLo, 0);
	ASSERT_EQ(multiHi, 4);

	//---------------------------------------

	reg[rs] = (uint64_t)-2;
	reg[rt] = 0x0000000000000002U;

	run();

	ASSERT_EQ(multiLo, (uint64_t)-4);
	ASSERT_EQ(multiHi, 0xFFFFFFFFFFFFFFFFU);

	//---------------------------------------

	delSegment(seg);
	printf("DMULT   translated successfully\n");
}

static void TranslationTest_DMULTU(code_segment_data_t* segmentData)
{
	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;

	uint8_t rs = 1U;
	uint8_t rt = 2U;

	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;

	// MIPS DMULTU Instruction
	mipsCode[0] = 0x0000001DU + (rs << 21U) + (rt << 16U);
	mipsCode[1] = 0x00000008U;	// JR to Rs(0)
	mipsCode[2] = 0x00000000U;	// Delay Slot

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	reg[rs] = 0x0000000000000002U;
	reg[rt] = 0x0000000000000002U;

	run();

	ASSERT_EQ(multiLo, 4U);
	ASSERT_EQ(multiHi, 0U);

	//---------------------------------------

	reg[rs] = 0x0000000000000002U;
	reg[rt] = 0x0000000200000000U;

	run();

	ASSERT_EQ(multiLo, 0x400000000U);
	ASSERT_EQ(multiHi, 0U);

	//---------------------------------------

	reg[rs] = 0x0000000200000000U;
	reg[rt] = 0x0000000000000002U;

	run();

	ASSERT_EQ(multiLo, 0x400000000U);
	ASSERT_EQ(multiHi, 0);

	//---------------------------------------

	reg[rs] = 0x0000000200000000U;
	reg[rt] = 0x0000000200000000U;

	run();

	ASSERT_EQ(multiLo, 0);
	ASSERT_EQ(multiHi, 4);

	//---------------------------------------

	delSegment(seg);
	printf("DMULTU  translated successfully\n");
}

static void TranslationTest_DSLL(code_segment_data_t* segmentData)
{
	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;

	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;

	// MIPS DSLL Instruction
	mipsCode[0] = 0x00000038 + (2U << 16U) + (1U << 11U) + (3U << 6U);
	mipsCode[1] = 0x00000008;	// JR to Rs(0)
	mipsCode[2] = 0x00000000;	// Delay Slot

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	reg[1] = 0U;
	reg[2] = 0x0003000000030000U;

	run();

	ASSERT_EQ(reg[2], 0x0003000000030000U);
	ASSERT_EQ(reg[1], (0x0003000000030000U) << 3U );

	//---------------------------------------
	// Crosses over 32-bit boundary

	reg[1] = 0U;
	reg[2] = 0x0000000080000000U;

	run();

	ASSERT_EQ(reg[2], 0x0000000080000000U);
	ASSERT_EQ(reg[1], (0x0000000080000000ULL) << 3U );

	//---------------------------------------
	// register writeback

	mipsCode[0] = 0x00000038 + (1U << 16U) + (1U << 11U) + (3U << 6U);
	Translate(seg);
	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	reg[1] = 0x1000000080000001U;

	run();

	ASSERT_EQ(reg[1], (0x1000000080000001ULL) << 3U );

	//---------------------------------------

	delSegment(seg);

	printf("DSLL    translated successfully\n");
}

static void TranslationTest_DSLL32(code_segment_data_t* segmentData)
{
	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;

	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;

	// MIPS DSLL32 Instruction
	mipsCode[0] = 0x0000003C + (2U << 16U) + (1U << 11U) + (3U << 6U);
	mipsCode[1] = 0x00000008;	// JR to Rs(0)
	mipsCode[2] = 0x00000000;	// Delay Slot

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	reg[1] = 0U;
	reg[2] = 0x0003000000030000U;

	run();

	ASSERT_EQ(reg[2], 0x0003000000030000ULL);
	ASSERT_EQ(reg[1], (0x0003000000030000ULL) << 35U );

	//---------------------------------------
	// Crosses over 32-bit boundary

	reg[1] = 0U;
	reg[2] = 0x0000000000000001U;

	run();

	ASSERT_EQ(reg[2], 0x0000000000000001U);
	ASSERT_EQ(reg[1], (0x0000000000000001ULL) << 35U );

	//---------------------------------------
	// register writeback

	mipsCode[0] = 0x0000003C + (1U << 16U) + (1U << 11U) + (3U << 6U);
	Translate(seg);
	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	reg[1] = 0x0003000000030000U;

	run();

	ASSERT_EQ(reg[1], (0x0003000000030000ULL) << 35U );

	//---------------------------------------

	delSegment(seg);
	printf("DSLL32  translated successfully\n");
}

static void TranslationTest_DSLLV(code_segment_data_t* segmentData)
{
	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;

	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;

	// MIPS DSLLV Instruction
	mipsCode[0] = 0x00000014 + (2U << 21U) + (1U << 16U) + (3U << 11U);	// R3 = R1 << R2
	mipsCode[1] = 0x00000008;	// JR to Rs(0)
	mipsCode[2] = 0x00000000;	// Delay Slot

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	reg[1] = 0x0000000000000001U;
	reg[2] = 0x0000000000000002U;
	reg[3] = 0x0000000000000000U;

	run();

	ASSERT_EQ(reg[1], 0x0000000000000001U);
	ASSERT_EQ(reg[2], 0x0000000000000002U);
	ASSERT_EQ(reg[3], reg[1] << reg[2]);


	//---------------------------------------
	// Crosses over 32-bit boundary, shift == 32

	reg[1] = 0x0000000000000001U;
	reg[2] = 0x0000000000000020U;
	reg[3] = 0x0000000000000000U;

	run();

	ASSERT_EQ(reg[1], 0x0000000000000001U);
	ASSERT_EQ(reg[2], 0x0000000000000020U);
	ASSERT_EQ(reg[3], reg[1] << reg[2]);

	//---------------------------------------
	// Crosses over 32-bit boundary, shift > 32

	reg[1] = 0x0000000000000001U;
	reg[2] = 0x0000000000000021U;
	reg[3] = 0x0000000000000000U;

	run();

	ASSERT_EQ(reg[1], 0x0000000000000001U);
	ASSERT_EQ(reg[2], 0x0000000000000021U);
	ASSERT_EQ(reg[3], reg[1] << reg[2]);

	//---------------------------------------
	// Register writeback

	mipsCode[0] = 0x00000014 + (2U << 21U) + (1U << 16U) + (2U << 11U);	// R2 = R1 << R2

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;


	reg[1] = 0x0000000000000001U;
	reg[2] = 0x0000000000000020U;

	run();

	ASSERT_EQ(reg[1], 0x0000000000000001U);
	ASSERT_EQ(reg[2], 0x0000000000000001ULL << 0x0000000000000020U);

	//---------------------------------------
	// Register writeback

	mipsCode[0] = 0x00000014 + (2U << 21U) + (1U << 16U) + (1U << 11U);	// R2 = R1 << R2

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	reg[1] = 0x0000000000000001U;
	reg[2] = 0x0000000000000020U;

	run();

	ASSERT_EQ(reg[2], 0x0000000000000020U);
	ASSERT_EQ(reg[1], 0x0000000000000001ULL << 0x0000000000000020U);

	//---------------------------------------
	// Register writeback

	mipsCode[0] = 0x00000014 + (2U << 21U) + (1U << 16U) + (2U << 11U);	// R2 = R1 << R2

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	reg[1] = 0x0000000000000001U;
	reg[2] = 0x0000000000000002U;

	run();

	ASSERT_EQ(reg[1], 0x0000000000000001U);
	ASSERT_EQ(reg[2], 0x0000000000000001U << 0x0000000000000002U);

	//---------------------------------------
	// Register writeback

	mipsCode[0] = 0x00000014 + (2U << 21U) + (1U << 16U) + (1U << 11U);	// R2 = R1 << R2

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	reg[1] = 0x0000000000000001U;
	reg[2] = 0x0000000000000002U;

	run();

	ASSERT_EQ(reg[2], 0x000000000000002U);
	ASSERT_EQ(reg[1], 0x0000000000000001U << 0x0000000000000002U);

	//---------------------------------------

	delSegment(seg);
	printf("DSLLV   translated successfully\n");
}

static void TranslationTest_DSRA(code_segment_data_t* segmentData)
{
	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;

	uint8_t rt = 1;
	uint8_t rd = 2;

	uint8_t imm = 0x3U;

	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;

	// MIPS DSRA Instruction
	mipsCode[0] = 0x0000003B + (rt << 16U) + (rd << 11U) + (imm << 6U);
	mipsCode[1] = 0x00000008;	// JR to Rs(0)
	mipsCode[2] = 0x00000000;	// Delay Slot

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -1000;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	reg[rt] = (uint64_t)-753;
	reg[rd] = 0;

	run();

	ASSERT_EQ(reg[rd], -95);

	//---------------------------------------
	// register writeback

	mipsCode[0] = 0x0000003B + (1 << 16U) + (1 << 11U) + (imm << 6U);
	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;


	reg[1] = (uint64_t)-753;

	run();

	ASSERT_EQ(reg[1], -95);

	//---------------------------------------

	delSegment(seg);
	printf("DSRA    translated successfully\n");
}

static void TranslationTest_DSRA32(code_segment_data_t* segmentData)
{
	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;

	uint8_t rd = 1;
	uint8_t rt = 2;

	uint8_t imm = 0x3U;

	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;

	// MIPS DRSA32 Instruction
	mipsCode[0] = 0x0000003FU + (rt << 16U) + (rd << 11U) + (imm << 6U);
	mipsCode[1] = 0x00000008U;	// JR to Rs(0)
	mipsCode[2] = 0x00000000U;	// Delay Slot

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	reg[rt] = (uint64_t)(-95LL << 35);
	reg[rd] = 0;

	run();

	ASSERT_EQ(reg[rd], -95);

	//---------------------------------------
	// register writeback
	mipsCode[0] = 0x0000003FU + (1 << 16U) + (1 << 11U) + (imm << 6U);
	Translate(seg);
	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	reg[1] = (uint64_t)(-95LL << 35);

	run();

	ASSERT_EQ(reg[1], -95);

	//---------------------------------------

	delSegment(seg);
	printf("DSRA32  translated successfully\n");
}

static void TranslationTest_DSRAV(code_segment_data_t* segmentData)
{
	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;

	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;

	// MIPS DSRAV Instruction
	mipsCode[0] = 0x00000017 + (1 << 21U) + (2 << 16U) + (3 << 11U);
	mipsCode[1] = 0x00000008;	// JR to Rs(0)
	mipsCode[2] = 0x00000000;	// Delay Slot

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	reg[1] = 0x3U;
	reg[2] = (uint64_t)-16;
	reg[3] = 0U;

	run();

	ASSERT_EQ(reg[1], 0x3U);
	ASSERT_EQ(reg[2], (uint64_t)-16LL);
	ASSERT_EQ(reg[3], (uint64_t)-2LL);

	//---------------------------------------

	reg[1] = 35U;
	reg[2] = (uint64_t)(-2LL) << 35U;
	reg[3] = 0U;

	run();

	ASSERT_EQ(reg[1], 35U);
	ASSERT_EQ(reg[2], (uint64_t)(-2LL) << 35U);
	ASSERT_EQ(reg[3], (uint64_t)-2LL);


	//---------------------------------------
	// register writeback

	mipsCode[0] = 0x00000017 + (1 << 21U) + (2 << 16U) + (2 << 11U);
	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	reg[2] = (uint64_t)-16;
	reg[1] = 0x3U;

	run();

	ASSERT_EQ(reg[2], (uint64_t)-2LL);
	ASSERT_EQ(reg[1], (uint64_t)3);

	//---------------------------------------
	// register writeback

	mipsCode[0] = 0x00000017 + (2 << 21U) + (1 << 16U) + (2 << 11U);
	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	reg[1] = (uint64_t)-16LL;
	reg[2] = 3U;

	run();

	ASSERT_EQ(reg[2], (uint64_t)-2LL);
	ASSERT_EQ(reg[1], (uint64_t)-16LL);

	//---------------------------------------

	delSegment(seg);
	printf("DSRAV   translated successfully\n");
}

static void TranslationTest_DSRL(code_segment_data_t* segmentData)
{
	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;

	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;

	// MIPS DSRL Instruction
	mipsCode[0] = 0x0000003AU + (1 << 16U) + (2 << 11U) + (3U << 6U);
	mipsCode[1] = 0x00000008U;	// JR to Rs(0)
	mipsCode[2] = 0x00000000U;	// Delay Slot

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	reg[1] = 0x0000001600000016ULL;
	reg[2] = 0U;

	run();

	ASSERT_EQ(reg[1], 0x0000001600000016ULL);
	ASSERT_EQ(reg[2], (reg[1] >> 3U));

	//---------------------------------------

	reg[1] = (uint32_t)-16LL;
	reg[2] = 0U;

	run();

	ASSERT_EQ(reg[1], (uint32_t)-16LL);
	ASSERT_EQ(reg[2], (reg[1] >> 3U));

	//---------------------------------------
	// register writeback
	mipsCode[0] = 0x0000003AU + (2 << 16U) + (2 << 11U) + (3U << 6U);
	Translate(seg);
	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	reg[2] = 0x0000001600000016ULL;

	run();

	ASSERT_EQ(reg[2], (0x0000001600000016ULL >> 3U));

	//---------------------------------------

	delSegment(seg);
	printf("DSRL    translated successfully\n");
}

static void TranslationTest_DSRL32(code_segment_data_t* segmentData)
{
	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;

	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;

	// MIPS DSRL32 Instruction
	mipsCode[0] = 0x0000003E + (1 << 16U) + (2 << 11U) + (3U << 6U);
	mipsCode[1] = 0x00000008;	// JR to Rs(0)
	mipsCode[2] = 0x00000000;	// Delay Slot

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	reg[1] = 0x0100000001000000ULL;
	reg[2] = 0U;

	run();

	ASSERT_EQ(reg[1], 0x0100000001000000ULL);
	ASSERT_EQ(reg[2], (reg[1] >> 35U));

	//---------------------------------------

	reg[1] = 0x8100000001000000ULL;
	reg[2] = 0U;

	run();

	ASSERT_EQ(reg[1], 0x8100000001000000ULL);
	ASSERT_EQ(reg[2], (reg[1] >> 35U));

	//---------------------------------------
	// register writeback

	mipsCode[0] = 0x0000003E + (1 << 16U) + (1 << 11U) + (3U << 6U);
	Translate(seg);
	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	reg[1] = 0x0100000001000000ULL;

	run();

	ASSERT_EQ(reg[1], (0x0100000001000000ULL >> 35U));

	//---------------------------------------

	delSegment(seg);
	printf("DSRL32  translated successfully\n");
}

static void TranslationTest_DSRLV(code_segment_data_t* segmentData)
{
	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;

	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;

	// MIPS DSRLV Instruction
	mipsCode[0] = 0x00000016U + (1 << 21U) + (2 << 16U) + (3 << 11U);
	mipsCode[1] = 0x00000008;	// JR to Rs(0)
	mipsCode[2] = 0x00000000;	// Delay Slot

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	reg[2] = 0x0100000001000000ULL;
	reg[1] = 3U;
	reg[3] = 0U;

	run();

	ASSERT_EQ(reg[2], 0x0100000001000000ULL);
	ASSERT_EQ(reg[1], 3U);
	ASSERT_EQ(reg[3], (reg[2] >> reg[1]));

	//---------------------------------------

	reg[2] = 0x0100000001000000ULL;
	reg[1] = 35U;
	reg[3] = 0U;

	run();

	ASSERT_EQ(reg[2], 0x0100000001000000ULL);
	ASSERT_EQ(reg[1], 35U);
	ASSERT_EQ(reg[3], (reg[2] >> reg[1]));

	//---------------------------------------
	// register writeback

	mipsCode[0] = 0x00000016U + (1 << 21U) + (2 << 16U) + (2 << 11U);

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;


	reg[2] = 0x0100000001000000ULL;
	reg[1] = 3U;
	reg[3] = 0U;

	run();

	ASSERT_EQ(reg[1], 3U);
	ASSERT_EQ(reg[2], (0x0100000001000000ULL >> 3U));

	//---------------------------------------
	// register writeback

	mipsCode[0] = 0x00000016U + (2 << 21U) + (1 << 16U) + (2 << 11U);

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	reg[1] = 0x0100000001000000ULL;
	reg[2] = 3U;

	run();

	ASSERT_EQ(reg[1], 0x0100000001000000ULL);
	ASSERT_EQ(reg[2], (0x0100000001000000ULL >> 3U));

	//---------------------------------------
	delSegment(seg);
	printf("DSRLV   translated successfully\n");
}

static void TranslationTest_DSUB(code_segment_data_t* segmentData)
{
	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;

	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;

	// MIPS DSUB Instruction
	mipsCode[0] = 0x0000002E + (3 << 21U) + (2 << 16U) + (1 << 11U);
	mipsCode[1] = 0x00000008;	// JR to Rs(0)
	mipsCode[2] = 0x00000000;	// Delay Slot

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	reg[1] = 0x0U;
	reg[2] = 0x0000000100000001U;
	reg[3] = 0x0000000300000003U;

	run();

	ASSERT_EQ(reg[1], reg[3] - reg[2]);

	//---------------------------------------
	// Test for overflow trap

	reg[1] = 0U;
	reg[2] = 0x7000000000000002U;
	reg[3] = 0x8000000000000001U;

	run();

	ASSERT_EQ(reg[1], 0U);
	ASSERT_EQ(reg[2], 0x7000000000000002U);
	ASSERT_EQ(reg[3], 0x8000000000000001U);

	//---------------------------------------
	// register writeback

	mipsCode[0] = 0x0000002E + (1U << 21U) + (2U << 16U) + (2U << 11U);
	Translate(seg);
	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	reg[2] = 0x0000000100000001U;
	reg[1] = 0x0000000300000003U;

	run();

	ASSERT_EQ(reg[2], 0x0000000200000002U);

	//---------------------------------------
	// register writeback

	mipsCode[0] = 0x0000002E + (2U << 21U) + (1U << 16U) + (2U << 11U);
	Translate(seg);
	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	reg[1] = 0x0000000100000001U;
	reg[2] = 0x0000000300000003U;

	run();

	ASSERT_EQ(reg[2], 0x0000000200000002U);

	//---------------------------------------

	delSegment(seg);
	printf("DSUB    translated successfully\n");
}

static void TranslationTest_DSUBU(code_segment_data_t* segmentData)
{
	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;

	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;

	// MIPS DSUBU Instruction
	mipsCode[0] = 0x0000002FU + (3U << 21U) + (2U << 16U) + (1U << 11U);
	mipsCode[1] = 0x00000008U;	// JR to Rs(0)
	mipsCode[2] = 0x00000000U;	// Delay Slot

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	reg[1] = 0x0U;
	reg[2] = 0x0000000100000001U;
	reg[3] = 0x0000000300000003U;

	run();

	ASSERT_EQ(reg[1], reg[3] - reg[2]);

	//---------------------------------------
	// Test for overflow

	reg[1] = 0U;
	reg[2] = 0xF000000000000002U;
	reg[3] = 0x8000000000000001U;

	run();

	ASSERT_EQ(reg[1], reg[3] - reg[2]);
	ASSERT_EQ(reg[2], 0xF000000000000002U);
	ASSERT_EQ(reg[3], 0x8000000000000001U);

	//---------------------------------------
	// register writeback

	mipsCode[0] = 0x0000002F + (1U << 21U) + (2U << 16U) + (2U << 11U);
	Translate(seg);
	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	reg[2] = 0x0000000100000001U;
	reg[1] = 0x0000000300000003U;

	run();

	ASSERT_EQ(reg[2], 0x0000000200000002U);

	//---------------------------------------
	// register writeback

	mipsCode[0] = 0x0000002F + (2U << 21U) + (1U << 16U) + (2U << 11U);
	Translate(seg);
	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	reg[1] = 0x0000000100000001U;
	reg[2] = 0x0000000300000003U;

	run();

	ASSERT_EQ(reg[2], 0x0000000200000002U);

	//---------------------------------------

	delSegment(seg);
	printf("DSUBU   translated successfully\n");
}

static void TranslationTest_J(code_segment_data_t* segmentData)
{
	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;

	uint32_t imm = 0x08000040U >> 2U;

	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;


	mipsCode[0] = 0x20000000 + (1 << 21U) + (1 << 16U) + 1;
	mipsCode[1] = 0x08000000 + imm;  // MIPS J Instruction
	mipsCode[2] = 0x00000000;	// Delay Slot
	mipsCode[3] = 0x00000008;	// JR to Rs(0)
	mipsCode[4] = 0x00000000;	// Delay Slot

	mipsCode[(imm&0xFF) - 1] = 0x20000000 + (3 << 21U) + (3 << 16U) + 1;
	mipsCode[(imm&0xFF) + 0] = 0x20000000 + (2 << 21U) + (2 << 16U) + 1;
	mipsCode[(imm&0xFF) + 1] = 0x00000008;	// JR to Rs(0)
	mipsCode[(imm&0xFF) + 2] = 0x00000000;	// Delay Slot

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	reg[1] = 0U;
	reg[2] = 0U;
	reg[3] = 0U;

	run();

	ASSERT_EQ(reg[1], 1);
	ASSERT_EQ(reg[2], 1);
	ASSERT_EQ(reg[3], 0);

	//---------------------------------------

	if (seg->pBranchNext)
	{
		delSegment(seg->pBranchNext);
	}

	delSegment(seg);

	printf("J       translated successfully\n");
}

static void TranslationTest_JAL(code_segment_data_t* segmentData)
{
	uint32_t imm = 0x08000040U >> 2U;

	// MIPS JAL Instruction
	mipsCode[0] = 0x20000000U + (1U << 21U) + (1U << 16U) + 1U;
	mipsCode[1] = 0x0C000000U + imm;	//JAL
	mipsCode[2] = 0x20000000U + (4U << 21U) + (4U << 16U) + 1U;
	mipsCode[3] = 0x00000008U;	// JR to Rs(0)
	mipsCode[4] = 0x00000000U;	// Delay Slot

	mipsCode[(imm&0xFFU) - 1U] = 0x20000000U + (3U << 21U) + (3U << 16U) + 1U;
	mipsCode[(imm&0xFFU) + 0U] = 0x20000000U + (2U << 21U) + (2U << 16U) + 1U;
	mipsCode[(imm&0xFFU) + 1U] = 0x00000008U + (31U << 21U);	// JR to R31
	mipsCode[(imm&0xFFU) + 2U] = 0x00000000U;	// Delay Slot

	code_seg_t* seg = CompileCodeAt(mipsCode);
	segmentData->dbgCurrentSegment = seg;

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	reg[1] = 0U;
	reg[2] = 0U;
	reg[3] = 0U;
	reg[4] = 0U;
	reg[31] = 0U;

	run();

	ASSERT_EQ(reg[1], 1);
	ASSERT_EQ(reg[2], 1);
	ASSERT_EQ(reg[3], 0);
	ASSERT_EQ(reg[4], 1);
	ASSERT_EQ(reg[31], (uint32_t)&mipsCode[3]);

	//---------------------------------------

	// TODO test when the delay slot is within the next 256MB memory block

	if (seg->pBranchNext != NULL)
	{
		delSegment(seg->pBranchNext);
	}

	if (seg->pContinueNext != NULL)
	{
		delSegment(seg->pContinueNext);
	}

	printf("JAL     translated successfully\n");
}

static void TranslationTest_JALR(code_segment_data_t* segmentData)
{
	uint32_t imm = 0x88000040U >> 2U;

	// MIPS JALR Instruction
	mipsCode[0] = 0x20000000U + (1U << 21U) + (1U << 16U) + 1U;
	mipsCode[1] = 0x00000009U + (3U << 21U) + (31U << 16U);			// JALR, PC = R3, LR = R4
	mipsCode[2] = 0x00000000U;	// Delay Slot
	mipsCode[3] = 0x20000000U + (2U << 21U) + (2U << 16U) + 1U;
	mipsCode[4] = 0x00000008U;	// JR to Rs(0)
	mipsCode[5] = 0x00000000U;	// Delay Slot

	mipsCode[(imm&0xFFU) - 1U] = 0x20000000U + (5U << 21U) + (5U << 16U) + 1U;
	mipsCode[(imm&0xFFU) + 0U] = 0x20000000U + (6U << 21U) + (6U << 16U) + 1U;
	mipsCode[(imm&0xFFU) + 1U] = 0x00000008U + (31U << 21U);	// JR to R31
	mipsCode[(imm&0xFFU) + 2U] = 0x00000000U;	// Delay Slot

	code_seg_t* seg = CompileCodeAt(mipsCode);
	segmentData->dbgCurrentSegment = seg;

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;
	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	reg[1] = 0U;
	reg[2] = 0U;
	reg[3] = imm << 2U;
	reg[4] = 31U;
	reg[5] = 0U;
	reg[6] = 0U;

	run();

	ASSERT_EQ(reg[1], 1U);
	ASSERT_EQ(reg[2], 1U);
	ASSERT_EQ(reg[3], imm << 2U);
	ASSERT_EQ(reg[31], (uint32_t)&mipsCode[3]);
	ASSERT_EQ(reg[5], 0U);
	ASSERT_EQ(reg[6], 1U);

	//---------------------------------------

	delSegment(seg);
	delSegment(getSegmentAt((uintptr_t)&mipsCode[(imm&0xFFU) + 0U]));

	printf("JALR    translated successfully\n");
}

static void TranslationTest_JR(code_segment_data_t* segmentData)
{
	uint32_t imm = 0x88000040U >> 2U;

	// MIPS JR Instruction
	mipsCode[0] = 0x20000000U + (4U << 21U) + (4U << 16U) + 1U;
	mipsCode[1] = 0x00000008 + (1 << 21U);		// JR R1
	mipsCode[2] = 0x00000000;	// Delay Slot

	mipsCode[(imm&0xFFU) - 1U] = 0x20000000 + (3 << 21U) + (3 << 16U) + 1;
	mipsCode[(imm&0xFFU) + 0U] = 0x20000000 + (2 << 21U) + (2 << 16U) + 1;
	mipsCode[(imm&0xFFU) + 1U] = 0x00000008;	// JR to Rs(0)
	mipsCode[(imm&0xFFU) + 2U] = 0x00000000;	// Delay Slot

	code_seg_t* seg = CompileCodeAt(mipsCode);
	segmentData->dbgCurrentSegment = seg;

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;
	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	reg[1] = imm << 2U;
	reg[2] = 0U;
	reg[3] = 0U;

	run();

	ASSERT_EQ(reg[2], 1U);
	ASSERT_EQ(reg[3], 0U);
	//---------------------------------------

	delSegment(seg);
	delSegment(getSegmentAt((uintptr_t)&mipsCode[(imm&0xFFU) + 0U]));

	printf("JR      translated successfully\n");
}

static void TranslationTest_LB(code_segment_data_t* segmentData)
{
	// MIPS LB Instruction
	mipsCode[64] = 0x80000000U + (1U << 21U) + (2U << 16U) + (uint16_t)16;
	mipsCode[65] = 0x20000000U + (2U << 21U) + (2U << 16U) + 0;				// ADD R2 = R2 + 0 (Force register save)
	mipsCode[66] = 0x00000008U;	// JR to Rs(0)
	mipsCode[67] = 0x00000000U;	// Delay Slot
	mipsCode[68] = 0x00000123U;	// Data to load

	code_seg_t* seg = CompileCodeAt(&mipsCode[64]);

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;
	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	reg[1] = (uint32_t)seg->MIPScode;

	//---------------------------------------

	reg[2] = 0U;

	run();

	ASSERT_EQ(reg[2], 0x23U);

	//---------------------------------------

	mipsCode[68] = 0x00000183U;	// Data to load
	reg[2] = 0U;

	run();

	ASSERT_EQ(reg[2], 0xFFFFFFFFFFFFFF83ULL);

	//---------------------------------------


	mipsCode[128] = 0x00000134U;	// Data to load
	mipsCode[64] = 0x80000000U + (1U << 21U) + (2U << 16U) + (uint16_t)256;

	delSegment(seg);
	seg = CompileCodeAt(&mipsCode[64]);
	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	reg[2] = 0U;

	run();

	ASSERT_EQ(reg[2], 0x34U);

	//---------------------------------------

	mipsCode[0] = 0x00000145U;	// Data to load
	mipsCode[64] = 0x80000000U + (1U << 21U) + (2U << 16U) + (((uint16_t)-256)&0xFFFFU);

	delSegment(seg);
	seg = CompileCodeAt(&mipsCode[64]);
	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	reg[2] = 0U;

	run();

	ASSERT_EQ(reg[2], 0x45U);

	delSegment(seg);
	printf("LB      translated successfully\n");
}

static void TranslationTest_LBU(code_segment_data_t* segmentData)
{
	// MIPS LBU Instruction
	mipsCode[64] = 0x90000000U + (1U << 21U) + (2U << 16U) + (uint16_t)16;
	mipsCode[65] = 0x20000000U + (2U << 21U) + (2U << 16U) + 0;				// ADD R2 = R2 + 0 (Force register save)
	mipsCode[66] = 0x00000008U;	// JR to Rs(0)
	mipsCode[67] = 0x00000000U;	// Delay Slot
	mipsCode[68] = 0x00000123U;	// Data to load

	code_seg_t* seg = CompileCodeAt(&mipsCode[64]);

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;
	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	reg[1] = (uint32_t)seg->MIPScode;

	//---------------------------------------

	reg[2] = 0U;

	run();

	ASSERT_EQ(reg[2], 0x23U);

	//---------------------------------------

	mipsCode[68] = 0x00000183U;	// Data to load
	reg[2] = 0U;

	run();

	ASSERT_EQ(reg[2], 0x83ULL);

	//---------------------------------------
	// Large positive offset

	mipsCode[128] = 0x00000134U;	// Data to load
	mipsCode[64] = 0x90000000U + (1U << 21U) + (2U << 16U) + (uint16_t)256;

	delSegment(seg);
	seg = CompileCodeAt(&mipsCode[64]);
	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	reg[2] = 0U;

	run();

	ASSERT_EQ(reg[2], 0x34U);

	//---------------------------------------
	// Large negative offset

	mipsCode[0] = 0x00000145U;	// Data to load
	mipsCode[64] = 0x90000000U + (1U << 21U) + (2U << 16U) + (((uint16_t)-256)&0xFFFFU);

	delSegment(seg);
	seg = CompileCodeAt(&mipsCode[64]);
	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	reg[2] = 0U;

	run();

	ASSERT_EQ(reg[2], 0x45U);

	delSegment(seg);
	printf("LBU     translated successfully\n");
}

static void TranslationTest_LD(code_segment_data_t* segmentData)
{
	// MIPS LD Instruction
	mipsCode[64] = 0xDC000000U + (1U << 21U) + (2U << 16U) + (uint16_t)16;
	mipsCode[65] = 0x60000000U + (2U << 21U) + (2U << 16U) + 0;				// DADDI R2 = R2 + 0 (Force register save)
	mipsCode[66] = 0x00000008U;	// JR to Rs(0)
	mipsCode[67] = 0x00000000U;	// Delay Slot
	mipsCode[68] = 0x12345678U;	// Data to load
	mipsCode[69] = 0x9ABCDEF0U;	// Data to load

	code_seg_t* seg = CompileCodeAt(&mipsCode[64]);

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;
	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	reg[1] = (uint32_t)seg->MIPScode;

	//---------------------------------------

	reg[2] = 0U;

	run();

	ASSERT_EQ(reg[2], 0x123456789ABCDEF0ULL);

	//---------------------------------------

	mipsCode[128] = 0x00000134U;	// Data to load
	mipsCode[129] = 0x00000432U;	// Data to load
	mipsCode[64] = 0xDC000000U + (1U << 21U) + (2U << 16U) + (uint16_t)256;


	delSegment(seg);
	seg = CompileCodeAt(&mipsCode[64]);
	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	reg[2] = 0U;

	run();

	ASSERT_EQ(reg[2], 0x0000013400000432ULL);

	//---------------------------------------

	mipsCode[0] = 0x00000145U;	// Data to load
	mipsCode[1] = 0x00000541U;	// Data to load
	mipsCode[64] = 0xDC000000U + (1U << 21U) + (2U << 16U) + (((uint16_t)-256)&0xFFFFU);

	delSegment(seg);
	seg = CompileCodeAt(&mipsCode[64]);
	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	reg[2] = 0U;

	run();

	ASSERT_EQ(reg[2], 0x0000014500000541ULL);

	delSegment(seg);
	printf("LD      translated successfully\n");
}

#if TEST_COPROCESSOR
static void TranslationTest_LDCz(code_segment_data_t* segmentData)
{
	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;

	uint8_t rs = 1;
	uint8_t rt = 2;

	uint16_t imm = 0x10U;

	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;

	// MIPS LDCz Instruction
	mipsCode[0] = 0x30000000 + (rs << 21U) + (rt << 16U) + (uint16_t)imm;
	mipsCode[1] = 0x00000008;	// JR to Rs(0)
	mipsCode[2] = 0x00000000;	// Delay Slot

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	reg[rs] = 0x1111111111111111U;

	run();

	ASSERT_EQ(1, 0);


	//---------------------------------------

	delSegment(seg);
	printf("LDCz translated successfully\n");
}
#endif

static void TranslationTest_LDL(code_segment_data_t* segmentData)
{
	// MIPS LDL Instruction
	mipsCode[64] = 0x68000000U + (1U << 21U) + (2U << 16U) + (uint16_t)16;
	mipsCode[65] = 0x60000000U + (2U << 21U) + (2U << 16U) + 0;				// DADDI R2 = R2 + 0 (Force register save)
	mipsCode[66] = 0x00000008U;	// JR to Rs(0)
	mipsCode[67] = 0x00000000U;	// Delay Slot

	mipsCode[68] = 0x04030201U;	// Data to load
	mipsCode[69] = 0x08070605U;	// Data to load
	mipsCode[70] = 0x0C0B0A09U;	// Data to load
	mipsCode[71] = 0x000F0E0DU;	// Data to load

	uint32_t i;

	for (i = 0U; i < 8U; i++)
	{
		mipsCode[64] = 0x68000000U + (1U << 21U) + (2U << 16U) + (uint16_t)(16U + i);

		code_seg_t* seg = CompileCodeAt(&mipsCode[64]);

		*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;
		*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

		reg[1] = (uint32_t)seg->MIPScode;
		reg[2] = 0x7060504030201000ULL;

		run();

		uint64_t update_mask= ((uint64_t)(-1) << (i * 8U));
		uint8_t* data = (uint8_t*)&mipsCode[68];
		uint64_t mem_data = *((uint64_t*)&data[i]);

		ASSERT_EQ(reg[2], (0x7060504030201000ULL & ~update_mask) | (mem_data & update_mask));

		delSegment(seg);
	}

	printf("LDL     translated successfully\n");
}

static void TranslationTest_LDR(code_segment_data_t* segmentData)
{
	// MIPS LDR Instruction
	mipsCode[64] = 0x6C000000U + (1U << 21U) + (2U << 16U) + (uint16_t)16;
	mipsCode[65] = 0x60000000U + (2U << 21U) + (2U << 16U) + 0;				// DADDI R2 = R2 + 0 (Force register save)
	mipsCode[66] = 0x00000008U;	// JR to Rs(0)
	mipsCode[67] = 0x00000000U;	// Delay Slot

	mipsCode[68] = 0x04030201U;	// Data to load
	mipsCode[69] = 0x08070605U;	// Data to load
	mipsCode[70] = 0x0C0B0A09U;	// Data to load
	mipsCode[71] = 0x000F0E0DU;	// Data to load

	uint32_t i;

	for (i = 0U; i < 8U; i++)
	{
		mipsCode[64] = 0x6C000000U + (1U << 21U) + (2U << 16U) + (uint16_t)(16U + i);

		code_seg_t* seg = CompileCodeAt(&mipsCode[64]);

		*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;
		*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

		reg[1] = (uint32_t)seg->MIPScode;
		reg[2] = 0x7060504030201000ULL;

		uint64_t update_mask= ((uint64_t)(-1) >> ((7 - i) * 8U));
		uint8_t* data = (uint8_t*)&mipsCode[68];
		uint64_t mem_data = *((uint64_t*)&data[7 - i]);

		printf("%d    0x%016llX    0x%016llX\n", i, mem_data, update_mask);

		run();

		ASSERT_EQ(reg[2], (0x7060504030201000ULL & ~update_mask) | (mem_data & update_mask));

		delSegment(seg);
	}

	printf("LDR     translated successfully\n");
}

static void TranslationTest_LH(code_segment_data_t* segmentData)
{
	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;

	uint8_t rs = 1;
	uint8_t rt = 2;

	uint16_t imm = 0x10U;

	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;

	// MIPS LH Instruction
	mipsCode[0] = 0x30000000 + (rs << 21U) + (rt << 16U) + (uint16_t)imm;
	mipsCode[1] = 0x00000008;	// JR to Rs(0)
	mipsCode[2] = 0x00000000;	// Delay Slot

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	reg[rs] = 0x1111111111111111U;

	run();

	ASSERT_EQ(1, 0);


	//---------------------------------------

	delSegment(seg);
	printf("LH      translated successfully\n");
}

static void TranslationTest_LHU(code_segment_data_t* segmentData)
{
	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;

	uint8_t rs = 1;
	uint8_t rt = 2;

	uint16_t imm = 0x10U;

	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;

	// MIPS LHU Instruction
	mipsCode[0] = 0x30000000 + (rs << 21U) + (rt << 16U) + (uint16_t)imm;
	mipsCode[1] = 0x00000008;	// JR to Rs(0)
	mipsCode[2] = 0x00000000;	// Delay Slot

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	reg[rs] = 0x1111111111111111U;

	run();

	ASSERT_EQ(1, 0);


	//---------------------------------------

	delSegment(seg);
	printf("LHU     translated successfully\n");
}

static void TranslationTest_LL(code_segment_data_t* segmentData)
{
	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;

	uint8_t rs = 1;
	uint8_t rt = 2;

	uint16_t imm = 0x10U;

	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;

	// MIPS LL Instruction
	mipsCode[0] = 0x30000000 + (rs << 21U) + (rt << 16U) + (uint16_t)imm;
	mipsCode[1] = 0x00000008;	// JR to Rs(0)
	mipsCode[2] = 0x00000000;	// Delay Slot

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	reg[rs] = 0x1111111111111111U;

	run();

	ASSERT_EQ(1, 0);


	//---------------------------------------

	delSegment(seg);
	printf("LL      translated successfully\n");
}

static void TranslationTest_LLD(code_segment_data_t* segmentData)
{
	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;

	uint8_t rs = 1;
	uint8_t rt = 2;

	uint16_t imm = 0x10U;

	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;

	// MIPS LLD Instruction
	mipsCode[0] = 0x30000000 + (rs << 21U) + (rt << 16U) + (uint16_t)imm;
	mipsCode[1] = 0x00000008;	// JR to Rs(0)
	mipsCode[2] = 0x00000000;	// Delay Slot

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	reg[rs] = 0x1111111111111111U;

	run();

	ASSERT_EQ(1, 0);


	//---------------------------------------

	delSegment(seg);
	printf("LLD     translated successfully\n");
}

static void TranslationTest_LUI(code_segment_data_t* segmentData)
{
	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;

	uint8_t rs = 1;
	uint8_t rt = 2;

	uint16_t imm = 0x10U;

	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;

	// MIPS LUI Instruction
	mipsCode[0] = 0x30000000 + (rs << 21U) + (rt << 16U) + (uint16_t)imm;
	mipsCode[1] = 0x00000008;	// JR to Rs(0)
	mipsCode[2] = 0x00000000;	// Delay Slot

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	reg[rs] = 0x1111111111111111U;

	run();

	ASSERT_EQ(1, 0);


	//---------------------------------------

	delSegment(seg);
	printf("LUI     translated successfully\n");
}

static void TranslationTest_LW(code_segment_data_t* segmentData)
{
	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;

	uint8_t rs = 1;
	uint8_t rt = 2;

	uint16_t imm = 0x10U;

	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;

	// MIPS LW Instruction
	mipsCode[0] = 0x30000000 + (rs << 21U) + (rt << 16U) + (uint16_t)imm;
	mipsCode[1] = 0x00000008;	// JR to Rs(0)
	mipsCode[2] = 0x00000000;	// Delay Slot

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	reg[rs] = 0x1111111111111111U;

	run();

	ASSERT_EQ(1, 0);


	//---------------------------------------

	delSegment(seg);
	printf("LW      translated successfully\n");
}

#if TEST_COPROCESSOR
static void TranslationTest_LWCz(code_segment_data_t* segmentData)
{
	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;

	uint8_t rs = 1;
	uint8_t rt = 2;

	uint16_t imm = 0x10U;

	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;

	// MIPS LWCz Instruction
	mipsCode[0] = 0x30000000 + (rs << 21U) + (rt << 16U) + (uint16_t)imm;
	mipsCode[1] = 0x00000008;	// JR to Rs(0)
	mipsCode[2] = 0x00000000;	// Delay Slot

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	reg[rs] = 0x1111111111111111U;

	run();

	ASSERT_EQ(1, 0);


	//---------------------------------------

	delSegment(seg);
	printf("LWCz    translated successfully\n");
}
#endif

static void TranslationTest_LWL(code_segment_data_t* segmentData)
{
	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;

	uint8_t rs = 1;
	uint8_t rt = 2;

	uint16_t imm = 0x10U;

	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;

	// MIPS LWL Instruction
	mipsCode[0] = 0x30000000 + (rs << 21U) + (rt << 16U) + (uint16_t)imm;
	mipsCode[1] = 0x00000008;	// JR to Rs(0)
	mipsCode[2] = 0x00000000;	// Delay Slot

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	reg[rs] = 0x1111111111111111U;

	run();

	ASSERT_EQ(1, 0);


	//---------------------------------------

	delSegment(seg);
	printf("LWL     translated successfully\n");
}

static void TranslationTest_LWR(code_segment_data_t* segmentData)
{
	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;

	uint8_t rs = 1;
	uint8_t rt = 2;

	uint16_t imm = 0x10U;

	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;

	// MIPS LWR Instruction
	mipsCode[0] = 0x30000000 + (rs << 21U) + (rt << 16U) + (uint16_t)imm;
	mipsCode[1] = 0x00000008;	// JR to Rs(0)
	mipsCode[2] = 0x00000000;	// Delay Slot

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	reg[rs] = 0x1111111111111111U;

	run();

	ASSERT_EQ(1, 0);


	//---------------------------------------

	delSegment(seg);
	printf("LWR     translated successfully\n");
}

static void TranslationTest_LWU(code_segment_data_t* segmentData)
{
	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;

	uint8_t rs = 1;
	uint8_t rt = 2;

	uint16_t imm = 0x10U;

	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;

	// MIPS LWU Instruction
	mipsCode[0] = 0x30000000 + (rs << 21U) + (rt << 16U) + (uint16_t)imm;
	mipsCode[1] = 0x00000008;	// JR to Rs(0)
	mipsCode[2] = 0x00000000;	// Delay Slot

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	reg[rs] = 0x1111111111111111U;

	run();

	ASSERT_EQ(1, 0);


	//---------------------------------------

	delSegment(seg);
	printf("LWU     translated successfully\n");
}

static void TranslationTest_MFHI(code_segment_data_t* segmentData)
{
	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;

	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;

	// MIPS MFHI Instruction
	mipsCode[0] = 0x00000010U + (1U << 11U);
	mipsCode[1] = 0x00000008U;	// JR to Rs(0)
	mipsCode[2] = 0x00000000U;	// Delay Slot

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------
	// 32-bit move

	reg[1] = 0U;
	*((volatile uint64_t*)MMAP_FP_BASE + REG_MULTHI) = 0x00000001U;

	run();

	ASSERT_EQ(reg[1], 0x00000001U);

	//---------------------------------------
	// 64-bit move

	reg[1] = 0U;
	*((volatile uint64_t*)MMAP_FP_BASE + REG_MULTHI) = 0x0123456789ABCDEFU;

	run();

	ASSERT_EQ(reg[1], 0x0123456789ABCDEFU);

	//---------------------------------------

	delSegment(seg);
	printf("MFHI    translated successfully\n");
}

static void TranslationTest_MFLO(code_segment_data_t* segmentData)
{
	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;

	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;

	// MIPS MFLO Instruction
	mipsCode[0] = 0x00000012U + (1 << 11U);
	mipsCode[1] = 0x00000008U;	// JR to Rs(0)
	mipsCode[2] = 0x00000000U;	// Delay Slot

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------
	// 32-bit move

	reg[1] = 0U;
	*((volatile uint64_t*)MMAP_FP_BASE + REG_MULTLO) = 0x00000001U;

	run();

	ASSERT_EQ(reg[1], 0x00000001U);

	//---------------------------------------
	// 64-bit move

	reg[1] = 0U;
	*((volatile uint64_t*)MMAP_FP_BASE + REG_MULTLO) = 0x0123456789ABCDEFU;

	run();

	ASSERT_EQ(reg[1], 0x0123456789ABCDEFU);

	//---------------------------------------

	delSegment(seg);
	printf("MFLO    translated successfully\n");
}

static void TranslationTest_MOVN(code_segment_data_t* segmentData)
{
	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;

	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;

	// MIPS MOVN Instruction
	mipsCode[0] = 0x0000000BU + (3U << 21U) + (2U << 16U) + (1U << 11U);
	mipsCode[1] = 0x00000008U;	// JR to Rs(0)
	mipsCode[2] = 0x00000000U;	// Delay Slot

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	reg[1] = 0U;
	reg[2] = 1U;
	reg[3] = 0x3U;

	run();

	ASSERT_EQ(reg[1], 0x3U);

	//---------------------------------------

	reg[1] = 0U;
	reg[2] = 0U;
	reg[3] = 0x3U;

	run();

	ASSERT_EQ(reg[1], 0U);

	//---------------------------------------

	delSegment(seg);
	printf("MOVN    translated successfully\n");
}

static void TranslationTest_MOVZ(code_segment_data_t* segmentData)
{
	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;

	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;

	// MIPS MOVN Instruction
	mipsCode[0] = 0x0000000AU + (3U << 21U) + (2U << 16U) + (1U << 11U);
	mipsCode[1] = 0x00000008U;	// JR to Rs(0)
	mipsCode[2] = 0x00000000U;	// Delay Slot

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	reg[1] = 0U;
	reg[2] = 0U;
	reg[3] = 0x3U;

	run();

	ASSERT_EQ(reg[1], 0x3U);

	//---------------------------------------

	reg[1] = 0U;
	reg[2] = 1U;
	reg[3] = 0x3U;

	run();

	ASSERT_EQ(reg[1], 0U);

	//---------------------------------------

	delSegment(seg);
	printf("MOVZ    translated successfully\n");
}

static void TranslationTest_MTHI(code_segment_data_t* segmentData)
{
	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;

	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;

	// MIPS MTHI Instruction
	mipsCode[0] = 0x00000011U + (1U << 21U);
	mipsCode[1] = 0x00000008U;	// JR to Rs(0)
	mipsCode[2] = 0x00000000U;	// Delay Slot

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------
	// 32-bit move
	reg[1] = 0x0000000000000001U;

	run();

	ASSERT_EQ(*((volatile uint64_t*)MMAP_FP_BASE + REG_MULTHI), 0x0000000000000001U);

	//---------------------------------------
	// 64-bit move
	reg[1] = 0x0000000100000001U;

	run();

	ASSERT_EQ(*((volatile uint64_t*)MMAP_FP_BASE + REG_MULTHI), 0x0000000100000001U);

	//---------------------------------------

	delSegment(seg);
	printf("MTHI    translated successfully\n");
}

static void TranslationTest_MTLO(code_segment_data_t* segmentData)
{
	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;

	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;

	// MIPS MTLO Instruction
	mipsCode[0] = 0x00000013U + (1U << 21U);
	mipsCode[1] = 0x00000008U;	// JR to Rs(0)
	mipsCode[2] = 0x00000000U;	// Delay Slot

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------
	// 32-bit move

	reg[1] = 0x0000000000000001U;

	run();

	ASSERT_EQ(*((volatile uint64_t*)MMAP_FP_BASE + REG_MULTLO), 0x0000000000000001U);

	//---------------------------------------
	// 64-bit move

	reg[1] = 0x0000000100000001U;

	run();

	ASSERT_EQ(*((volatile uint64_t*)MMAP_FP_BASE + REG_MULTLO), 0x0000000100000001U);

	//---------------------------------------

	delSegment(seg);
	printf("MTLO    translated successfully\n");
}

static void TranslationTest_MULT(code_segment_data_t* segmentData)
{
	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;

	uint8_t rs = 1U;
	uint8_t rt = 2U;

	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;

	// MIPS MULT Instruction
	mipsCode[0] = 0x00000018U + (rs << 21U) + (rt << 16U);
	mipsCode[1] = 0x00000008U;	// JR to Rs(0)
	mipsCode[2] = 0x00000000U;	// Delay Slot

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	reg[1] = 0x10000000U;
	reg[2] = 0x10000000U;

	run();

	ASSERT_EQ(*((volatile uint64_t*)MMAP_FP_BASE + REG_MULTLO), 0x10000000ULL * 0x10000000ULL);
	ASSERT_EQ(*((volatile uint64_t*)MMAP_FP_BASE + REG_MULTHI), 0);

	//---------------------------------------

	reg[1] = (uint64_t)-3;
	reg[2] = (uint64_t)-3;

	run();

	ASSERT_EQ(*((volatile uint64_t*)MMAP_FP_BASE + REG_MULTLO), 9U);
	ASSERT_EQ(*((volatile uint64_t*)MMAP_FP_BASE + REG_MULTHI), 0);

	//---------------------------------------

	delSegment(seg);
	printf("MULT    translated successfully\n");
}

static void TranslationTest_MULTU(code_segment_data_t* segmentData)
{
	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;

	uint8_t rs = 1U;
	uint8_t rt = 2U;

	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;

	// MIPS MULTU Instruction
	mipsCode[0] = 0x00000019U + (rs << 21U) + (rt << 16U);
	mipsCode[1] = 0x00000008U;	// JR to Rs(0)
	mipsCode[2] = 0x00000000U;	// Delay Slot

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	reg[rs] = 0x1111111111111111U;

	run();

	ASSERT_EQ(1, 0);

	//---------------------------------------

	delSegment(seg);
	printf("MULTU   translated successfully\n");
}

static void TranslationTest_NOR(code_segment_data_t* segmentData)
{
	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;

	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;

	// MIPS NOR Instruction
	mipsCode[0] = 0x00000027U + (3U << 21U) + (2U << 16U) + (1U << 11U);
	mipsCode[1] = 0x00000008U;	// JR to Rs(0)
	mipsCode[2] = 0x00000000U;	// Delay Slot

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	reg[1] = 0U;
	reg[2] = 3U;
	reg[3] = 6U;

	run();

	ASSERT_EQ(reg[1], (3ULL | (!(6ULL))));

	//---------------------------------------

	delSegment(seg);
	printf("NOR     translated successfully\n");
}

static void TranslationTest_OR(code_segment_data_t* segmentData)
{
	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;

	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;

	// MIPS OR Instruction
	mipsCode[0] = 0x00000025U + (3U << 21U) + (2U << 16U) + (1U << 16U);
	mipsCode[1] = 0x00000008U;	// JR to Rs(0)
	mipsCode[2] = 0x00000000U;	// Delay Slot

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	reg[1] = 8U;
	reg[2] = 3U;
	reg[3] = 6U;

	run();

	ASSERT_EQ(reg[1], (3U | 6U));

	//---------------------------------------

	delSegment(seg);
	printf("OR      translated successfully\n");
}

static void TranslationTest_ORI(code_segment_data_t* segmentData)
{
	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;

	uint8_t rs = 1;
	uint8_t rt = 2;

	uint16_t imm = 0x10U;

	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;

	// MIPS ORI Instruction
	mipsCode[0] = 0x34000000U + (rs << 21U) + (rt << 16U) + (uint16_t)imm;
	mipsCode[1] = 0x00000008U;	// JR to Rs(0)
	mipsCode[2] = 0x00000000U;	// Delay Slot

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	reg[rt] = 0x100U;
	reg[rs] = 0x01U;

	run();

	ASSERT_EQ(reg[rt], (0x01U | imm));

	//---------------------------------------

	delSegment(seg);
	printf("ORI     translated successfully\n");
}

#if TEST_PREF
static void TranslationTest_PREF(code_segment_data_t* segmentData)
{
	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;

	uint8_t rs = 1;
	uint8_t rt = 2;

	uint16_t imm = 0x10U;

	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;

	// MIPS PREF Instruction
	mipsCode[0] = 0x30000000 + (rs << 21U) + (rt << 16U) + (uint16_t)imm;
	mipsCode[1] = 0x00000008;	// JR to Rs(0)
	mipsCode[2] = 0x00000000;	// Delay Slot

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	reg[rs] = 0x1111111111111111U;

	run();

	ASSERT_EQ(1, 0);


	//---------------------------------------

	delSegment(seg);
	printf("PREF translated successfully\n");
}
#endif

static void TranslationTest_SB(code_segment_data_t* segmentData)
{
	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;

	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;

	// MIPS SB Instruction
	mipsCode[0] = 0xA0000000U + (2 << 21U) + (1 << 16U);
	mipsCode[1] = 0x00000008U;	// JR to Rs(0)
	mipsCode[2] = 0x00000000U;	// Delay Slot

	mipsCode[3] = 0x11111100U;	// memory to write into

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	reg[1] = 0x3U;
	reg[2] = (uintptr_t)&mipsCode[3];

	run();

	ASSERT_EQ(mipsCode[3], 0x11111103U);

	//---------------------------------------

	delSegment(seg);
	printf("SB      translated successfully\n");
}

static void TranslationTest_SC(code_segment_data_t* segmentData)
{
	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;

	uint8_t rs = 1;
	uint8_t rt = 2;

	uint16_t imm = 0x10U;

	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;

	// MIPS SC Instruction
	mipsCode[0] = 0xE0000000 + (rs << 21U) + (rt << 16U) + (uint16_t)imm;
	mipsCode[1] = 0x00000008;	// JR to Rs(0)
	mipsCode[2] = 0x00000000;	// Delay Slot

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	reg[rs] = 0x1111111111111111U;

	run();

	ASSERT_EQ(1, 0);


	//---------------------------------------

	delSegment(seg);
	printf("SC      translated successfully\n");
}

static void TranslationTest_SCD(code_segment_data_t* segmentData)
{
	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;

	uint8_t rs = 1;
	uint8_t rt = 2;

	uint16_t imm = 0x10U;

	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;

	// MIPS SCD Instruction
	mipsCode[0] = 0x30000000 + (rs << 21U) + (rt << 16U) + (uint16_t)imm;
	mipsCode[1] = 0x00000008;	// JR to Rs(0)
	mipsCode[2] = 0x00000000;	// Delay Slot

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	reg[rs] = 0x1111111111111111U;

	run();

	ASSERT_EQ(1, 0);


	//---------------------------------------

	delSegment(seg);
	printf("SCD translated successfully\n");
}

static void TranslationTest_SD(code_segment_data_t* segmentData)
{
	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;

	uint8_t rs = 1;
	uint8_t rt = 2;

	uint16_t imm = 0x10U;

	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;

	// MIPS SD Instruction
	mipsCode[0] = 0x30000000 + (rs << 21U) + (rt << 16U) + (uint16_t)imm;
	mipsCode[1] = 0x00000008;	// JR to Rs(0)
	mipsCode[2] = 0x00000000;	// Delay Slot

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	reg[rs] = 0x1111111111111111U;

	run();

	ASSERT_EQ(1, 0);


	//---------------------------------------

	delSegment(seg);
	printf("SD translated successfully\n");
}

#if TEST_COPROCESSOR
static void TranslationTest_SDCz(code_segment_data_t* segmentData)
{
	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;

	uint8_t rs = 1;
	uint8_t rt = 2;

	uint16_t imm = 0x10U;

	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;

	// MIPS SDCz Instruction
	mipsCode[0] = 0x30000000 + (rs << 21U) + (rt << 16U) + (uint16_t)imm;
	mipsCode[1] = 0x00000008;	// JR to Rs(0)
	mipsCode[2] = 0x00000000;	// Delay Slot

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	reg[rs] = 0x1111111111111111U;

	run();

	ASSERT_EQ(1, 0);


	//---------------------------------------

	delSegment(seg);
	printf("SDCz    translated successfully\n");
}
#endif

static void TranslationTest_SDL(code_segment_data_t* segmentData)
{
	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;

	uint8_t rs = 1;
	uint8_t rt = 2;

	uint16_t imm = 0x10U;

	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;

	// MIPS SDL Instruction
	mipsCode[0] = 0x30000000 + (rs << 21U) + (rt << 16U) + (uint16_t)imm;
	mipsCode[1] = 0x00000008;	// JR to Rs(0)
	mipsCode[2] = 0x00000000;	// Delay Slot

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	reg[rs] = 0x1111111111111111U;

	run();

	ASSERT_EQ(1, 0);


	//---------------------------------------

	delSegment(seg);
	printf("SDL translated successfully\n");
}

static void TranslationTest_SDR(code_segment_data_t* segmentData)
{
	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;

	uint8_t rs = 1;
	uint8_t rt = 2;

	uint16_t imm = 0x10U;

	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;

	// MIPS SDR Instruction
	mipsCode[0] = 0x30000000 + (rs << 21U) + (rt << 16U) + (uint16_t)imm;
	mipsCode[1] = 0x00000008;	// JR to Rs(0)
	mipsCode[2] = 0x00000000;	// Delay Slot

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	reg[rs] = 0x1111111111111111U;

	run();

	ASSERT_EQ(1, 0);


	//---------------------------------------

	delSegment(seg);
	printf("SDR translated successfully\n");
}

static void TranslationTest_SH(code_segment_data_t* segmentData)
{
	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;

	uint8_t rs = 1;
	uint8_t rt = 2;

	uint16_t imm = 0x10U;

	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;

	// MIPS SH Instruction
	mipsCode[0] = 0x30000000 + (rs << 21U) + (rt << 16U) + (uint16_t)imm;
	mipsCode[1] = 0x00000008;	// JR to Rs(0)
	mipsCode[2] = 0x00000000;	// Delay Slot

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	reg[rs] = 0x1111111111111111U;

	run();

	ASSERT_EQ(1, 0);


	//---------------------------------------

	delSegment(seg);
	printf("SH translated successfully\n");
}

static void TranslationTest_SLL(code_segment_data_t* segmentData)
{
	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;

	uint8_t rs = 1;
	uint8_t rt = 2;

	uint16_t imm = 0x10U;

	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;

	// MIPS SLL Instruction
	mipsCode[0] = 0x30000000 + (rs << 21U) + (rt << 16U) + (uint16_t)imm;
	mipsCode[1] = 0x00000008;	// JR to Rs(0)
	mipsCode[2] = 0x00000000;	// Delay Slot

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	reg[rs] = 0x1111111111111111U;

	run();

	ASSERT_EQ(1, 0);

	//---------------------------------------

	delSegment(seg);
	printf("SLL translated successfully\n");
}

static void TranslationTest_SLLV(code_segment_data_t* segmentData)
{
	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;

	uint8_t rs = 1;
	uint8_t rt = 2;

	uint16_t imm = 0x10U;

	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;

	// MIPS SLLV Instruction
	mipsCode[0] = 0x30000000 + (rs << 21U) + (rt << 16U) + (uint16_t)imm;
	mipsCode[1] = 0x00000008;	// JR to Rs(0)
	mipsCode[2] = 0x00000000;	// Delay Slot

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	reg[rs] = 0x1111111111111111U;

	run();

	ASSERT_EQ(1, 0);


	//---------------------------------------

	delSegment(seg);
	printf("SLLV translated successfully\n");
}

static void TranslationTest_SLT(code_segment_data_t* segmentData)
{
	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;

	uint8_t rs = 1;
	uint8_t rt = 2;

	uint16_t imm = 0x10U;

	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;

	// MIPS SLT Instruction
	mipsCode[0] = 0x30000000 + (rs << 21U) + (rt << 16U) + (uint16_t)imm;
	mipsCode[1] = 0x00000008;	// JR to Rs(0)
	mipsCode[2] = 0x00000000;	// Delay Slot

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	reg[rs] = 0x1111111111111111U;

	run();

	ASSERT_EQ(1, 0);


	//---------------------------------------

	delSegment(seg);
	printf("SLT translated successfully\n");
}

static void TranslationTest_SLTI(code_segment_data_t* segmentData)
{
	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;

	uint8_t rs = 1;
	uint8_t rt = 2;

	uint16_t imm = 0x10U;

	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;

	// MIPS SLTI Instruction
	mipsCode[0] = 0x30000000 + (rs << 21U) + (rt << 16U) + (uint16_t)imm;
	mipsCode[1] = 0x00000008;	// JR to Rs(0)
	mipsCode[2] = 0x00000000;	// Delay Slot

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	reg[rs] = 0x1111111111111111U;

	run();

	ASSERT_EQ(1, 0);


	//---------------------------------------

	delSegment(seg);
	printf("SLTI translated successfully\n");
}

static void TranslationTest_SLTIU(code_segment_data_t* segmentData)
{
	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;

	uint8_t rs = 1;
	uint8_t rt = 2;

	uint16_t imm = 0x10U;

	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;

	// MIPS SLTIU Instruction
	mipsCode[0] = 0x30000000 + (rs << 21U) + (rt << 16U) + (uint16_t)imm;
	mipsCode[1] = 0x00000008;	// JR to Rs(0)
	mipsCode[2] = 0x00000000;	// Delay Slot

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	reg[rs] = 0x1111111111111111U;

	run();

	ASSERT_EQ(1, 0);


	//---------------------------------------

	delSegment(seg);
	printf("SLTIU translated successfully\n");
}

static void TranslationTest_SLTU(code_segment_data_t* segmentData)
{
	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;

	uint8_t rs = 1;
	uint8_t rt = 2;

	uint16_t imm = 0x10U;

	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;

	// MIPS SLTU Instruction
	mipsCode[0] = 0x30000000 + (rs << 21U) + (rt << 16U) + (uint16_t)imm;
	mipsCode[1] = 0x00000008;	// JR to Rs(0)
	mipsCode[2] = 0x00000000;	// Delay Slot

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	reg[rs] = 0x1111111111111111U;

	run();

	ASSERT_EQ(1, 0);


	//---------------------------------------

	delSegment(seg);
	printf("SLTU translated successfully\n");
}

static void TranslationTest_SRA(code_segment_data_t* segmentData)
{
	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;

	uint8_t rs = 1;
	uint8_t rt = 2;

	uint16_t imm = 0x10U;

	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;

	// MIPS SRA Instruction
	mipsCode[0] = 0x30000000 + (rs << 21U) + (rt << 16U) + (uint16_t)imm;
	mipsCode[1] = 0x00000008;	// JR to Rs(0)
	mipsCode[2] = 0x00000000;	// Delay Slot

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	reg[rs] = 0x1111111111111111U;

	run();

	ASSERT_EQ(1, 0);


	//---------------------------------------

	delSegment(seg);
	printf("SRA translated successfully\n");
}

static void TranslationTest_SRAV(code_segment_data_t* segmentData)
{
	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;

	uint8_t rs = 1;
	uint8_t rt = 2;

	uint16_t imm = 0x10U;

	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;

	// MIPS SRAV Instruction
	mipsCode[0] = 0x30000000 + (rs << 21U) + (rt << 16U) + (uint16_t)imm;
	mipsCode[1] = 0x00000008;	// JR to Rs(0)
	mipsCode[2] = 0x00000000;	// Delay Slot

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	reg[rs] = 0x1111111111111111U;

	run();

	ASSERT_EQ(1, 0);


	//---------------------------------------

	delSegment(seg);
	printf("SRAV translated successfully\n");
}

static void TranslationTest_SRL(code_segment_data_t* segmentData)
{
	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;

	uint8_t rs = 1;
	uint8_t rt = 2;

	uint16_t imm = 0x10U;

	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;

	// MIPS SRL Instruction
	mipsCode[0] = 0x30000000 + (rs << 21U) + (rt << 16U) + (uint16_t)imm;
	mipsCode[1] = 0x00000008;	// JR to Rs(0)
	mipsCode[2] = 0x00000000;	// Delay Slot

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	reg[rs] = 0x1111111111111111U;

	run();

	ASSERT_EQ(1, 0);


	//---------------------------------------

	delSegment(seg);
	printf("SRL translated successfully\n");
}

static void TranslationTest_SRLV(code_segment_data_t* segmentData)
{
	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;

	uint8_t rs = 1;
	uint8_t rt = 2;

	uint16_t imm = 0x10U;

	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;

	// MIPS SRLV Instruction
	mipsCode[0] = 0x30000000 + (rs << 21U) + (rt << 16U) + (uint16_t)imm;
	mipsCode[1] = 0x00000008;	// JR to Rs(0)
	mipsCode[2] = 0x00000000;	// Delay Slot

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	reg[rs] = 0x1111111111111111U;

	run();

	ASSERT_EQ(1, 0);


	//---------------------------------------

	delSegment(seg);
	printf("SRLV translated successfully\n");
}

static void TranslationTest_SUB(code_segment_data_t* segmentData)
{
	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;

	uint8_t rs = 1;
	uint8_t rt = 2;

	uint16_t imm = 0x10U;

	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;

	// MIPS SUB Instruction
	mipsCode[0] = 0x30000000 + (rs << 21U) + (rt << 16U) + (uint16_t)imm;
	mipsCode[1] = 0x00000008;	// JR to Rs(0)
	mipsCode[2] = 0x00000000;	// Delay Slot

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	reg[rs] = 0x1111111111111111U;

	run();

	ASSERT_EQ(1, 0);


	//---------------------------------------

	delSegment(seg);
	printf("SUB     translated successfully\n");
}

static void TranslationTest_SUBU(code_segment_data_t* segmentData)
{
	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;

	uint8_t rs = 1;
	uint8_t rt = 2;

	uint16_t imm = 0x10U;

	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;

	// MIPS SUBU Instruction
	mipsCode[0] = 0x30000000 + (rs << 21U) + (rt << 16U) + (uint16_t)imm;
	mipsCode[1] = 0x00000008;	// JR to Rs(0)
	mipsCode[2] = 0x00000000;	// Delay Slot

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	reg[rs] = 0x1111111111111111U;

	run();

	ASSERT_EQ(1, 0);


	//---------------------------------------

	delSegment(seg);
	printf("SUBU    translated successfully\n");
}

static void TranslationTest_SW(code_segment_data_t* segmentData)
{
	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;

	uint8_t rs = 1;
	uint8_t rt = 2;

	uint16_t imm = 0x10U;

	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;

	// MIPS SW Instruction
	mipsCode[0] = 0x30000000 + (rs << 21U) + (rt << 16U) + (uint16_t)imm;
	mipsCode[1] = 0x00000008;	// JR to Rs(0)
	mipsCode[2] = 0x00000000;	// Delay Slot

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	reg[rs] = 0x1111111111111111U;

	run();

	ASSERT_EQ(1, 0);


	//---------------------------------------

	delSegment(seg);
	printf("SW      translated successfully\n");
}

#if TEST_COPROCESSOR
static void TranslationTest_SWCz(code_segment_data_t* segmentData)
{
	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;

	uint8_t rs = 1;
	uint8_t rt = 2;

	uint16_t imm = 0x10U;

	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;

	// MIPS SWCz Instruction
	mipsCode[0] = 0x30000000 + (rs << 21U) + (rt << 16U) + (uint16_t)imm;
	mipsCode[1] = 0x00000008;	// JR to Rs(0)
	mipsCode[2] = 0x00000000;	// Delay Slot

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	reg[rs] = 0x1111111111111111U;

	run();

	ASSERT_EQ(1, 0);


	//---------------------------------------

	delSegment(seg);
	printf("SWCz translated successfully\n");
}
#endif

static void TranslationTest_SWL(code_segment_data_t* segmentData)
{
	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;

	uint8_t rs = 1;
	uint8_t rt = 2;

	uint16_t imm = 0x10U;

	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;

	// MIPS SWL Instruction
	mipsCode[0] = 0x30000000 + (rs << 21U) + (rt << 16U) + (uint16_t)imm;
	mipsCode[1] = 0x00000008;	// JR to Rs(0)
	mipsCode[2] = 0x00000000;	// Delay Slot

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	reg[rs] = 0x1111111111111111U;

	run();

	ASSERT_EQ(1, 0);


	//---------------------------------------

	delSegment(seg);
	printf("SWL     translated successfully\n");
}
static void TranslationTest_SWR(code_segment_data_t* segmentData)
{
	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;

	uint16_t imm = 0x1U;

	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;

	// MIPS SWR Instruction
	mipsCode[0] = 0xB8000000 + (2U << 21U) + (1U << 16U) + imm;
	mipsCode[1] = 0x00000008U;	// JR to Rs(0)
	mipsCode[2] = 0x00000000U;	// Delay Slot
	mipsCode[3] = 0x03030303U;

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	reg[1] = 0x000000000004080CU;
	reg[2] = (uintptr_t)&mipsCode[3];

	run();

	// assumming little endian
	ASSERT_EQ(mipsCode[3], 0x04080C03U);


	//---------------------------------------

	delSegment(seg);
	printf("SWR     translated successfully\n");
}

#if TEST_SYNC
static void TranslationTest_SYNC(code_segment_data_t* segmentData)
{
	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;

	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;

	// MIPS SYNC Instruction
	mipsCode[0] = 0x0000000F + (0U << 6U);
	mipsCode[1] = 0x00000008;	// JR to Rs(0)
	mipsCode[2] = 0x00000000;	// Delay Slot

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	run();

	//---------------------------------------

	delSegment(seg);
	printf("SYNC translated successfully\n");
}
#endif

#if TEST_SYSCALL
static void TranslationTest_SYSCALL(code_segment_data_t* segmentData)
{
	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;

	uint8_t rs = 1;
	uint8_t rt = 2;

	uint16_t imm = 0x10U;

	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;

	// MIPS SYSCALL Instruction
	mipsCode[0] = 0x0000000C;
	mipsCode[1] = 0x00000008;	// JR to Rs(0)
	mipsCode[2] = 0x00000000;	// Delay Slot

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	run();

	ASSERT_EQ(1, 0);

	//---------------------------------------

	delSegment(seg);
	printf("SYSCALL translated successfully\n");
}
#endif

#if TEST_TRAP
static void TranslationTest_TEQ(code_segment_data_t* segmentData)
{
	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;

	uint8_t rs = 1;
	uint8_t rt = 2;

	uint16_t imm = 0x10U;

	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;

	// MIPS TEQ Instruction
	mipsCode[0] = 0x30000000 + (rs << 21U) + (rt << 16U) + (uint16_t)imm;
	mipsCode[1] = 0x00000008;	// JR to Rs(0)
	mipsCode[2] = 0x00000000;	// Delay Slot

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	reg[rs] = 0x1111111111111111U;

	run();

	ASSERT_EQ(1, 0);


	//---------------------------------------

	delSegment(seg);
	printf("TEQ translated successfully\n");
}
static void TranslationTest_TEQI(code_segment_data_t* segmentData)
{
	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;

	uint8_t rs = 1;
	uint8_t rt = 2;

	uint16_t imm = 0x10U;

	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;

	// MIPS TEQI Instruction
	mipsCode[0] = 0x30000000 + (rs << 21U) + (rt << 16U) + (uint16_t)imm;
	mipsCode[1] = 0x00000008;	// JR to Rs(0)
	mipsCode[2] = 0x00000000;	// Delay Slot

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	reg[rs] = 0x1111111111111111U;

	run();

	ASSERT_EQ(1, 0);


	//---------------------------------------

	delSegment(seg);
	printf("TEQI translated successfully\n");
}
static void TranslationTest_TGE(code_segment_data_t* segmentData)
{
	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;

	uint8_t rs = 1;
	uint8_t rt = 2;

	uint16_t imm = 0x10U;

	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;

	// MIPS TGE Instruction
	mipsCode[0] = 0x30000000 + (rs << 21U) + (rt << 16U) + (uint16_t)imm;
	mipsCode[1] = 0x00000008;	// JR to Rs(0)
	mipsCode[2] = 0x00000000;	// Delay Slot

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	reg[rs] = 0x1111111111111111U;

	run();

	ASSERT_EQ(1, 0);


	//---------------------------------------

	delSegment(seg);
	printf("TGE translated successfully\n");
}
static void TranslationTest_TGEI(code_segment_data_t* segmentData)
{
	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;

	uint8_t rs = 1;
	uint8_t rt = 2;

	uint16_t imm = 0x10U;

	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;

	// MIPS TGEI Instruction
	mipsCode[0] = 0x30000000 + (rs << 21U) + (rt << 16U) + (uint16_t)imm;
	mipsCode[1] = 0x00000008;	// JR to Rs(0)
	mipsCode[2] = 0x00000000;	// Delay Slot

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	reg[rs] = 0x1111111111111111U;

	run();

	ASSERT_EQ(1, 0);


	//---------------------------------------

	delSegment(seg);
	printf("TGEI translated successfully\n");
}
static void TranslationTest_TGEIU(code_segment_data_t* segmentData)
{
	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;

	uint8_t rs = 1;
	uint8_t rt = 2;

	uint16_t imm = 0x10U;

	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;

	// MIPS TGEIU Instruction
	mipsCode[0] = 0x30000000 + (rs << 21U) + (rt << 16U) + (uint16_t)imm;
	mipsCode[1] = 0x00000008;	// JR to Rs(0)
	mipsCode[2] = 0x00000000;	// Delay Slot

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	reg[rs] = 0x1111111111111111U;

	run();

	ASSERT_EQ(1, 0);

	//---------------------------------------

	delSegment(seg);
	printf("TGEIU translated successfully\n");
}
static void TranslationTest_TGEU(code_segment_data_t* segmentData)
{
	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;

	uint8_t rs = 1;
	uint8_t rt = 2;

	uint16_t imm = 0x10U;

	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;

	// MIPS TGEU Instruction
	mipsCode[0] = 0x30000000 + (rs << 21U) + (rt << 16U) + (uint16_t)imm;
	mipsCode[1] = 0x00000008;	// JR to Rs(0)
	mipsCode[2] = 0x00000000;	// Delay Slot

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	reg[rs] = 0x1111111111111111U;

	run();

	ASSERT_EQ(1, 0);

	//---------------------------------------

	delSegment(seg);
	printf("TGEU translated successfully\n");
}
static void TranslationTest_TLT(code_segment_data_t* segmentData)
{
	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;

	uint8_t rs = 1;
	uint8_t rt = 2;

	uint16_t imm = 0x10U;

	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;

	// MIPS TLT Instruction
	mipsCode[0] = 0x30000000 + (rs << 21U) + (rt << 16U) + (uint16_t)imm;
	mipsCode[1] = 0x00000008;	// JR to Rs(0)
	mipsCode[2] = 0x00000000;	// Delay Slot

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	reg[rs] = 0x1111111111111111U;

	run();

	ASSERT_EQ(1, 0);


	//---------------------------------------

	delSegment(seg);
	printf("TLT translated successfully\n");
}
static void TranslationTest_TLTI(code_segment_data_t* segmentData)
{
	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;

	uint8_t rs = 1;
	uint8_t rt = 2;

	uint16_t imm = 0x10U;

	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;

	// MIPS TLTI Instruction
	mipsCode[0] = 0x30000000 + (rs << 21U) + (rt << 16U) + (uint16_t)imm;
	mipsCode[1] = 0x00000008;	// JR to Rs(0)
	mipsCode[2] = 0x00000000;	// Delay Slot

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	reg[rs] = 0x1111111111111111U;

	run();

	ASSERT_EQ(1, 0);


	//---------------------------------------

	delSegment(seg);
	printf("TLTI translated successfully\n");
}
static void TranslationTest_TLTIU(code_segment_data_t* segmentData)
{
	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;

	uint8_t rs = 1;
	uint8_t rt = 2;

	uint16_t imm = 0x10U;

	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;

	// MIPS TLTIU Instruction
	mipsCode[0] = 0x30000000 + (rs << 21U) + (rt << 16U) + (uint16_t)imm;
	mipsCode[1] = 0x00000008;	// JR to Rs(0)
	mipsCode[2] = 0x00000000;	// Delay Slot

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	reg[rs] = 0x1111111111111111U;

	run();

	ASSERT_EQ(1, 0);

	//---------------------------------------

	delSegment(seg);
	printf("TLTIU translated successfully\n");
}
static void TranslationTest_TLTU(code_segment_data_t* segmentData)
{
	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;

	uint8_t rs = 1;
	uint8_t rt = 2;

	uint16_t imm = 0x10U;

	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;

	// MIPS TLTU Instruction
	mipsCode[0] = 0x30000000 + (rs << 21U) + (rt << 16U) + (uint16_t)imm;
	mipsCode[1] = 0x00000008;	// JR to Rs(0)
	mipsCode[2] = 0x00000000;	// Delay Slot

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	reg[rs] = 0x1111111111111111U;

	run();

	ASSERT_EQ(1, 0);


	//---------------------------------------

	delSegment(seg);
	printf("TLTU translated successfully\n");
}
static void TranslationTest_TNE(code_segment_data_t* segmentData)
{
	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;

	uint8_t rs = 1;
	uint8_t rt = 2;

	uint16_t imm = 0x10U;

	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;

	// MIPS TNE Instruction
	mipsCode[0] = 0x30000000 + (rs << 21U) + (rt << 16U) + (uint16_t)imm;
	mipsCode[1] = 0x00000008;	// JR to Rs(0)
	mipsCode[2] = 0x00000000;	// Delay Slot

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	reg[rs] = 0x1111111111111111U;

	run();

	ASSERT_EQ(1, 0);


	//---------------------------------------

	delSegment(seg);
	printf("TNE translated successfully\n");
}
static void TranslationTest_TNEI(code_segment_data_t* segmentData)
{
	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;

	uint8_t rs = 1;
	uint8_t rt = 2;

	uint16_t imm = 0x10U;

	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;

	// MIPS TNEI Instruction
	mipsCode[0] = 0x30000000 + (rs << 21U) + (rt << 16U) + (uint16_t)imm;
	mipsCode[1] = 0x00000008;	// JR to Rs(0)
	mipsCode[2] = 0x00000000;	// Delay Slot

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	reg[rs] = 0x1111111111111111U;

	run();

	ASSERT_EQ(1, 0);


	//---------------------------------------

	delSegment(seg);
	printf("TNEI    translated successfully\n");
}
#endif

static void TranslationTest_XOR(code_segment_data_t* segmentData)
{
	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;

	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;

	// MIPS XOR Instruction
	mipsCode[0] = 0x00000026 + (1U << 21U) + (2U << 16U) + (3U << 11U);
	mipsCode[1] = 0x00000008;	// JR to Rs(0)
	mipsCode[2] = 0x00000000;	// Delay Slot

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	reg[1] = 0x0000002300000023U;
	reg[2] = 0x0000003200000032U;
	reg[3] = 0x0000000000000000U;

	run();

	ASSERT_EQ(reg[1], 0x0000002300000023U);
	ASSERT_EQ(reg[2], 0x0000003200000032U);
	ASSERT_EQ(reg[3], 0x0000001100000011U);

	//---------------------------------------

	delSegment(seg);
	printf("XOR     translated successfully\n");
}

static void TranslationTest_XORI(code_segment_data_t* segmentData)
{
	code_seg_t* seg = newSegment();
	segmentData->dbgCurrentSegment = seg;


	uint16_t imm = 0x03U;

	seg->MIPScode = mipsCode;
	seg->MIPScodeLen = 2U;
	seg->Type = SEG_ALONE;

	// MIPS XORI Instruction
	mipsCode[0] = 0x38000000 + (1U << 21U) + (2U << 16U) + imm;
	mipsCode[1] = 0x00000008;	// JR to Rs(0)
	mipsCode[2] = 0x00000000;	// Delay Slot

	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -100;

	Translate(seg);

	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	//---------------------------------------

	reg[1] = 0x0030000000000012U;

	run();

	ASSERT_EQ(reg[2], 0x0030000000000011U);

	//---------------------------------------

	delSegment(seg);
	printf("XORI    translated successfully\n");
}


void Translation_Test(code_segment_data_t* segmentData)
{
	mipsCode = (uint32_t*)(MMAP_STATIC_REGION);

	run = (pfvru1)segmentData->segStart->ARMEntryPoint;
	reg = (uint64_t*)MMAP_FP_BASE;

	TranslationTest_ADD(segmentData);
	TranslationTest_ADDI(segmentData);
	TranslationTest_ADDIU(segmentData);
	TranslationTest_ADDU(segmentData);
	TranslationTest_AND(segmentData);
	TranslationTest_ANDI(segmentData);
	TranslationTest_BEQ(segmentData);
	TranslationTest_BEQL(segmentData);
	TranslationTest_BGEZ(segmentData);
	TranslationTest_BGEZAL(segmentData);
	TranslationTest_BGEZALL(segmentData);
	TranslationTest_BGEZL(segmentData);
	TranslationTest_BGTZ(segmentData);
	TranslationTest_BGTZL(segmentData);
	TranslationTest_BLEZ(segmentData);
	TranslationTest_BLEZL(segmentData);
	TranslationTest_BLTZ(segmentData);
	TranslationTest_BLTZAL(segmentData);
	TranslationTest_BLTZALL(segmentData);
	TranslationTest_BLTZL(segmentData);
	TranslationTest_BNE(segmentData);
	TranslationTest_BNEL(segmentData);
#if TEST_BREAK
	TranslationTest_BREAK(segmentData);
#endif
	TranslationTest_DADD(segmentData);
	TranslationTest_DADDI(segmentData);
	TranslationTest_DADDIU(segmentData);
	TranslationTest_DADDU(segmentData);
	TranslationTest_DDIV(segmentData);
	TranslationTest_DDIVU(segmentData);
	TranslationTest_DIV(segmentData);
	TranslationTest_DIVU(segmentData);
	TranslationTest_DMULT(segmentData);
	TranslationTest_DMULTU(segmentData);
	TranslationTest_DSLL(segmentData);
	TranslationTest_DSLL32(segmentData);
	TranslationTest_DSLLV(segmentData);
	TranslationTest_DSRA(segmentData);
	TranslationTest_DSRA32(segmentData);
	TranslationTest_DSRAV(segmentData);
	TranslationTest_DSRL(segmentData);
	TranslationTest_DSRL32(segmentData);
	TranslationTest_DSRLV(segmentData);
	TranslationTest_DSUB(segmentData);
	TranslationTest_DSUBU(segmentData);
	TranslationTest_J(segmentData);
	TranslationTest_JAL(segmentData);
	TranslationTest_JALR(segmentData);
	TranslationTest_JR(segmentData);
	TranslationTest_LB(segmentData);
	TranslationTest_LBU(segmentData);
	TranslationTest_LD(segmentData);
#if TEST_COPROCESSOR
	TranslationTest_LDCz(segmentData);
#endif
	TranslationTest_LDL(segmentData);
	TranslationTest_LDR(segmentData);
	TranslationTest_LH(segmentData);
	TranslationTest_LHU(segmentData);
	TranslationTest_LL(segmentData);
	TranslationTest_LLD(segmentData);
	TranslationTest_LUI(segmentData);
	TranslationTest_LW(segmentData);
#if TEST_COPROCESSOR
	TranslationTest_LWCz(segmentData);
#endif
	TranslationTest_LWL(segmentData);
	TranslationTest_LWR(segmentData);
	TranslationTest_LWU(segmentData);
	TranslationTest_MFHI(segmentData);
	TranslationTest_MFLO(segmentData);
	TranslationTest_MOVN(segmentData);
	TranslationTest_MOVZ(segmentData);
	TranslationTest_MTHI(segmentData);
	TranslationTest_MTLO(segmentData);
	TranslationTest_MULT(segmentData);
	TranslationTest_MULTU(segmentData);
	TranslationTest_NOR(segmentData);
	TranslationTest_OR(segmentData);
	TranslationTest_ORI(segmentData);
#if TEST_PREF
	TranslationTest_PREF(segmentData);
#endif
	TranslationTest_SB(segmentData);
	TranslationTest_SC(segmentData);
	TranslationTest_SCD(segmentData);
	TranslationTest_SD(segmentData);
#if TEST_COPROCESSOR
	TranslationTest_SDCz(segmentData);
#endif
	TranslationTest_SDL(segmentData);
	TranslationTest_SDR(segmentData);
	TranslationTest_SH(segmentData);
	TranslationTest_SLL(segmentData);
	TranslationTest_SLLV(segmentData);
	TranslationTest_SLT(segmentData);
	TranslationTest_SLTI(segmentData);
	TranslationTest_SLTIU(segmentData);
	TranslationTest_SLTU(segmentData);
	TranslationTest_SRA(segmentData);
	TranslationTest_SRAV(segmentData);
	TranslationTest_SRL(segmentData);
	TranslationTest_SRLV(segmentData);
	TranslationTest_SUB(segmentData);
	TranslationTest_SUBU(segmentData);
	TranslationTest_SW(segmentData);
#if TEST_COPROCESSOR
	TranslationTest_SWCz(segmentData);
#endif
	TranslationTest_SWL(segmentData);
	TranslationTest_SWR(segmentData);

#if TEST_SYNC
	TranslationTest_SYNC(segmentData);
#endif

#if TEST_SYSCALL
	TranslationTest_SYSCALL(segmentData);
#endif

#if TEST_TRAP
	TranslationTest_TEQ(segmentData);
	TranslationTest_TEQI(segmentData);
	TranslationTest_TGE(segmentData);
	TranslationTest_TGEI(segmentData);
	TranslationTest_TGEIU(segmentData);
	TranslationTest_TGEU(segmentData);
	TranslationTest_TLT(segmentData);
	TranslationTest_TLTI(segmentData);
	TranslationTest_TLTIU(segmentData);
	TranslationTest_TLTU(segmentData);
	TranslationTest_TNE(segmentData);
	TranslationTest_TNEI(segmentData);
#endif
	TranslationTest_XOR(segmentData);
	TranslationTest_XORI(segmentData);
}
