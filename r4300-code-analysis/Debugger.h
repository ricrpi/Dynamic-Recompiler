/*
 * Debugger.h
 *
 *  Created on: 21 May 2014
 *      Author: rjhender
 */

#ifndef DEBUGGER_H_
#define DEBUGGER_H_

void Debugger_start(const code_segment_data_t* const segmentData);

#define HELP_TRANS 	"\ttranslate                         Translate Current Segment\n" \
					"\ttranslate full                    Translate Current Segment and Generate ARM code\n" \
					"\ttranslate DelaySlot               Perform DelaySlot Translation\n"\
					"\ttranslate CountRegister           Perform CountRegister Translation\n"\
					"\ttranslate 32BitRegisters          Perform 32BitRegisters Translation\n"\
					"\ttranslate ReduceRegistersUsed     Perform ReduceRegistersUsed Translation\n"\
					"\ttranslate LoadStoreWriteBack      Perform LoadStoreWriteBack Translation\n"\
					"\ttranslate memory                  Perform Memory Translation\n"\
					"\ttranslate write                   Generate ARM code from Intermediate code\n"

#define HELP_PRINT 	"\tprint mips                        Print current segment MIPS code\n"\
					"\tprint mips [x]                    Print [x] instructions from current segment MIPS code\n"\
					"\tprint mips [x] [y]                Print [y] MIPS code instructions starting from address [x]\n"\
					"\tprint intermediate                Print intermediate code\n" \
					"\tprint literal                     Print literals for this code segment\n" \
					"\tprint arm                         Print current segment ARM code\n"\
					"\tprint arm [x]                     Print [x] instructions from current segment ARM code\n" \
					"\tprint arm [x] [y]                 Print [y] ARM code instructions starting from address [x]\n"

#define HELP_SEG 	"\tseg [1|2]                         Change to segment, current Segment branches/continues to\n" \
					"\tseg 0                             Change to First segment\n" \
					"\tseg                               Print Summary for code segment\n"

#define HELP_GEN 	"R4300 code analysis debugger:\n\n"\
					"Usage:\n"\
					"\thelp                              Prints this Help page\n"\
					HELP_PRINT "\n"\
					HELP_TRANS "\n"\
					HELP_SEG "\n"

#endif /* DEBUGGER_H_ */
