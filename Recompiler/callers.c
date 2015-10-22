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

#include "CodeSegments.h"
#include "callers.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

//=================== Callers ========================================

/*
 * Description: Function to malloc and initialize a new caller element for a caller linked list
 *
 * Parameters:	const code_seg_t* const caller		The code segment that contains the brancj/jump instruction
 *
 * Returns:		caller_t*
 */
static caller_t* newCaller(const code_seg_t* const caller)
{
	caller_t *newCaller;
	newCaller = malloc(sizeof(caller_t));

	assert(caller);

	newCaller->codeSeg = (code_seg_t*)caller;
	newCaller->next = NULL;

#if SHOW_CALLER
	printf("newCaller(%p)\n", caller);
#endif
	return newCaller;
}

/*
 * Description: Function to Invalidate all segments that branch to the provided codeSegment and free associated objects
 *
 * 				Other segments will be modified to branch to FUNC_GEN_BRANCH_UNKNOWN if they
 * 				have been 'patched' to branch to this codeSegment.
 *
 * 				This function is so that a codeSegment can be deleted without leaving any references to it
 * 				within generated code.
 *
 * Parameters:	code_seg_t* const codeSegment		The code segment that can no longer be jumped directly to
 */
void freeCallers(code_seg_t* const codeSegment)
{
	caller_t *prev;
	caller_t *next;

#if SHOW_CALLER
	printf("free Callers of codeSegment %p\n", codeSegment);
#endif

	//remove any existing callers
	if (codeSegment->callers != NULL)
	{
		prev = codeSegment->callers;

		while (prev != NULL)
		{
			if (prev->codeSeg != NULL)
			{
#if SHOW_CALLER
				printf("\t%p, codeSeg %p\n", prev, prev->codeSeg);
#endif
				invalidateBranch(prev->codeSeg);
			}

			next = prev->next;
			free(prev);
			prev = next;
		}
	}
	codeSegment->callers = NULL;
}

/*
 * Description: Function to add a caller to the callees linked list.
 *
 * 				TODO: check for duplicates?
 *
 * Parameters:	const code_seg_t* const caller		The segment contaiming the branch instruction
 *				code_seg_t* const callee			The segment being branched to
 *
 */
void addToCallers(const code_seg_t* const caller, code_seg_t* const callee)
{
	if (caller != NULL && callee != NULL)
	{
#if SHOW_CALLER
		printf("addToCallers(%p, %p)\n",caller, callee);
#endif
		if (callee->callers != NULL)
		{
			caller_t *currentCaller = callee->callers;

			// go to the end of the linked list
			while (currentCaller->next)
			{
				currentCaller = currentCaller->next;
			}

			// append to last caller in list
			currentCaller->next = newCaller(caller);
		}
		else
		{
			// set the first element in the caller linked list
			callee->callers = newCaller(caller);
		}
	}
}
