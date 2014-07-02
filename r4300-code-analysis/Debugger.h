/*
 * Debugger.h
 *
 *  Created on: 21 May 2014
 *      Author: rjhender
 */

#ifndef DEBUGGER_H_
#define DEBUGGER_H_

void Debugger_start(code_segment_data_t* segmentData);

#define HELP_OPT 	"\topt                         Optimize Current Segment\n" \
					"\topt DelaySlot               Perform DelaySlot Optimization"\
					"\topt CountRegister           Perform CountRegister Optimization"\
					"\topt 32BitRegisters          Perform 32BitRegisters Optimization"\
					"\topt ReduceRegistersUsed     Perform ReduceRegistersUsed Optimization"\
					"\topt LoadStoreWriteBack      Perform LoadStoreWriteBack Optimization"

#define HELP_PRINT 	"\tprint mips                  Print current segment MIPS code\n"\
					"\tprint intermediate          Print intermediate code\n" \
					"\tprint arm                   Print current segment ARM code\n"

#define HELP_SEG 	"\tseg [x]                     Change to segment at x\n" \
					"\tseg                         Print Summary for code segment\n"

#define HELP_GEN 	"R4300 debugger:\n\n"\
					"Usage:\n"\
					"\thelp\t\tPrints this Help page\n"\
					HELP_PRINT \
					HELP_OPT \
					HELP_SEG

#endif /* DEBUGGER_H_ */
