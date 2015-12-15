/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *                                                                         *
 *  Dynamic-Recompiler - Turns MIPS code into ARM code                       *
 *  Original source: http://github.com/ricrpi/Dynamic-Recompiler             *
 *  Copyright (C) 2015  Richard Hender                                       *
 *                                                                           *
 *  This program is free software: you can redistribute it and/or modify     *
 *  it under the terms of the GNU General Public License as published by     *
 *  the Free Software Foundation, either version 3 of the License, or        *
 *  (at your option) any later version.                                      *
 *                                                                           *
 *  This program is distributed in the hope that it will be useful,          *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *  GNU General Public License for more details.                             *
 *                                                                           *
 *  You should have received a copy of the GNU General Public License        *
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.    *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "mem_state.h"
#include "CodeSegments.h"
#include "Debugger.h"
#include "DebugDefines.h"

static memMap_t* Blocks = NULL;
static uint32_t memMapCount = 0U;


/*
 * Description: Function to malloc the address / code Segment mapping
 *
 *
 * Parameters:	memMap_t* MemoryBlocks		An array of memory block descriptions
 * 				uint32_t Count				The number of block descriptions in the array.
 *
 */
void initMemState(memMap_t MemoryBlocks[], uint32_t Count)
{
	Blocks = malloc(Count * sizeof(memMap_t));
	memMapCount = Count;

	memcpy(Blocks, MemoryBlocks, Count * sizeof(memMap_t));

	uint32_t x;

	for (x=0U; x< Count; x++)
	{
		Blocks[x]._memStatePtr = malloc(Blocks[x].size * sizeof(code_seg_t*));
		memset(Blocks[x]._memStatePtr,0, Blocks[x].size * sizeof(code_seg_t*));
	}
}

/*
 * Description: Function to free the address / code Segment mapping
 *
 *
 * Parameters:	none
 *
 */
void freeMemState()
{
	uint32_t x;

	for (x=0U; x< memMapCount; x++)
	{
		free(Blocks[x]._memStatePtr);
	}

	free(Blocks);
}

code_seg_t* getSegmentAt(uintptr_t address)
{
	int x;

	for (x=0; x < memMapCount; x++)
	{
		if (Blocks[x].address <= address && Blocks[x].address + Blocks[x].size > address)
		{
			return Blocks[x]._memStatePtr[(address - Blocks[x].address)/sizeof(code_seg_t*)];
		}
	}

	printf("getSegmentAt(0x%08x) failed. Not a valid memory address\n", address);

	return NULL;
}

void setMemState(const uintptr_t address, const uint32_t length, const code_seg_t* codeSeg)
{
	int x;

	for (x=0; x < memMapCount; x++)
	{
		if (Blocks[x].address <= address && Blocks[x].address + Blocks[x].size > address)
		{
			int i;

			for (i=0; i < length; i++)
			{
				code_seg_t* Blockcurrent = Blocks[x]._memStatePtr[(address - Blocks[x].address)/sizeof(code_seg_t*) + i];
				if (!codeSeg && Blockcurrent)
				{
					if (Blockcurrent->callers != NULL
							|| Blockcurrent->Intermcode != NULL)
					{
						printf("memory leak - Codesegment data not deleted before removing references\n");
#if ABORT_MEMORY_LEAK
						abort();
#endif
					}
				}
				Blocks[x]._memStatePtr[(address - Blocks[x].address)/sizeof(code_seg_t*) + i] = codeSeg;
			}

#if SHOW_CODE_SEG_MAP_CHANGES
			printf("code seg map address 0x%08x to 0x%08x set to 0x%x\n", address, address + length*4, (size_t)codeSeg);
#endif
			return;
		}
	}

		printf("setMemState(0x%08x, %d, 0x%08x)\n", address, length, (uintptr_t)codeSeg);
		abort();
}
