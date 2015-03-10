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
