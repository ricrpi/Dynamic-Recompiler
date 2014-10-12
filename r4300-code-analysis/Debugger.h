/*
 * Debugger.h
 *
 *  Created on: 21 May 2014
 *      Author: rjhender
 */

#ifndef DEBUGGER_H_
#define DEBUGGER_H_

int Debugger_start(const code_segment_data_t* const segmentData);

#define HELP_TRANS 	"\ttranslate                         Translate Current Segment\n" \
					"\ttranslate full                    Translate Current Segment and Generate ARM code\n\n" \
					"\ttranslate init                    Initialize segment for translation\n"\
					"\ttranslate DelaySlot               Perform DelaySlot Translation\n"\
					"\ttranslate Count                   Perform Count Register Translation\n"\
					"\ttranslate Constants               Perform Constant Optimization\n"\
					"\ttranslate 32BitRegisters          Perform 32BitRegisters Translation\n"\
					"\ttranslate memory                  Perform Memory Translation\n"\
					"\ttranslate LoadStoreWriteBack      Perform LoadStoreWriteBack Translation\n"\
					"\ttranslate LoadCacheRegisters      Load Register from Cache\n"\
					"\ttranslate Registers               Perform Register renumbering\n"\
					"\ttranslate Branch                  Perform Branch Translation\n"\
					"\ttranslate StoreCacheRegisters     Store Registers to Cache\n"\
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

//===============================================================

/*
 * 'CodeStart' will only generate the first #defined Test below or
 * will generate the default startup code for emulation.
 *
 * Type 'Start' on the debugger command prompt to begin executing
 * the test/emulation.
 */
//#define TEST_BRANCHING_FORWARD
//#define TEST_BRANCHING_BACKWARD
#define TEST_BRANCH_TO_C
//#define TEST_LITERAL



#endif /* DEBUGGER_H_ */
