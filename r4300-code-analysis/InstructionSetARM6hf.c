/*
 * ARMencode.c
 *
 *  Created on: 16 Apr 2014
 *      Author: rjhender
 */

#include "CodeSegments.h"
#include "InstructionSetARM6hf.h"

/*
 * R4300 code does not have a MOV instruction to load an immediate into a register
 * Instead it may use r0 (which is always 0) in combination with other instructions to load values
 * e.g. ORI, ANDI
 */
void EncodeSegment(code_seg_t CodeSegment)
{

}
