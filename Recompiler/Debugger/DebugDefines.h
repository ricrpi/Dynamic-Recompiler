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

#ifndef DEBUGDEFINES_H_
#define DEBUGDEFINES_H_


// ========= TEST Setups ==============================================

//#define TEST_BRANCHING_FORWARD
//#define TEST_BRANCHING_BACKWARD
//#define TEST_BRANCH_TO_C
//#define TEST_LITERAL
//#define TEST_ROR

// ========= Extra Debugging Information ==============================

/* useage:
 * 			0	Never
 * 			1	Runtime Configurable
 * 			2	Always
*/
// Print the register map when registers re-assigned (TranslateRegiters())
#define SHOW_REG_TRANSLATION_MAP			(1)
#define SHOW_REG_TRANSLATION_MAP_PROGRESS	(1)

//Print when a segment is deleted
#define SHOW_PRINT_SEGMENT_DELETE			(0)

#define SHOW_CALLER							(1)

// ========================================================

// Print the constants in intermediate code
//#define SHOW_PRINT_INT_CONST

//Print the raw Hex values when reading arm instructions (arm_decode)
//#define SHOW_PRINT_ARM_VALUE

//#define SHOW_PRINT_MIPS_VALUE

// Print MIPS code / Translation step for each intermediate instruction
#define USE_INSTRUCTION_COMMENTS

//store the initial MIPS registers assigned to intermediate instruction
#define USE_INSTRUCTION_INIT_REGS

// Use the translation that adds debugging markers

//#define USE_TRANSLATE_DEBUG						// think this must be active for branch unknowns to work!
//#define USE_TRANSLATE_DEBUG_SET_CURRENT_SEG		// think this must be active for branch unknowns to work!
//#define USE_TRANSLATE_DEBUG_PRINT_SEGMENT
//#define USE_TRANSLATE_DEBUG_PRINT_REGISTERS_ON_ENTRY
//#define USE_TRANSLATE_DEBUG_BREAK_AT_END		// causes the debugger to start at the end of code segment execution
//#define USE_TRANSLATE_DEBUG_LINE_NUMBERS
//#define USE_BREAKPOINTS

// ========= Recompiling Checks =======================================

// You can strip the REG_HOST flags when emiting arm code
// or when registers are translated.

//#define DO_HOSTREG_RENUMBER_IN_TRANSLATIONS
//#define ASSERT_ARM_NOT_COMPILED

#define SHOW_CODE_SEG_MAP_CHANGES

// ========= Customize Aborts for debugging ===========================

//#define ABORT_ARM_DECODE
#define ABORT_ARM_ENCODE
#define ABORT_EXCEEDED_GLOBAL_OFFSET
#define ABORT_UNKNOWN_TRANSLATE

#endif /* DEBUGDEFINES_H_ */
