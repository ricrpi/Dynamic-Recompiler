/*
 * CodeSegments.h
 *
 *  Created on: 16 Apr 2014
 *      Author: rjhender
 */

#ifndef CODESEGMENTS_H_
#define CODESEGMENTS_H_

#include <stdint.h>


typedef enum
{
	BLOCK_INVALID,			// invalid code section
	BLOCK_PART,				// within a block
	BLOCK_END,				// block will not continue after this point (e.g. Jump with no link)
	BLOCK_CONTINUES			// block may/will continue after end of segment e.g. conditional branch, jump/branch with link etc.
} block_type_t;

typedef struct _code_seg
{
	struct _code_seg* nextCodeSegmentLinkedList;		//next code segment in linked list

	uint32_t* MIPScode;				// an index to mips code
	uint32_t MIPScodeLen;			// a length of mips code
	uint32_t MIPSReturnRegister;		// boolean segments returns;
	uint32_t* MIPSnextInstructionIndex;

	uint32_t MIPSRegistersUsed[3];		//The registers read/written by segment

	uint32_t MIPSRegistersUsedCount;//Count of the registers read/written by segment

	uint32_t* ARMcode;				// a pointer to arm code
	uint32_t ARMcodeLen;			// a length to arm code

	struct _code_seg* pCodeSegmentTargets[2];	// the code segment(s) we may branch to. will need relinking after DMA moves

	//uint32_t callers;			// array of code segments that may call this segment
	block_type_t blockType;		// if this is BLOCK_END then we can place literals afterward
} code_seg_t;


typedef struct _code_segment_data
{
	uint32_t count;
	code_seg_t* FirstSegment;
} code_segment_data_t;

code_segment_data_t* GenerateCodeSegmentData(uint32_t* ROM, uint32_t size);

/*
 * Provides the length of the code segment starting at uiMIPSword.
 * It will only scan up to uiNumInstructions. if it fails to jump within
 * uinumInstructions then the function will report the segment as invalid.
 *
 * On hitting a branch or jump it will stop searching and return the instruction count.
 *
 * if pJumpToAddress is provided then it will be populated with the address the
 * branch or jump would go to.
 */
int32_t validCodeSegment(
		uint32_t* puiCodeBase,
		uint32_t uiStart,
		uint32_t uiNumInstructions,
		uint32_t* pCodeLen,
		uint32_t* pJumpToAddress,
		uint32_t* ReturnRegister);


#endif /* CODESEGMENTS_H_ */
