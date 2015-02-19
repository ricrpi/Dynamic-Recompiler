/*
 * literals.h
 *
 *  Created on: 27 Jan 2015
 *      Author: rjhender
 */

#ifndef RECOMPILER_LITERALS_H_
#define RECOMPILER_LITERALS_H_

#include <stdint.h>

typedef struct _literal_t
{
	struct _literal_t* next;
	int32_t value;
} literal_t;

#include "CodeSegments.h"

uint32_t addLiteral(struct _code_seg* const codeSegment,  regID_t* const base, int32_t* const offset, const uint32_t value);

void freeLiterals(struct _code_seg* codeSegment);

#endif /* RECOMPILER_LITERALS_H_ */
