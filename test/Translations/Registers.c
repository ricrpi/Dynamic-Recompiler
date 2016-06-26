/*
 * Registers.c
 *
 *  Created on: 5 Jun 2016
 *      Author: ric
 */


#include "Translate.h"
#include "CodeSegments.h"
#include "memory.h"

#include <string.h>
#include <assert.h>
#include <stdio.h>

// provide a pointer to the emulated N64 registers
static volatile uint64_t* reg = NULL;

// provide a function pointer to the START routine
static pfvru1 run = NULL;

static void Test_Translate_Registers_emulate_temp()
{
	code_seg_t* seg = newSegment();

	seg->Intermcode = newInstrI(ARM_MOV, AL, REG_TEMP_SCRATCH0, REG_NOT_USED, REG_NOT_USED, 0);

	Translate_Registers(seg);

	const Instruction_t* const ins_mov = seg->Intermcode;

	// Register R2 should be loaded from cache
	assert(ins_mov->instruction == ARM_MOV);
	assert(ins_mov->Reg[INS_RD1].regID == REG_HOST_R0);

	assert(ins_mov->nextInstruction == NULL);

	delSegment(seg);

	printf("Test_Translate_Registers_emulate_temp() passed\n");
}

static void Test_Translate_Registers_emulate_str()
{
	code_seg_t* seg = newSegment();

	seg->Intermcode = newInstrI(ARM_MOV, AL, 1U, REG_NOT_USED, REG_NOT_USED, 0);

	Translate_Registers(seg);

	const Instruction_t* const ins_mov = seg->Intermcode;
	const Instruction_t* const ins_str = ins_mov->nextInstruction;

	// Register R2 should be loaded from cache
	assert(ins_mov->instruction == ARM_MOV);
	assert(ins_mov->Reg[INS_RD1].regID > REG_HOST);

	assert(ins_str->instruction == ARM_STR);
	assert(ins_str->Reg[INS_R1].regID == ins_mov->Reg[INS_RD1].regID);
	assert(ins_str->offset ==  8U);

	delSegment(seg);

	printf("Test_Translate_Registers_emulate_str() passed\n");
}

static void Test_Translate_Registers_emulate_ldr()
{
	code_seg_t* seg = newSegment();

	seg->Intermcode = newInstr(ARM_MOV, AL, REG_TEMP_SCRATCH0, REG_NOT_USED, 1U);

	Translate_Registers(seg);

	const Instruction_t* const ins_ldr = seg->Intermcode;
	const Instruction_t* const ins_mov = ins_ldr->nextInstruction;

	// Register R2 should be loaded from cache
	assert(ins_ldr->instruction == ARM_LDR);
	assert(ins_ldr->Reg[INS_RD1].regID > REG_HOST);
	assert(ins_ldr->offset == 8U);

	assert(ins_mov->instruction == ARM_MOV);
	assert(ins_mov->Reg[INS_R2].regID == ins_ldr->Reg[INS_RD1].regID);
	assert(ins_mov->Reg[INS_RD1].regID == REG_HOST + 0U);

	assert(ins_mov->nextInstruction == NULL);

	delSegment(seg);

	printf("Test_Translate_Registers_emulate_ldr() passed\n");
}

static void Test_Translate_Registers_emulate_full()
{
	code_seg_t* seg = newSegment();

	seg->Intermcode = newInstr(ARM_MOV, AL, REG_TEMP_SCRATCH0, REG_NOT_USED, 1U);

	Instruction_t* ins = seg->Intermcode;
	uint32_t r = 1U;

	for (r = 1U; r < 32U; r++)
	{
		ins->nextInstruction = newInstrI(ARM_MOV, AL, r, REG_NOT_USED, REG_NOT_USED, r);
		ins = ins->nextInstruction;
	}

	ins->nextInstruction = newInstrB(AL, *((uint32_t*)(MMAP_FP_BASE + FUNC_GEN_STOP)), 1);


	Translate_Registers(seg);
	Translate_Write(seg);


	*((volatile int32_t*)MMAP_FP_BASE + REG_COUNT) = -1000;
	*((uintptr_t*)(MMAP_FP_BASE + RECOMPILED_CODE_START)) = (uintptr_t)seg->ARMEntryPoint;

	delSegment(seg);

	run();

	for (r = 1U; r < 32U; r++)
	{
		printf("r%u = %u\n", r, reg[r]);
		assert(reg[r] == r);
	}

	printf("Test_Translate_Registers_emulate_full() passed\n");
}


void Test_Registers(code_segment_data_t* segmentData)
{
	Test_Translate_Registers_emulate_temp();
	Test_Translate_Registers_emulate_str();
	Test_Translate_Registers_emulate_ldr();

	run = (pfvru1)segmentData->segStart->ARMEntryPoint;
	reg = (uint64_t*)MMAP_FP_BASE;


	Test_Translate_Registers_emulate_full();
}
