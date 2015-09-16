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

#if defined(USE_BREAKPOINTS) || defined(USE_TRANSLATE_DEBUG_SET_CURRENT_SEG)
#if defined(USE_INSTRUCTION_COMMENTS)
	currentTranslation = "Debug - Get CodeSegment";
#endif
	regID_t base;
	int32_t offset;

	//TODO Nasty global segmentData!

	//load current segment Address
	addLiteral(codeSegment, &base, &offset, (uint32_t)codeSegment);
	codeSegment->Intermcode = new_ins = newInstrI(ARM_LDR_LIT, AL, REG_TEMP_SCRATCH0, REG_NOT_USED, base, offset);
	new_ins->nextInstruction = ins;
	ins = new_ins;
#endif

#if defined(USE_BREAKPOINTS)
#if defined(USE_INSTRUCTION_COMMENTS)
	currentTranslation = "Debug - Service Breakpoints";
#endif
	new_ins = newInstrPUSH(AL, REG_HOST_STM_ALL);
	ADD_LL_NEXT(new_ins, ins);

	new_ins = newInstr(ARM_MOV, AL, REG_HOST_R1, REG_NOT_USED, REG_HOST_SP);
	ADD_LL_NEXT(new_ins, ins);

	ins = insertCall_To_C(codeSegment, ins, AL,(uint32_t)ServiceBreakPoint, 0);

	new_ins = newInstrPOP(AL, REG_HOST_STM_ALL);
	ADD_LL_NEXT(new_ins, ins);
#endif

#if defined(USE_INSTRUCTION_COMMENTS)
	currentTranslation = "Debug - Load Segment ID";
#endif

#if defined(USE_TRANSLATE_DEBUG_SET_CURRENT_SEG)

	//load segmentData->dbgCurrentSegment address
	addLiteral(codeSegment, &base, &offset, (uint32_t)&segmentData.dbgCurrentSegment);
	new_ins 		= newInstrI(ARM_LDR_LIT, AL, REG_TEMP_SCRATCH1, REG_NOT_USED, base, offset);
	ADD_LL_NEXT(new_ins, ins);

#if defined(USE_INSTRUCTION_COMMENTS)
	currentTranslation = "Debug - Store Segment ID";
#endif
#endif

	//store
	new_ins 		= newInstrI(ARM_STR, AL, REG_NOT_USED, REG_TEMP_SCRATCH0, REG_TEMP_SCRATCH1, 0);
	ADD_LL_NEXT(new_ins, ins);

#if defined(USE_TRANSLATE_DEBUG_PRINT_SEGMENT)
#if defined(USE_INSTRUCTION_COMMENTS)
	currentTranslation = "Call DebugRuntimePrintSegment()";
#endif
	ins = insertCall_To_C(codeSegment, ins, AL,(uint32_t)DebugRuntimePrintSegment, 0);
#endif

#if defined(USE_TRANSLATE_DEBUG_PRINT_REGISTERS_ON_ENTRY)
#if defined(USE_INSTRUCTION_COMMENTS)
	currentTranslation = "Call DebugRuntimePrintMIPS()";
#endif
	{
		static code_seg_t* lastSeg = NULL;

		if (lastSeg != codeSegment)
		{
			ins = insertCall_To_C(codeSegment, ins, AL,(uint32_t)DebugRuntimePrintMIPS, 0);
			lastSeg = codeSegment;
		}
	}
#endif

#if defined(USE_INSTRUCTION_COMMENTS)
	currentTranslation = "Debug - Instruction count";
#endif

#if defined(USE_TRANSLATE_DEBUG_LINE_NUMBERS)
	int x=0;
	while (ins->nextInstruction->nextInstruction)
	{
		new_ins = newInstrI(ARM_MOV, AL, REG_EMU_DEBUG1, REG_NOT_USED, REG_NOT_USED, x);
		ADD_LL_NEXT(new_ins, ins);

		x++;
		ins = ins->nextInstruction;
	}
#endif
}
