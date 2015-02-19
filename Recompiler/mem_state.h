/*
 * mem_state.h
 *
 *  Created on: 27 Jan 2015
 *      Author: rjhender
 */

#ifndef RECOMPILER_MEM_STATE_H_
#define RECOMPILER_MEM_STATE_H_

#include <stdint.h>
#include <stddef.h>
#include "CodeSegments.h"

typedef struct
{
	uint64_t address;
	uint64_t size;
	code_seg_t** _memStatePtr;
} memMap_t;

void initMemState(memMap_t* MemoryBlocks, uint32_t Count);

code_seg_t* getSegmentAt(size_t address);

void setMemState(size_t address, uint32_t length, code_seg_t* codeSeg);

#endif /* RECOMPILER_MEM_STATE_H_ */
