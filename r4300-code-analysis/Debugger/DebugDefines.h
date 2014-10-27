/*
 * DebugDefines.h
 *
 *  Created on: 23 Oct 2014
 *      Author: rjhender
 */

#ifndef DEBUGDEFINES_H_
#define DEBUGDEFINES_H_


// ========= TEST Setups ==============================================

//#define TEST_BRANCHING_FORWARD
//#define TEST_BRANCHING_BACKWARD
//#define TEST_BRANCH_TO_C
//#define TEST_LITERAL

// ========= Extra Debugging Information ==============================

//#define SHOW_REG_TRANSLATION_MAP
//#define SHOW_PRINT_INT_CONST
#define SHOW_PRINT_ARM_VALUE
#define SHOW_PRINT_SEGMENT_DELETE

// ========= Recompiling Checks =======================================

// You can strip the REG_HOST flags when emiting arm code
// or when registers are translated.
#undef DO_HOSTREG_RENUMBER_IN_TRANSLATIONS


// ========= Customize Aborts for debugging ===========================

#define ABORT_ARM_DECODE
//#define ABORT_ARM_ENCODE
#define ABORT_EXCEEDED_GLOBAL_OFFSET


#endif /* DEBUGDEFINES_H_ */
