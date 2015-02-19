/*
 * callers.c
 *
 *  Created on: 27 Jan 2015
 *      Author: rjhender
 */

#include "CodeSegments.h"
#include "callers.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

//=================== Callers ========================================

static caller_t* newCaller(const code_seg_t* const caller)
{
	caller_t *newCaller;
	newCaller = malloc(sizeof(caller_t));

	assert(caller);

	newCaller->codeSeg = (code_seg_t*)caller;
newCaller->next = NULL;

#ifdef SHOW_CALLER
	printf("newCaller(%p)\n", caller);
#endif
	return newCaller;
}

void updateCallers(code_seg_t* const codeSegment)
{
	caller_t *caller;

	if (codeSegment->callers)
	{
		caller = codeSegment->callers;
#ifdef SHOW_CALLER
	printf("update Callers of codeSegment %p (invalidate branches)\n", codeSegment);
#endif

		while (caller)
		{
#ifdef SHOW_CALLER
			printf("\t%p, codeSeg %p\n", caller, caller->codeSeg);
#endif
			invalidateBranch(caller->codeSeg);
			caller->codeSeg = NULL;
			caller = caller->next;
		}
	}

}

void freeCallers(code_seg_t* const codeSegment)
{
	caller_t *prev;
	caller_t *next;

#ifdef SHOW_CALLER
	printf("free Callers of codeSegment %p\n", codeSegment);
#endif

	//remove any existing callers
	if (codeSegment->callers)
	{
		prev = codeSegment->callers;

		while (prev)
		{
#ifdef SHOW_CALLER
		printf("\t%p, codeSeg %p\n", prev, prev->codeSeg);
#endif
			next = prev->next;
			free(prev);
			prev = next;
		}
	}
	codeSegment->callers = NULL;
}

static void addToCallers(const code_seg_t* const caller, code_seg_t* const callee)
{
	caller_t *currentCaller;
	if (!callee) return;

	printf("addToCallers(%p, %p)\n",caller, callee);

	//remove any existing callers
	if (callee->callers)
	{
		currentCaller = callee->callers;


		while (currentCaller->next) currentCaller = currentCaller->next;

		currentCaller->next = newCaller(caller);
	}
	else
	{
		callee->callers = newCaller(caller);
	}
}
