/*
 * Debug.c
 *
 *  Created on: 28 Oct 2014
 *      Author: rjhender
 */

#include "Translate.h"
#include "Debugger.h"	// RWD


void Translate_BreakAtEndSegment(code_seg_t* codeSegment)
{
	Instruction_t*ins, *new_ins;
	ins = codeSegment->Intermcode;

#if defined(USE_INSTRUCTION_COMMENTS)
	currentTranslation = "Call Debugger_wrapper()";
#endif

	while (ins->nextInstruction->nextInstruction)
	{
		ins = ins->nextInstruction;
	}

	new_ins = newInstrPUSH(AL, REG_HOST_STM_ALL);
	ADD_LL_NEXT(new_ins, ins);

	new_ins = newInstr(ARM_MOV, AL, REG_HOST_R0, REG_NOT_USED, REG_HOST_SP);
	ADD_LL_NEXT(new_ins, ins);

	ins = insertCall_To_C(codeSegment, ins, AL,(uint32_t)Debugger_wrapper, 0);

	new_ins = newInstrPOP(AL, REG_HOST_STM_ALL);
	ADD_LL_NEXT(new_ins, ins);
}

void Translate_Debug(code_seg_t* codeSegment)
{
	Instruction_t*ins;
	Instruction_t*new_ins;
	ins = codeSegment->Intermcode;

#if defined(USE_INSTRUCTION_COMMENTS)
	currentTranslation = "Debug - Load Segment ID";
#endif
	regID_t base;
	int32_t offset;

	//TODO Nasty global segmentData!

	//load current segment Address
	addLiteral(codeSegment, &base, &offset, (uint32_t)codeSegment);
	codeSegment->Intermcode = new_ins = newInstrI(ARM_LDR_LIT, AL, REG_TEMP_DBG1, REG_NOT_USED, base, offset);
	new_ins->nextInstruction = ins;
	ins = new_ins;

	//load segmentData->dbgCurrentSegment address
	addLiteral(codeSegment, &base, &offset, (uint32_t)&segmentData.dbgCurrentSegment);
	new_ins 		= newInstrI(ARM_LDR_LIT, AL, REG_TEMP_DBG2, REG_NOT_USED, base, offset);
	ADD_LL_NEXT(new_ins, ins);

#if defined(USE_INSTRUCTION_COMMENTS)
	currentTranslation = "Debug - Store Segment ID";
#endif

	//store
	new_ins 		= newInstrI(ARM_STR, AL, REG_NOT_USED, REG_TEMP_DBG1, REG_TEMP_DBG2, 0);
	ADD_LL_NEXT(new_ins, ins);

#if defined(USE_INSTRUCTION_COMMENTS)
	currentTranslation = "Call DebugRuntimePrintSegment()";
#endif

	ins = insertCall_To_C(codeSegment, ins, AL,(uint32_t)DebugRuntimePrintSegment, REG_HOST_STM_EABI);

#if defined(USE_INSTRUCTION_COMMENTS)
	currentTranslation = "Debug - Instruction count";
#endif

	int x=0;
	while (ins->nextInstruction->nextInstruction)
	{
		new_ins = newInstrI(ARM_MOV, AL, REG_EMU_DEBUG1, REG_NOT_USED, REG_NOT_USED, x);
		ADD_LL_NEXT(new_ins, ins);

		x++;
		ins = ins->nextInstruction;
	}
}
