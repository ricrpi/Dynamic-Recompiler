/*
 * OptimizeARM.c
 *
 *  Created on: 23 Oct 2014
 *      Author: rjhender
 */

#include "Translate.h"

void Translate_LoadStoreWriteBack(code_seg_t* const codeSegment)
{
	Instruction_t*ins;
	ins = codeSegment->Intermcode;
#if defined(USE_INSTRUCTION_COMMENTS)
	currentTranslation = "LoadStoreWriteBack";
#endif

	while (ins)
	{
		ins = ins->nextInstruction;
	}
}
