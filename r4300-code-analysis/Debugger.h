/*
 * Debugger.h
 *
 *  Created on: 21 May 2014
 *      Author: rjhender
 */

#ifndef DEBUGGER_H_
#define DEBUGGER_H_

void Debugger_start(code_segment_data_t* segmentData);


#define HELP_PRINT ""

#define HELP_SEG "\tseg [x] mips\tShow segment MIPS code x\n" \
		"\tseg [x] arm\tShow segment ARM code\n"\
		"\tseg [x]\tPrint Summary for code segment\n"

#define HELP_GEN "R4300 debugger:\n\n"\
		"Usage:\n"\
		"\thelp\t\tPrints this Help page\n"\
		HELP_PRINT \
		HELP_SEG




#endif /* DEBUGGER_H_ */
