/*
 * literals.c
 *
 *  Created on: 27 Jan 2015
 *      Author: rjhender
 */

#include "literals.h"
#include "CodeSegments.h"
#include "memory.h"


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

