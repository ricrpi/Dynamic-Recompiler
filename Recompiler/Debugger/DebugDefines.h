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

// ========= Runtime Configurable Debugging Information ==============================
/* useage:
 * 			0	Never
 * 			1	Runtime Configurable
 * 			2	Always
*/

// Print the register map when registers re-assigned (TranslateRegiters())
#define SHOW_REG_TRANSLATION_MAP			(1)
#define SHOW_REG_TRANSLATION_MAP_PROGRESS	(1)

//Print when a segment is deleted
#define SHOW_PRINT_SEGMENT_DELETE			(1)

// ========= Debugging Information ===============================================

// Print out changes code segment callers
#define SHOW_CALLER							(1)

// Print the constants in intermediate code
#define SHOW_PRINT_INT_CONST				(0)

// Print the raw Hex values when reading arm instructions (arm_decode)
#define SHOW_PRINT_ARM_VALUE				(0)

#define SHOW_PRINT_MIPS_VALUE				(0)

// Print out changes to the code segment mapping
#define SHOW_CODE_SEG_MAP_CHANGES			(0)

// Print out information on the CompileAt() call. This call finds new code segments
#define SHOW_COMPILEAT_STEPS				(1)

// ========= Recompiling Options =======================================

// Print MIPS code / Translation step for each intermediate instruction
#define USE_INSTRUCTION_COMMENTS			(1)

//store the initial MIPS registers assigned to intermediate instruction
#define USE_INSTRUCTION_INIT_REGS			(1)

// Use the translation that adds debugging markers

#define USE_TRANSLATE_DEBUG					(0)

#define USE_TRANSLATE_DEBUG_SET_CURRENT_SEG	(0)

#define USE_TRANSLATE_DEBUG_PRINT_SEGMENT	(0)

#define USE_TRANSLATE_DEBUG_PRINT_REGISTERS_ON_ENTRY (0)

// causes the debugger to start at the end of code segment execution
#define USE_TRANSLATE_DEBUG_BREAK_AT_END	(0)

#define USE_TRANSLATE_DEBUG_LINE_NUMBERS	(0)

#define USE_BREAKPOINTS						(0)

// You can strip the REG_HOST flags when emiting arm code
// or when registers are translated.
#define DO_HOSTREG_RENUMBER_IN_TRANSLATIONS	(0)

// Switch to decide whether to push/pop host LR register for branching and returning (1)
// or  branch without link and lookup return address to return (0)
#define USE_HOST_MANAGED_BRANCHING			(1)

// ========= Customize Aborts for debugging ===========================

#define ABORT_ARM_DECODE					(0)

#define ABORT_ARM_ENCODE					(1)

#define ABORT_EXCEEDED_GLOBAL_OFFSET		(1)

#define ABORT_UNKNOWN_TRANSLATE				(1)

#define ABORT_MEMORY_LEAK					(0)

#endif /* DEBUGDEFINES_H_ */
