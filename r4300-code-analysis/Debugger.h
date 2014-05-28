/*
 * Debugger.h
 *
 *  Created on: 21 May 2014
 *      Author: rjhender
 */

#ifndef DEBUGGER_H_
#define DEBUGGER_H_

void Debugger_start(code_segment_data_t* segmentData);


#define HELP_PRINT "\tprint mips\tPrint current segment MIPS code\n"\
		"\tprint arm\tPrint current segment ARM code\n"

#define HELP_SEG "\tseg [x] \tChange to segment at x\n" \
		"\tseg\tPrint Summary for code segment\n"

#define HELP_GEN "R4300 debugger:\n\n"\
		"Usage:\n"\
		"\thelp\t\tPrints this Help page\n"\
		HELP_PRINT \
		HELP_SEG

#endif /* DEBUGGER_H_ */
