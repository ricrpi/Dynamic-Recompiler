/*
 * CodeSegments.h
 *
 *  Created on: 16 Apr 2014
 *      Author: rjhender
 */

#ifndef CODESEGMENTS_H_
#define CODESEGMENTS_H_

#include <stdint.h>

typedef struct _code_seg
{
	uint32_t MIPScode;				// an address to mips code
	uint32_t* ARMcode;				// a pointer to arm code
	uint32_t MIPScodeLen;			// a length of mips code
	uint32_t ARMcodeLen;			// a length to arm code

	uint32_t ReturnRegister;		// boolean segments returns;
	uint32_t next;					// the address arm will jump to at end of this segment
	uint32_t callers;				// array of code segments that may call this segment

} code_seg_t;


typedef struct _code_segment_data
{
	uint32_t count;
	code_seg_t* segments;
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
