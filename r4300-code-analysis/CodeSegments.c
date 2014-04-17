/*
 * CodeSegments.c
 *
 *  Created on: 16 Apr 2014
 *      Author: rjhender
 */

#include "CodeSegments.h"
#include "InstructionSetMIPS4.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static code_segment_data_t segmentData = {0, 0};

int32_t ops_validCodeSegment(uint32_t* puiCodeBase, uint32_t uiStart, uint32_t uiNumInstructions, uint32_t* pCodeLen, uint32_t* pJumpToAddress, uint32_t* ReturnRegister);


code_segment_data_t* GenerateCodeSegmentData(uint32_t* ROM, uint32_t size)
{
	int x,y;
	uint32_t segmentCount = 0;

	//find the number of segments

	for (x=64/4; x< size/4; )
	{
		uint32_t uiLen;
		int bValid = ops_validCodeSegment(ROM, x, 4096, &uiLen, NULL, NULL);

		if (bValid && ROM[x])
		{
			printf("decoding segment 0x%08X with %d instructions\n", x*4, uiLen);

			for (y=x; y < x+ uiLen; y++) ops_decode((y)*4, ROM[y]);

			//printf("%3d, br=0x%08X | ", len, br*4);

			printf("\n");

			//ops_decode((x+len-1)*4, ROM[x+len-1]);
			segmentCount ++;
		}
		x += uiLen;

	}

	segmentData.count = segmentCount;
	segmentCount = 0;

	//in case function has been called before
	if (segmentData.segments) free(segmentData.segments);

	//now we have a count we can calloc for the segment data array
	//TODO malloc protection
	segmentData.segments = malloc(segmentCount * sizeof(code_seg_t));
	memset(segmentData.segments, 0, segmentCount * sizeof(code_seg_t));

	for (x=64/4; x< size/4; )
	{
		uint32_t len, uiBranchAddress, returnReg;
		int bValid = ops_validCodeSegment(&ROM[x], x, 4096, &len, &uiBranchAddress, &returnReg);

		if (bValid && ROM[x])
		{
			segmentData.segments[segmentCount].MIPScode = x * 4;
			segmentData.segments[segmentCount].MIPScodeLen = len;
			segmentData.segments[segmentCount].ARMcodeLen = 0;
			segmentData.segments[segmentCount].next = uiBranchAddress;
			segmentData.segments[segmentCount].ReturnRegister = returnReg;

			segmentCount ++;
		}

		x += len;

	}

	return &segmentData;
}


int32_t ops_validCodeSegment(uint32_t* puiCodeBase, uint32_t uiStart, uint32_t uiNumInstructions, uint32_t* pCodeLen, uint32_t* pJumpToAddress, uint32_t* ReturnRegister)
{
	int x;

	for(x=uiStart; x<uiNumInstructions; x++)
	{
		int op = ops_type(puiCodeBase[x]);

		if (INVALID == op)
		{
			if (pJumpToAddress) *pJumpToAddress = 0;
			if (pCodeLen) *pCodeLen = x + 1;
			if (ReturnRegister) *ReturnRegister = 0;
			return 0;
		}

		else if ( (op & OPS_JUMP) == OPS_JUMP)
		{
			if (pJumpToAddress) *pJumpToAddress = ops_JumpAddressOffset(puiCodeBase[x]);
			if (pCodeLen) *pCodeLen = x - uiStart + 1;

			if (op == JR) //only JR can set PC to the Link Register (or other register!)
			{
				if (ReturnRegister) *ReturnRegister = (puiCodeBase[x]>>21)&0x1f;
			}
			return 1;
		}
		/*
		 * else if ( (op & OPS_BRANCH) == OPS_BRANCH)
		{
			if (pJumpToAddress) *pJumpToAddress = ops_JumpAddressOffset(puiCodeBase[x]);
			if (pCodeLen) *pCodeLen = x - uiStart + 1;
			return 1;
		}
		*/
		else if ( (op & OPS_CALL) == OPS_CALL)
		{
			if (pJumpToAddress) *pJumpToAddress = (uint32_t)(x) + ops_JumpAddressOffset(puiCodeBase[x]);
			if (pCodeLen) *pCodeLen = x - uiStart + 1;
			if (ReturnRegister) *ReturnRegister = 0;
			return 1;
		}
	}
	if (pCodeLen) *pCodeLen = uiNumInstructions + 1;
	if (ReturnRegister) *ReturnRegister = 0;

	return 0;
}
