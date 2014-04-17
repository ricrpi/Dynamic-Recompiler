/*
 * Optimize.c
 *
 *  Created on: 16 Apr 2014
 *      Author: rjhender
 */

#include "CodeSegments.h"

/*
 * look for where multiple Loads or stores can be combined into an LDM STM instruction.
 * Also check for write-back.
 */
static void ARM_LDMSTM(code_seg_t CodeSegment)
{
	int x=0;
	while (x < CodeSegment.ARMcodeLen)
	{
		x++;
	}
}

/*
 * Function to check for loading to register followed by register usage
 *
 * This will cause a stall in the pipeline as the register is not ready on
 * the following instruction*/
static void ARM_LDR(code_seg_t CodeSegment)
{

}

/*
 * Function to look for ARM instructions that branch forward by less than 3 instructions
 * and make following code conditional.
 *
 * An ARM branch uses 3 instruction slots to refill the pipeline so by using
 * conditions we may be able to save some time.
 *
*/
static void ARM_BranchForward(code_seg_t CodeSegment)
{
	int x=0;
	while (x < CodeSegment.ARMcodeLen)
	{
		x++;
	}
}

void Optimize_ARM(code_seg_t CodeSegment)
{
	ARM_LDMSTM(CodeSegment);
	ARM_LDR(CodeSegment);
	ARM_BranchForward(CodeSegment);
}
