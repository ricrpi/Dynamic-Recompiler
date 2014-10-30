
#include "Translate.h"

/*
 * Function to correct the offset to be applied for literal store/loading
 */
void Translate_Literals(code_seg_t* const codeSegment)
{
	Instruction_t*ins;
	ins = codeSegment->Intermcode;
	uint32_t x = 0;

	uint32_t InterimCodeLen = 0;
	uint32_t CountLiterals = 0;

	literal_t* l = codeSegment->literals;

#if defined(USE_INSTRUCTION_COMMENTS)
	currentTranslation = "Literals";
#endif

	while (l)
	{
		CountLiterals ++;
		l = l->next;
	}

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
				ins->offset = ins->offset - x -CountLiterals*4 - 8;

				if (ins->offset < 0)
				{
					ins->U = 0;
					ins->offset = -ins->offset;
				}

			}
			else if (codeSegment->Type == SEG_END)
			{
				ins->offset = InterimCodeLen * 4 - x + ins->offset - 8;	//TODO 4 or 8?
			}

			break;
		default:
			break;
		}
		ins = ins->nextInstruction;
		x+=4;
	}
}
