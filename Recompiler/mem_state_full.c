/*
 * mmap_full.c
 *
 *  Created on: 27 Jan 2015
 *      Author: rjhender
 */

#include <stdio.h>
#include <stdlib.h>
#include "mem_state.h"
#include "CodeSegments.h"


static memMap_t** Blocks = NULL;
static uint32_t memMapCount = 0;

void initMemState(memMap_t* MemoryBlocks, uint32_t Count)
{
	Blocks = malloc(Count * sizeof(memMap_t));
	memMapCount = Count;

	memcpy(Blocks, MemoryBlocks, Count * sizeof(memMap_t));

	int x;

	for (x=0; x< Count; x++)
	{
		Blocks[x]->_memStatePtr = malloc(Blocks[x]->size * sizeof(code_seg_t*));
		memset(Blocks[x]->_memStatePtr,0, Blocks[x]->size * sizeof(code_seg_t*));
	}
}

code_seg_t* getSegmentAt(size_t address)
{
	int x;

	for (x=0; x < memMapCount; x++)
	{
		if (Blocks[x]->address <= address && Blocks[x]->address + Blocks[x]->size > address)
		{
			return Blocks[x]->_memStatePtr[(address - Blocks[x]->address)/sizeof(code_seg_t*)];
		}
	}

	printf("getSegmentAt(0x%08x) failed. Not a valid memory address\n", address);

	return NULL;
}

void setMemState(size_t address, uint32_t length, code_seg_t* codeSeg)
{
	int x;

	for (x=0; x < memMapCount; x++)
	{
		if (Blocks[x]->address <= address && Blocks[x]->address + Blocks[x]->size > address)
		{
			int i;

			for (i=0; i < length; i++)
			{
				code_seg_t* Blockcurrent = Blocks[x]->_memStatePtr[(address - Blocks[x]->address)/sizeof(code_seg_t*) + i];
				if (!codeSeg && Blockcurrent)
				{
					if (Blockcurrent->callers || Blockcurrent->Intermcode )
					{
						printf("memory leak - Codesegment data not deleted before removing references\n");
						abort();
					}
				}
				Blockcurrent = codeSeg;
				return;
			}
		}
	}

		printf("setMemState(0x%08x, %d, 0x%08x)\n", address, length, (uint32_t)codeSeg);
		abort();
}
