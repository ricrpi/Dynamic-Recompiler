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

#ifndef DEBUGGER_H_
#define DEBUGGER_H_

#include <sys/ucontext.h>
#include "CodeSegments.h"


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


#define HELP_PRINT 	"\tprint arm                         Print current segment ARM code\n"\
					"\tprint arm [x]                     Print [x] instructions from current segment ARM code\n" \
					"\tprint arm [x] [y]                 Print [y] ARM code instructions starting from address [x]\n" \
					"\tprint intermediate                Print intermediate code\n" \
					"\tprint literal                     Print literals for this code segment\n" \
					"\tprint lookup [x] [y]              Print CodeSegment corresponding to mips address [x] to [x + y]\n"\
					"\tprint mips                        Print current segment MIPS code\n"\
					"\tprint mips [x]                    Print [x] instructions from current segment MIPS code\n"\
					"\tprint mips [x] [y]                Print [y] MIPS code instructions starting from address [x]\n"\
					"\tprint reg                         Print registers\n" \
					"\tprint reg mips                    Print mips registers\n" \
					"\tprint reg arm                     Print arm registers\n" \
					"\tprint value [x] [y]               Print raw values from address [x] to [x + y]\n"

#define HELP_SET	"\tshowPrintSegmentDelete            Print when segment is deleted\n"\
					"\tshowRegTranslationMap             Print out mapping between HOST registers and emulated regisers\n"\
					"\tshowRegTranslationMapProgress     Print register mapping info on-the-fly\n"

#define HELP_SEG 	"\tseg [1|2]                         Change to segment, current Segment branches/continues to\n" \
					"\tseg 0                             Change to First segment\n" \
					"\tseg start                         Change to START segment\n" \
					"\tseg stop                          Change to STOP segment\n" \
					"\tseg                               Print Summary for code segment\n"

#define HELP_COMP	"\tcompile [addr]                    Compile the MIPS code at address\n"

#define HELP_GEN 	"R4300 code analysis debugger:\n\n"\
					"Usage:\n"\
					"\thelp                              Prints this Help page\n"\
					HELP_PRINT "\n"\
					HELP_TRANS "\n"\
					HELP_SET "\n"\
					HELP_SEG "\n"\
					HELP_COMP "\n"\
					"set [parameter] [value]\n"

void Debugger_wrapper();

void ServiceBreakPoint(code_seg_t* codeSeg, size_t* regs);

int Debugger_start(const code_segment_data_t* const segmentData, mcontext_t* context, size_t* regs);

void DebugRuntimePrintMIPS();
#endif /* DEBUGGER_H_ */
