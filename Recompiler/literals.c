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

#include "literals.h"
#include "CodeSegments.h"
#include "memory.h"
#include <stdio.h>
#include <stdlib.h>

static uint32_t GlobalLiteralCount = 0;
//================== Literals ========================================

static literal_t* newLiteral(const uint32_t value)
{
	literal_t* newLiteral = malloc(sizeof(literal_t));

	newLiteral->value = value;
	newLiteral->next = NULL;

	return newLiteral;
}

void freeLiterals(code_seg_t* codeSegment)
{
	literal_t *current;
	literal_t *next;

	//remove any existing literals
	if (codeSegment->literals)
	{
		current = codeSegment->literals;

		while (current)
		{
			next = current->next;
			free(current);
			current = next;
		}
	}
	codeSegment->literals = NULL;
}

// Function: addLiteral
//
// Description:	Function that adds a literal to a codesegment.
//
// Parameters:	code_seg_t* const codeSegment	The segment to add literal to
//				regID_t* const base				Variable to receive the register to use for literal loading
//				int32_t* const offset			Variable to receive the offset for literal loading
//				const uint32_t value			The value of the literal
//
// Returns:		Zero
//
// TODO what if segment length means offset > 4096?
uint32_t addLiteral(code_seg_t* const codeSegment, regID_t* const base, int32_t* const offset, const uint32_t value)
{
	int index = 4;

	// If the literal is already global then might as well use it
	int x;
	for (x=1; x < 1024; x++)		//Scan existing global literals for Value
	{
		if (*((uint32_t*)MMAP_FP_BASE - x) == value)
		{
			*offset = x * 4;
			*base = REG_EMU_FP;
			return 0;
		}
	}

	// Now we need to add a new literal
	//If this is a SEG_SANDWICH then needs to be global
	if (SEG_SANDWICH == codeSegment->Type)
	{
		if (GlobalLiteralCount >= 1024)
		{
			printf("CodeSegments.c:%d Run out of Global Literal positions\n", __LINE__);
#if defined(ABORT_EXCEEDED_GLOBAL_OFFSET)
			abort();
#endif
		}

		GlobalLiteralCount++;

		*offset = -(GlobalLiteralCount)*4;
		*base = REG_EMU_FP;
		*((uint32_t*)MMAP_FP_BASE - GlobalLiteralCount) = value;

		return 0;
	}
	else if (codeSegment->literals)
	{
		literal_t* nxt = codeSegment->literals;
		while (nxt->next)
		{
			if (nxt->value == value) break; // already exists

			index+=4;
			nxt = nxt->next;
		}

		if (nxt->value == value)
		{
			*offset = index;
			*base = REG_HOST_PC;
		}
		else
		{
			nxt->next = newLiteral(value);
			*offset = index;
			*base = REG_HOST_PC;
		}
	}
	else	// codeSegment does not have any literals yet
	{
		codeSegment->literals = newLiteral(value);
		*offset = 0;
		*base = REG_HOST_PC;
	}

	return 0;
}

