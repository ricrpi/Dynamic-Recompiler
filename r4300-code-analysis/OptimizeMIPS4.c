/*
 * Optimize.c
 *
 *  Created on: 16 Apr 2014
 *      Author: rjhender
 */

#include "CodeSegments.h"

/*
 * Function to rearrange instructions so instructions linked by registers are closer together
 * e.g. XOR r1, r2, r3				XOR r1, r2, r3
 * 		ADD r5, r5, #4		=>		SUB r3, r3, #-1
 * 		SUB r3, r3, #-1 			ADD r5, r5, #4
 * 		...							...
 */
static void Optimize_R4300_Order(code_seg_t CodeSegment)
{
	int x=0;
	while (x < CodeSegment.MIPScodeLen)
	{

		x++;
	}
}

//=============================================================

void Optimize_R4300(code_seg_t CodeSegment)
{
	Optimize_R4300_Order(CodeSegment);
}
