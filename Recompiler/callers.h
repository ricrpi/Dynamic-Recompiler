/*
 * callers.h
 *
 *  Created on: 27 Jan 2015
 *      Author: rjhender
 */

#ifndef RECOMPILER_CALLERS_H_
#define RECOMPILER_CALLERS_H_

#include "CodeSegments.h"

typedef struct _caller_t
{
	struct _caller_t* next;
	struct _code_seg* codeSeg;
} caller_t;

struct _code_seg;

void updateCallers(struct _code_seg* const codeSegment);

#endif /* RECOMPILER_CALLERS_H_ */
