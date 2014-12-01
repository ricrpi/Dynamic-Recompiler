/*
 * Translate.c
 *
 *  Created on: 16 Apr 2014
 *      Author: rjhender
 */

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

uint32_t uiCountFrequency 	= 40;	// must be less than 128 else may not be able to encode in imm8
uint32_t bMemoryOffsetCheck = 0;
uint32_t bDoDMAonInterrupt 	= 1;

char* currentTranslation = NULL;

//=============================================================

Instruction_t* insertP_R_A(code_seg_t* const code_seg, Instruction_t* ins, const Condition_e cond)
{
	Instruction_t* new_ins;
	regID_t base;
	int32_t offset;

#if defined(USE_INSTRUCTION_COMMENTS)
	char* oldCurrentTranslation = currentTranslation;
	currentTranslation = "insertP_R_A()";
#endif

	addLiteral(code_seg, &base, &offset, (uint32_t)&p_r_a);

	// we have to save r0 to r3 but as EAPI uses sp to point to arg 5 onwards we can't save all the registers in one go
	new_ins 	= newInstrPUSH(AL, REG_HOST_STM_EABI);
	ADD_LL_NEXT(new_ins, ins);

	new_ins 	= newInstrPUSH(AL, REG_HOST_STM_ALL & ~REG_HOST_STM_EABI);
	ADD_LL_NEXT(new_ins, ins);

	new_ins 		= newInstrI(ARM_LDR_LIT, AL, REG_HOST_R0, REG_NOT_USED, base, offset);
	ADD_LL_NEXT(new_ins, ins);

#if 1
	// load function address from [fp + offset] into PC
	new_ins 	= newInstr(ARM_BX, cond, REG_NOT_USED, REG_HOST_R0, REG_NOT_USED);
	new_ins->Ln = 1;
	ADD_LL_NEXT(new_ins, ins);
#else
	//set lr
	new_ins 	= newInstr(ARM_MOV, AL, REG_HOST_LR, REG_NOT_USED, REG_HOST_PC);
	ADD_LL_NEXT(new_ins, ins);

	// load function address from [fp + offset] into PC
	new_ins 	= newInstr(ARM_MOV, cond, REG_HOST_PC, REG_NOT_USED, REG_HOST_R0);
	ADD_LL_NEXT(new_ins, ins);
#endif

	// pop lr
	new_ins 	= newInstrPOP(AL, REG_HOST_STM_ALL & ~REG_HOST_STM_EABI);
	ADD_LL_NEXT(new_ins, ins);

	new_ins 	= newInstrPOP(AL, REG_HOST_STM_EABI);
	ADD_LL_NEXT(new_ins, ins);

#if defined(USE_INSTRUCTION_COMMENTS)
	currentTranslation = oldCurrentTranslation;
#endif

	return ins;
}


Instruction_t* insertCall_To_C(code_seg_t* const code_seg, Instruction_t* ins, const Condition_e cond, uint32_t functionAddress, uint32_t Rmask)
{
	Instruction_t* newInstruction;
	regID_t base;
	int32_t offset;

	addLiteral(code_seg, &base, &offset, functionAddress);

	newInstruction 		= newInstrI(ARM_LDR_LIT, AL, REG_TEMP_CALL2C, REG_NOT_USED, base, offset);
	ADD_LL_NEXT(newInstruction, ins);

	//push lr
	newInstruction 	= newInstrPUSH(AL, Rmask | REG_HOST_STM_LR);
	ADD_LL_NEXT(newInstruction, ins);

	newInstruction 	= newInstr(ARM_BX, cond, REG_NOT_USED, REG_TEMP_CALL2C, REG_NOT_USED);
	newInstruction->Ln = 1;
	ADD_LL_NEXT(newInstruction, ins);

	// pop lr
	newInstruction 	= newInstrPOP(AL, Rmask | REG_HOST_STM_LR);
	ADD_LL_NEXT(newInstruction, ins);

	return ins;
}

Instruction_t* insertCall_To_C_Jump(code_seg_t* const code_seg, Instruction_t* ins, const Condition_e cond, uint32_t functionAddress, uint32_t Rmask, Instruction_t* ReturnIns)
{
	Instruction_t* newInstruction;
	regID_t base;
	int32_t offset;

	addLiteral(code_seg, &base, &offset, functionAddress);

	newInstruction 		= newInstrI(ARM_LDR_LIT, AL, REG_TEMP_SCRATCH0, REG_NOT_USED, base, offset);
	ADD_LL_NEXT(newInstruction, ins);

	//push lr
	newInstruction 	= newInstrPUSH(AL, Rmask | REG_HOST_STM_LR);
	ADD_LL_NEXT(newInstruction, ins);

	//set lr
	newInstruction 	= newInstr(ARM_MOV, AL, REG_HOST_LR, REG_NOT_USED, REG_NOT_USED);
	newInstruction->branchToThisInstruction = ReturnIns;
	ADD_LL_NEXT(newInstruction, ins);

	// load function address from [fp + offset] into PC
	newInstruction 	= newInstr(ARM_MOV, cond, REG_HOST_PC, REG_NOT_USED, REG_TEMP_SCRATCH0);
	ADD_LL_NEXT(newInstruction, ins);

	// pop lr
	newInstruction 	= newInstrPOP(AL, Rmask | REG_HOST_STM_LR);
	ADD_LL_NEXT(newInstruction, ins);

	return ins;
}

//=============================================================

void Translate_Write(code_seg_t* codeSegment)
{
	emit_arm_code(codeSegment);
}

void Translate(code_seg_t* const codeSegment)
{
	int x;

	for (x=0; x < COUNTOF(Translations); x++)
	{
		Translations[x].function(codeSegment);
	}
}
