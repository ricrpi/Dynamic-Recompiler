/*
 * Constants.c
 *
 *  Created on: 23 Oct 2014
 *      Author: rjhender
 */

#include "Translate.h"

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
