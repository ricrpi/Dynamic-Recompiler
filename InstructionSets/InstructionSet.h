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

#ifndef INSTRUCTIONSET_H_
#define INSTRUCTIONSET_H_

#include <stdint.h>
#include <stddef.h>
#include "DebugDefines.h"

//-------------------------------------------------------------------


//-------------------------------------------------------------------

#define OPS_JUMP 			(0x01000)
#define OPS_LINK 			(0x02000)
#define OPS_BRANCH 			(0x04000)

#define OPS_LIKELY			(0x10000)
#define OPS_STR  			(0x20000)
#define OPS_LDR  			(0x40000)

#define STRIP(x) 			(x & 0xfff)

//Register IDs
#define REG_NOT_USED 		(0xffffU)
#define REG_FP		 		(0x0020U)


#define REG_WIDE	 		(0x0040U)			// 64-32 bit part of (register&0x3F).

#define REG_CO		 		(0x0080U)
#define REG_SPECIAL	 		(REG_CO) + 32U

#define REG_TEMP			(0x0100U)
#define REG_HOST		 	(0x0200U)

#define REG_INDEX			(REG_CO + 0)
#define REG_RANDOM			(REG_CO + 1)
#define REG_ENTRYLO0		(REG_CO + 2)
#define REG_ENTRYHI0		(REG_CO + 3)
#define REG_CONTEXT  		(REG_CO + 4)
#define REG_PAGEMASK		(REG_CO + 5)
#define REG_WIRED			(REG_CO + 6)
#define REG_BADVADDR 		(REG_CO + 8)
#define REG_COUNT    		(REG_CO + 9)
#define REG_ENTRYHI  		(REG_CO + 10)
#define REG_COMPARE  		(REG_CO + 11)
#define REG_STATUS   		(REG_CO + 12)
#define REG_CAUSE    		(REG_CO + 13)
#define REG_EPC		 		(REG_CO + 14)
#define REG_PRID	 		(REG_CO + 15)
#define REG_CONFIG		 	(REG_CO + 16)
#define REG_LLADDR	 		(REG_CO + 17)
#define REG_WATCHLO	 		(REG_CO + 18)
#define REG_WATCHHI	 		(REG_CO + 19)
#define REG_XCONTEXT 		(REG_CO + 20)
#define REG_PERR	 		(REG_CO + 26)
#define REG_CACHEERR 		(REG_CO + 27)
#define REG_TAGLO	 		(REG_CO + 28)
#define REG_TAGHI	 		(REG_CO + 29)
#define REG_ERROREPC		(REG_CO + 30)

#define Compare				(*((volatile uint32_t*)(MMAP_FP_BASE) + REG_COMPARE))
#define Cause 				(*((volatile uint32_t*)(MMAP_FP_BASE) + REG_CAUSE))
#define Status				(*((volatile uint32_t*)(MMAP_FP_BASE) + REG_STATUS))

// MIPS R4300 Special CPU Registers
#define REG_PC       		(REG_SPECIAL + 0)	// 64 bit
#define REG_FCR0	 		(REG_SPECIAL + 2)   // 32 bit
#define REG_FCR31    		(REG_SPECIAL + 3)   // 32 bit
#define REG_MULTHI  		(REG_SPECIAL + 4)	// 64 bit
#define REG_MULTLO   		(REG_SPECIAL + 6)	// 64 bit
#define REG_LLBIT    		(REG_SPECIAL + 8)	//  1 bit

//Temorary Registers
#define REG_TEMP_SCRATCH0	(REG_TEMP | 0x00)
#define REG_TEMP_SCRATCH1	(REG_TEMP | 0x01)
#define REG_TEMP_SCRATCH2	(REG_TEMP | 0x02)
#define REG_TEMP_SCRATCH3	(REG_TEMP | 0x03)

#define REG_TEMP_GEN1 		(REG_TEMP | 0x04)
#define REG_TEMP_GEN2 		(REG_TEMP | 0x05)
#define REG_TEMP_GEN3 		(REG_TEMP | 0x06)
#define REG_TEMP_GEN4 		(REG_TEMP | 0x07)
#define REG_TEMP_CALL2C		(REG_TEMP | 0x08)

#define REG_TEMP_DBG1		(REG_TEMP_GEN1)
#define REG_TEMP_DBG2		(REG_TEMP_GEN1)

//These are the HOST registers. Translation MUST not change registers equal to them
#define REG_HOST_FP			(REG_HOST | 0x0b)
#define REG_HOST_SP	 		(REG_HOST | 0x0d)
#define REG_HOST_LR	 		(REG_HOST | 0x0e)
#define REG_HOST_PC	 		(REG_HOST | 0x0f)

#define REG_HOST_R0	 		(REG_HOST | 0x00)
#define REG_HOST_R1	 		(REG_HOST | 0x01)
#define REG_HOST_R2			(REG_HOST | 0x02)
#define REG_HOST_R3			(REG_HOST | 0x03)
#define REG_HOST_R4			(REG_HOST | 0x04)
#define REG_HOST_R5			(REG_HOST | 0x05)
#define REG_HOST_R6			(REG_HOST | 0x06)
#define REG_HOST_R7			(REG_HOST | 0x07)
#define REG_HOST_R8			(REG_HOST | 0x08)
#define REG_HOST_R9			(REG_HOST | 0x09)
#define REG_HOST_R10		(REG_HOST | 0x0A)
#define REG_HOST_R11		(REG_HOST | 0x0B)
#define REG_HOST_R12		(REG_HOST | 0x0C)
#define REG_HOST_R13		(REG_HOST | 0x0D)
#define REG_HOST_R14		(REG_HOST | 0x0E)

// Configurable #defines if you want to change
// which registers hold the emulation frame data or emulation flags

#define REG_CC_FP			REG_HOST_R11		// The register used for Frame Pointer by compiler	DO NOT CHANGE IF USING GCC
#define REG_EMU_FP			REG_HOST_R10		// The 'FramePointer' used for MIPS register base
#define REG_EMU_FLAGS		REG_HOST_R5			// Register to hold flags passed between segments
#define REG_EMU_DEBUG1		REG_HOST_R3			// Register used for Debugging

// emulation flag bits
#define REG_EMU_FLAG_DS		(0x00)

//The following is for Register Mask operations
#define REG_HOST_STM_R0 		(0x0001)
#define REG_HOST_STM_R1 		(0x0002)
#define REG_HOST_STM_R2 		(0x0004)
#define REG_HOST_STM_R3 		(0x0008)
#define REG_HOST_STM_R1_3 		(REG_HOST_STM_R1|REG_HOST_STM_R2|REG_HOST_STM_R3)
#define REG_HOST_STM_R2_3 		(REG_HOST_STM_R2|REG_HOST_STM_R3)
#define REG_HOST_STM_FP 		(0x0800)
#define REG_HOST_STM_SP			(0x2000)
#define REG_HOST_STM_LR 		(0x4000)
#define REG_HOST_STM_PC 		(0x8000)
#define REG_HOST_STM_GENERAL 	(0x17FF)
#define REG_HOST_STM_EABI 		(0x000F)
#define REG_HOST_STM_EABI2      (0x5FF0)
#define REG_HOST_STM_ALL        (0x7FFF)

#define REG_T_SIZE	 (REG_HOST + 0x10)

//===============================================================

#if !REG_EMU_FP
#error "Need to have a register to point to Emulatated MIPS registers"
#endif

#ifndef REG_EMU_FLAGS
#define REG_EMU_FLAGS REG_NOT_USED
#endif

#ifndef REG_EMU_DEBUG1
#define REG_EMU_DEBUG1 REG_NOT_USED
#endif

//===============================================================

typedef enum _r_state_e {
	RS_REGISTER,
	RS_CONSTANT,
	RS_CONSTANT_I1,
	RS_CONSTANT_U1,
	RS_CONSTANT_I2,
	RS_CONSTANT_U2,
	RS_CONSTANT_I4,
	RS_CONSTANT_U4,
	RS_CONSTANT_I8,
	RS_CONSTANT_U8
} registerState_e;

typedef uint16_t regID_t;

typedef struct reg
{
	registerState_e state;
	regID_t regID;

	union
	{
		uint64_t u8;
		int64_t i8;
		uint32_t u4;
		int32_t i4;
		uint16_t u2;
		int16_t i2;
		uint8_t u1;
		int8_t i1;
	};
} reg_t;



//-----------------------------------------------------------------------------

typedef enum _Instruction_e {

	// Dynamic Recompiler / Generic instructions
	DR_UNKNOWN,
	DR_LITERAL,
	DR_INVALID,
	DR_NO_OP,
	DR_INT_BRANCH,	// Intermediae Branch to the Instruction (->branchToThisInstruction)
	DR_INT_BRANCH_LINK,	// Intermediae Branch to the Instruction (->branchToThisInstruction)

	// MIPS

	MIPS_SLL,	// Rd1 (rd) = R1 (rt) << imm5 				The contents of the low-order 32-bit word of GPR rt are shifted left, inserting zeroes into the emptied bits; the word result is placed in GPR rd. The bit shift count is specified by sa. If rd is a 64-bit register, the result word is sign-extended.
	MIPS_SRL,	// Rd1 (rd) = R1 (rt) >> imm5				The contents of the low-order 32-bit word of GPR rt are shifted right, inserting zeros into the emptied bits; the word result is placed in GPR rd. The bit shift count is specified by sa. If rd is a 64-bit register, the result word is sign-extended.
	MIPS_SRA,	// Rd1 (rd) = R1 (rt) >> imm5 (arithmetic)	The contents of the low-order 32-bit word of GPR rt are shifted right, duplicating the sign-bit (bit 31) in the emptied bits; the word result is placed in GPR rd. The bit shift count is specified by sa. If rd is a 64-bit register, the result word is sign-extended.
	MIPS_SLLV,	// Rd1 (rd) = R1 (rt) << R2 (rs)			The contents of the low-order 32-bit word of GPR rt are shifted left, inserting zeroes into the emptied bits; the result word is placed in GPR rd. The bit shift count is specified by the low-order five bits of GPR rs. If rd is a 64-bit register, the result word is sign-extended.
	MIPS_SRLV,	// Rd1 (rd) = R1 (rt) >> R2 (rs)			The contents of the low-order 32-bit word of GPR rt are shifted right, inserting zeros into the emptied bits; the word result is placed in GPR rd. The bit shift count is specified by the low-order five bits of GPR rs. If rd is a 64-bit register, the result word is sign-extended.
	MIPS_SRAV,	// Rd1 (rd) = R1 (rt) >> R2 (rs) (arithmetic) The contents of the low-order 32-bit word of GPR rt are shifted right, duplicating the sign-bit (bit 31) in the emptied bits; the word result is placed in GPR rd. The bit shift count is specified by the low-order five bits of GPR rs. If rd is a 64-bit register, the result word is sign-extended.
	MIPS_SYSCALL,
	MIPS_BREAK,	// A breakpoint exception occurs, immediately and unconditionally transferring control to the exception handler. The code field is available for use as software parameters, but is retrieved by the exception handler only by loading the contents of the memory word containing the instruction.
	MIPS_SYNC,
	MIPS_MFHI,	// Rd1 (rd) = HI 					To copy the special purpose HI register to a GPR.
	MIPS_MTHI,	// HI = R1 (rs)						To copy a GPR to the special purpose HI register.
	MIPS_MFLO,	// Rd1 (rd) = LO 					To copy the special purpose LO register to a GPR.
	MIPS_MTLO,	// LO = R1 (rs)						To copy a GPR to the special purpose LO register.
	MIPS_DSLLV,	// Rd1 (rd) = R1 (rt) << R2 (rs) 	To left shift a doubleword by a variable number of bits. The bit shift count in the range 0 to 63 is specified by the low-order six bits in GPR rs
	MIPS_DSRLV,	// Rd1 (rd) = R1 (rt) << R2 (rs) 	To right shift a doubleword by a variable number of bits. The bit shift count in the range 0 to 63 is specified by the low-order six bits in GPR rs
	MIPS_DSRAV,	// Rd1 (rd) = R1 (rt) << R2 (rs) (arithmetic). To right shift a doubleword by a variable number of bits. The bit shift count in the range 0 to 63 is specified by the low-order six bits in GPR rs
	MIPS_MULT,	// LO, HI = R1 (rs) * R2 (rt)		The 32-bit word value in GPR rt is multiplied by the 32-bit value in GPR rs, treating both operands as signed values, to produce a 64-bit result. The low-order 32-bit word of the result is placed into special register LO, and the high-order 32-bit word is placed into special register HI.
	MIPS_MULTU,	// LO, HI = R1 (rs) * R2 (rt)		The 32-bit word value in GPR rt is multiplied by the 32-bit value in GPR rs, treating both operands as unsigned values, to produce a 64-bit result. The low-order 32-bit word of the result is placed into special register LO, and the high-order 32-bit word is placed into special register HI.
	MIPS_DIV,	// (LO, HI) = R1 (rs) / R2 (rt) 	To divide 32-bit signed integers. The 32-bit word value in GPR rs is divided by the 32-bit value in GPR rt, treating both operands as signed values. The 32-bit quotient is placed into special register LO and the 32-bit remainder is placed into special register HI.
	MIPS_DIVU,	// (LO, HI) = R1 (rs) / R2 (rt) 	To divide 32-bit unsigned integers. The 32-bit word value in GPR rs is divided by the 32-bit value in GPR rt, treating both operands as unsigned values. The 32-bit quotient is placed into special register LO and the 32-bit remainder is placed into special register HI.
	MIPS_DMULT,	// (LO, HI) = R1 (rs) * R2 (rt) 	To multiply 64-bit signed integers. The 64-bit doubleword value in GPR rt is multiplied by the 64-bit value in GPR rs, treating both operands as signed values, to produce a 128-bit result. The low-order 64-bit doubleword of the result is placed into special register LO, and the high-order 64-bit doubleword is placed into special register HI.
	MIPS_DMULTU,	// (LO, HI) = R1 (rs) * R2 (rt) 	To multiply 64-bit unsigned integers. The 64-bit doubleword value in GPR rt is multiplied by the 64-bit value in GPR rs, treating both operands as unsigned values, to produce a 128-bit result. The low-order 64-bit doubleword of the result is placed into special register LO, and the high-order 64-bit doubleword is placed into special register HI.
	MIPS_DDIV,	// (LO, HI) = R1 (rs) / R2 (rt) 	To divide 64-bit signed integers. The 64-bit doubleword in GPR rs is divided by the 64-bit doubleword in GPR rt, treating both operands as signed values. The 64-bit quotient is placed into special register LO and the 64-bit remainder is placed into special register HI.
	MIPS_DDIVU,	// (LO, HI) = R1 (rs) / R2 (rt) 	To divide 64-bit unsigned integers. The 64-bit doubleword in GPR rs is divided by the 64-bit doubleword in GPR rt, treating both operands as unsigned values. The 64-bit quotient is placed into special register LO and the 64-bit remainder is placed into special register HI.
	MIPS_ADD,	// Rd1 (rd) = R1 (rs) + R2 (rt) 	To add 32-bit integers. If overflow occurs, then trap.
	MIPS_ADDU,	// Rd1 (rd) = R1 (rs) + R2 (rt) 	To add 32-bit integers.
	MIPS_SUB,	// Rd1 (rd) = R1 (rs) - R2 (rt) 	The 32-bit word value in GPR rt is subtracted from the 32-bit value in GPR rs to produce a 32-bit result. If the subtraction results in 32-bit 2’s complement arithmetic overflow then the destination register is not modified and an Integer Overflow exception occurs. If it does not overflow, the 32-bit result is placed into GPR rd.
	MIPS_SUBU,	// Rd1 (rd) = R1 (rs) - R2 (rt) 	The 32-bit word value in GPR rt is subtracted from the 32-bit value in GPR rs and the 32-bit arithmetic result is placed into GPR rd. No integer overflow exception occurs under any circumstances.
	MIPS_AND,	// Rd1 (rd) = R1 (rs) & R2 (rt) 	To do a bitwise logical AND.
	MIPS_OR,		// Rd1 (rd) = R1 (rs) | R2 (rt)		The contents of GPR rs are combined with the contents of GPR rt in a bitwise logical OR operation. The result is placed into GPR rd.
	MIPS_XOR,	// Rd1 (rd) = R1 (rs) ^ R2 (rt)		Combine the contents of GPR rs and GPR rt in a bitwise logical exclusive OR operation and place the result into GPR rd.
	MIPS_NOR,	// Rd1 (rd) = R1 (rs) | ~R2 (rt)	The contents of GPR rs are combined with the contents of GPR rt in a bitwise logical NOR operation. The result is placed into GPR rd.
	MIPS_SLT,	// Rd1 (rd) = R1 (rs) < R2 (rt)		Compare the contents of GPR rs and GPR rt as signed integers and record the Boolean result of the comparison in GPR rd. If GPR rs is less than GPR rt the result is 1 (true), otherwise 0 (false).
    MIPS_SLTU,	// Rd1 (rd) = R1 (rs) < R2 (rt)		Compare the contents of GPR rs and GPR rt as unsigned integers and record the Boolean result of the comparison in GPR rd. If GPR rs is less than GPR rt the result is 1 (true), otherwise 0 (false).
    MIPS_DADD,	// Rd1 (rd) = R1 (rs) + R2 (rt) 	To add 64-bit integers. If overflow occurs, then trap.
    MIPS_DADDU,	// Rd1 (rd) = R1 (rs) + R2 (rt) 	To add 64-bit integers.
    MIPS_DSUB,	// Rd1 (rd) = R1 (rs) - R2 (rt) 	To add 64-bit integers. If overflow occurs, then trap.
    MIPS_DSUBU,	// Rd1 (rd) = R1 (rs) - R2 (rt) 	To add 64-bit integers.
    MIPS_TGE,	// TRAP when R1 (rs) >= R2 (rt)		Compare the contents of GPR rs and GPR rt as signed integers; if GPR rs is greater than or equal to GPR rt then take a Trap exception.
    MIPS_TGEU,	// TRAP when R1 (rs) >= R2 (rt)		Compare the contents of GPR rs and GPR rt as unsigned integers; if GPR rs is greater than or equal to GPR rt then take a Trap exception.
    MIPS_TLT,	// TRAP when R1 (rs) < R2 (rt)		Compare the contents of GPR rs and GPR rt as signed integers; if GPR rs is less than GPR rt then take a Trap exception.
    MIPS_TLTU,	// TRAP when R1 (rs) < R2 (rt)		Compare the contents of GPR rs and GPR rt as unsigned integers; if GPR rs is less than GPR rt then take a Trap exception.
    MIPS_TEQ,	// TRAP when R1 (rs) == R2 (rt)		Compare the contents of GPR rs and GPR rt as signed integers; if GPR rs is equal to GPR rt then take a Trap exception. The contents of the code field are ignored by hardware and may be used to encode information for system software. To retrieve the information, system software must load the instruction word from memory.
    MIPS_TNE,	// TRAP when R1 (rs) != R2 (rt)		Compare the contents of GPR rs and GPR rt as signed integers; if GPR rs is not equal to GPR rt then take a Trap exception.
    MIPS_DSLL,	// Rd1 (rd) = R1 (rt) << imm5 		To left shift a doubleword by a fixed amount  0 to 31 bits.
    MIPS_DSRL,	// Rd1 (rd) = R1 (rt) >> imm5 		To right shift a doubleword by a fixed amount  0 to 31 bits.
    MIPS_DSRA,	// Rd1 (rd) = R1 (rt) >> imm5 (arithmetic). To right shift a doubleword by a fixed amount  0 to 31 bits.
    MIPS_DSLL32,	// Rd1 (rd) = R1 (rt) << (32 + imm5) To left shift a doubleword by a fixed amount  32 to 63 bits.
    MIPS_DSRL32, // Rd1 (rd) = R1 (rt) >> (32 + imm5) To right shift a doubleword by a fixed amount  32 to 63 bits.
    MIPS_DSRA32, // Rd1 (rd) = R1 (rt) >> (32 + imm5) (arithmetic). To right shift a doubleword by a fixed amount  32 to 63 bits.
    MIPS_TGEI,	// TRAP when R1 (rs) >= imm16		Compare the contents of GPR rs and the 16-bit signed immediate as signed integers; if GPR rs is greater than or equal to immediate then take a Trap exception.
    MIPS_TGEIU,	// TRAP when R1 (rs) >= imm16		Compare the contents of GPR rs and the 16-bit sign-extended immediate as unsigned integers; if GPR rs is greater than or equal to immediate then take a Trap exception. Because the 16-bit immediate is sign-extended before comparison, the instruction is able to represent the smallest or largest unsigned numbers. The representable values are at the minimum [0, 32767] or maximum [max_unsigned-32767, max_unsigned] end of the unsigned range.
    MIPS_TLTI,	// TRAP when R1 (rs) < imm16		Compare the contents of GPR rs and the 16-bit signed immediate as signed integers; if GPR rs is less than immediate then take a Trap exception.
    MIPS_TLTIU,	// TRAP when R1 (rs) < imm16		Compare the contents of GPR rs and the 16-bit sign-extended immediate as unsigned integers; if GPR rs is less than immediate then take a Trap exception. Because the 16-bit immediate is sign-extended before comparison, the instruction is able to represent the smallest or largest unsigned numbers. The representable values are at the minimum [0, 32767] or maximum [max_unsigned-32767, max_unsigned] end of the unsigned range.
    MIPS_TEQI,	// TRAP when R1 (rs) == imm16		Compare the contents of GPR rs and the 16-bit signed immediate as signed integers; if GPR rs is equal to immediate then take a Trap exception.
    MIPS_TNEI,	// TRAP when R1 (rs) != imm16		Compare the contents of GPR rs and the 16-bit signed immediate as signed integers; if GPR rs is not equal to immediate then take a Trap exception.
    MIPS_ADDI,	// Rd1 (rt) = R1 (rs) + imm16 		To add a constant to a 32-bit integer. If overflow occurs, then trap.
    MIPS_ADDIU,	// Rd1 (rt) = R1 (rs) + imm16 		To add a constant to a 32-bit integer.
    MIPS_SLTI,	// Rd1 (rt) = R1 (rs) + imm16		Compare the contents of GPR rs and the 16-bit signed immediate as signed integers and record the Boolean result of the comparison in GPR rt. If GPR rs is less than immediate the result is 1 (true), otherwise 0 (false).
    MIPS_SLTIU,	// Rd1 (rt) = R1 (rs) + imm16		Compare the contents of GPR rs and the sign-extended 16-bit immediate as unsigned integers and record the Boolean result of the comparison in GPR rt. If GPR rs is less than immediate the result is 1 (true), otherwise 0 (false). Because the 16-bit immediate is sign-extended before comparison, the instruction is able to represent the smallest or largest unsigned numbers. The representable values are at the minimum [0, 32767] or maximum [max_unsigned-32767, max_unsigned] end of the unsigned range.
    MIPS_ANDI,	// Rd1 (rt) = R1 (rs) & imm16 		To do a bitwise logical AND with a constant.
    MIPS_ORI,	// Rd1 (rt) = R1 (rs) | imm16 		To do a bitwise logical OR with a constant.
    MIPS_XORI,	// Rd1 (rt) = R1 (rs) ^ imm16 		Combine the contents of GPR rs and the 16-bit zero-extended immediate in a bitwise logical exclusive OR operation and place the result into GPR rt.
    MIPS_LUI,	// Rd1 (rt) = imm16 << 16 			The 16-bit immediate is shifted left 16 bits and concatenated with 16 bits of low-order zeros. The 32-bit result is sign-extended and placed into GPR rt.
    MIPS_MFC0,	// Rd1 = R1
    MIPS_MTC0,	// Rd1 = R1
    MIPS_TLBR,
    MIPS_TLBWI,
    MIPS_TLBWR,
    MIPS_TLBP,
    MIPS_ERET,		// Return from Exception
    MIPS_MFC1,
    MIPS_DMFC1,
    MIPS_CFC1,		// Rd1 (rt) = R1 (fs)				Copy FPU fs into rt
    MIPS_MTC1,
    MIPS_DMTC1,
    MIPS_CTC1,		// Rd1 (fs) = R1 (rt)				Copy from GPR rt to a FPU control register fs
    MIPS_BC1F,		// Branch on FP false
    MIPS_BC1T,
    MIPS_BC1FL,
    MIPS_BC1TL,
    MIPS_ADD_S,		// Rd1 (fd) = R1 (fs) + R2 (ft)
    MIPS_SUB_S,		// Rd1 (fd) = R1 (fs) - R2 (ft)
    MIPS_MUL_S,
    MIPS_DIV_S,
    MIPS_SQRT_S,
    MIPS_ABS_S,
    MIPS_MOV_S,
    MIPS_NEG_S,
    MIPS_ROUND_L_S,
    MIPS_TRUNC_L_S,
    MIPS_CEIL_L_S,
    MIPS_FLOOR_L_S,
    MIPS_ROUND_W_S,
    MIPS_TRUNC_W_S,
    MIPS_CEIL_W_S,		// Rd1 (fd) = convert( R1 (fs) )		The value in FPR fs in format fmt, is converted to a value in 32-bit word fixed-point format rounding toward +∞ (rounding mode 2). The result is placed in FPR fd. When the source value is Infinity, NaN, or rounds to an integer outside the range -231 to 231-1, the result cannot be represented correctly and an IEEE Invalid Operation condition exists. The result depends on the FP exception model currently active.
    MIPS_FLOOR_W_S,
    MIPS_CVT_D_S,		// Rd1 (fd) = convert( R1 (fs) )		The value in FPR fs in format fmt is converted to a value in double floating-point format rounded according to the current rounding mode in FCSR. The result is placed in FPR fd.
    MIPS_CVT_W_S,
    MIPS_CVT_L_S,
    MIPS_C_F_S,		// FP Compare	False
    MIPS_C_UN_S,		// FP Compare	Unordered
    MIPS_C_EQ_S,		// FP Compare	Equal
    MIPS_C_UEQ_S,	// FP Compare	Unordered or Equal
    MIPS_C_OLT_S,	// FP Compare	Ordered or Less Than
    MIPS_C_ULT_S,	// FP Compare	Unordered or Less Than
    MIPS_C_OLE_S,	// FP Compare	Ordered or Less Than or Equal
    MIPS_C_ULE_S,	// FP Compare	Unordered or Less Than or Equal
    MIPS_C_SF_S,		// FP Compare	Signaling False
    MIPS_C_NGLE_S,	// FP Compare	Not Greater than or Less Than or Equal
    MIPS_C_SEQ_S,	// FP Compare	Signaling Equal
    MIPS_C_NGL_S,	// FP Compare	Not Greater Than or Less Than
    MIPS_C_LT_S,		// FP Compare	Less Than
    MIPS_C_NGE_S,	// FP Compare	Not Greater Than or Equal
    MIPS_C_LE_S,		// FP Compare	Less Than or Equal
    MIPS_C_NGT_S,	// FP Compare	Not Greater Than
	MIPS_ADD_D,		// Rd1 (fd) = R1 (fs) + R2 (ft)
	MIPS_SUB_D,		// Rd1 (fd) = R1 (fs) - R2 (ft)
	MIPS_MUL_D,
	MIPS_DIV_D,
	MIPS_SQRT_D,
	MIPS_ABS_D,
	MIPS_MOV_D,
	MIPS_NEG_D,
	MIPS_ROUND_L_D,
	MIPS_TRUNC_L_D,
	MIPS_CEIL_L_D,
	MIPS_FLOOR_L_D,
	MIPS_ROUND_W_D,
	MIPS_TRUNC_W_D,
	MIPS_CEIL_W_D,
	MIPS_FLOOR_W_D,
	MIPS_CVT_S_D,
	MIPS_CVT_W_D,
	MIPS_CVT_L_D,
	MIPS_C_F_D,
	MIPS_C_UN_D,
	MIPS_C_EQ_D,
	MIPS_C_UEQ_D,
	MIPS_C_OLT_D,
	MIPS_C_ULT_D,
	MIPS_C_OLE_D,
	MIPS_C_ULE_D,
	MIPS_C_SF_D,
	MIPS_C_NGLE_D,
	MIPS_C_SEQ_D,
	MIPS_C_NGL_D,
	MIPS_C_LT_D,
	MIPS_C_NGE_D,
	MIPS_C_LE_D,
	MIPS_C_NGT_D,
	MIPS_CVT_S_W,
	MIPS_CVT_D_W,	// Rd1 (fd) = convert( R1 (fs) )		The value in FPR fs in format fmt is converted to a value in double floating-point format rounded according to the current rounding mode in FCSR. The result is placed in FPR fd.
	MIPS_CVT_S_L,
	MIPS_CVT_D_L,
	MIPS_DADDI,	// Rd1 (rt) = R1 (rs) + imm16 		To add a constant to a 64-bit integer. If overflow occurs, then trap.
	MIPS_DADDIU,	// Rd1 (rt) = R1 (rs) + imm16 		To add a constant to a 64-bit integer.
	MIPS_CACHE,
	MIPS_LL,		// Rd1 (rt) = memory[ R1 (base) + imm16]	The LL and SC instructions provide primitives to implement atomic Read-Modify-Write (RMW) operations for cached memory locations. The 16-bit signed offset is added to the contents of GPR base to form an effective address.
	MIPS_LWC1,	// Rd1 (rt) = memory[ R1 (base) + imm16]	The contents of the 32-bit word at the memory location specified by the aligned effective address are fetched and made available to coprocessor unit zz. The 16-bit signed offset is added to the contents of GPR base to form the effective address.
	MIPS_LLD,	// Rd1 (rt) = memory[ R1 (base) + imm16]	The LL and SC instructions provide primitives to implement atomic Read-Modify-Write (RMW) operations for cached memory locations. The 16-bit signed offset is added to the contents of GPR base to form an effective address.
	MIPS_LDC1,	// Rd1 (rt) = memory[ R1 (base) + imm16]	The contents of the 64-bit doubleword at the memory location specified by the aligned effective address are fetched and made available to coprocessor unit 1. The 16-bit signed offset is added to the contents of GPR base to form the effective address. The manner in which each coprocessor uses the data is defined by the individual coprocessor specifications. The usual operation would place the data into coprocessor general register rt.
	MIPS_LD,		// Rd1 (rt) = memory[ R1 (base) + imm16]	The contents of the 64-bit doubleword at the memory location specified by the aligned effective address are fetched and placed in GPR rt. The 16-bit signed offset is added to the contents of GPR base to form the effective address. The effective address must be naturally aligned. If any of the three least-significant bits of the address are non-zero, an Address Error exception occurs.
	MIPS_SC,		// memory[ R1 (base) + imm16] = R2 (rt)		The SC completes the RMW sequence begun by the preceding LL instruction executed on the processor. If it would complete the RMW sequence atomically, then the least-significant 32-bit word of GPR rt is stored into memory at the location specified by the aligned effective address and a one, indicating success, is written into GPR rt. Otherwise, memory is not modified and a zero, indicating failure, is written into GPR rt.
	MIPS_SWC1,	// memory[ R1 (base) + imm16] = R2 (rt)		Coprocessor unit zz supplies a 32-bit word which is stored at the memory location specified by the aligned effective address. The 16-bit signed offset is added to the contents of GPR base to form the effective address.
	MIPS_SCD,	// memory[ R1 (base) + imm16] = R2 (rt)		The SCD completes the RMW sequence begun by the preceding LLD instruction executed on the processor. If it would complete the RMW sequence atomically, then the 64-bit doubleword of GPR rt is stored into memory at the location specified by the aligned effective address and a one, indicating success, is written into GPR rt. Otherwise, memory is not modified and a zero, indicating failure, is written into GPR rt.
	MIPS_SDC1,	// memory[ R1 (base) + imm16] = R2 (rt)		Coprocessor unit zz supplies a 64-bit doubleword which is stored at the memory location specified by the aligned effective address. The 16-bit signed offset is added to the contents of GPR base to form the effective address.
	MIPS_SD,		//

	//---------------------------
	MIPS_J 		// PC = PC(31:27) | (PC + imm26)(26:0) 		This is a PC-region branch (not PC-relative); the effective target address is in the “current” 256 MB aligned region. The low 28 bits of the target address is the instr_index field shifted left 2 bits. The remaining upper bits are the corresponding bits of the address of the instruction in the delay slot (not the branch itself). Jump to the effective target address. Execute the instruction following the jump, in the branch delay slot, before jumping.
		= OPS_JUMP + STRIP(MIPS_SD) + 1,
	MIPS_JR,		// PC = R1 (rs) 							Jump to the effective target address in GPR rs. Execute the instruction following the jump, in the branch delay slot, before jumping.
	MIPS_JAL		// Place the return address link in GPR 31. The return link is the address of the second instruction following the branch, where execution would continue after a procedure call. This is a PC-region branch (not PC-relative); the effective target address is in the “current” 256 MB aligned region. The low 28 bits of the target address is the instr_index field shifted left 2 bits. The remaining upper bits are the corresponding bits of the address of the instruction in the delay slot (not the branch itself). Jump to the effective target address. Execute the instruction following the jump, in the branch delay slot, before jumping.
		= OPS_JUMP + OPS_LINK + STRIP(MIPS_JR) + 1,
	MIPS_JALR,	// Rd = PC + 8, PC = R1 (rs)			Place the return address link in GPR rd. The return link is the address of the second instruction following the branch, where execution would continue after a procedure call. Jump to the effective target address in GPR rs. Execute the instruction following the jump, in the branch delay slot, before jumping.

	//---------------------------

	MIPS_BLTZ 	// Branch if R1 (rs) < 0, 				offset is imm16 << 2 added to next instruction address. If the contents of GPR rs are less than zero (sign bit is 1), branch to the effective target address after the instruction in the delay slot is executed.
		= OPS_BRANCH + STRIP(MIPS_JALR) + 1,
	MIPS_BGEZ,	// Branch if R1 (rs) >= 0, 				offset is imm16 << 2 added to next instruction address. If the contents of GPR rs are greater than or equal to zero (sign bit is 0), branch to the effective target address after the instruction in the delay slot is executed.
	MIPS_BEQ,	// Branch if R1 (rs) == R2 (rt), 		offset is imm16 << 2 added to next instruction address. If the contents of GPR rs and GPR rt are equal, branch to the effective target address after the instruction in the delay slot is executed.
	MIPS_BNE,	// Branch if R1 (rs) != R2 (rt), 		offset is imm16 << 2 added to next instruction address. If the contents of GPR rs and GPR rt are not equal, branch to the effective target address after the instruction in the delay slot is executed.
	MIPS_BLEZ,	// Branch if R1 (rs) <= 0, 				offset is imm16 << 2 added to next instruction address. If the contents of GPR rs are less than or equal to zero (sign bit is 1 or value is zero), branch to the effective target address after the instruction in the delay slot is executed.
	MIPS_BGTZ,	// Branch if R1 (rs) > 0,				offset is imm16 << 2 added to next instruction address. If the contents of GPR rs are greater than zero (sign bit is 0 but value not zero), branch to the effective target address after the instruction in the delay slot is executed.

	MIPS_BLTZL	// Branch Likely if R1 (rs) < 0, 		offset is imm16 << 2 added to next instruction address. If the contents of GPR rs are less than zero (sign bit is 1), branch to the effective target address after the instruction in the delay slot is executed. If the branch is not taken, the instruction in the delay slot is not executed.
		= OPS_BRANCH + OPS_LIKELY + STRIP(MIPS_BGTZ) + 1,
	MIPS_BGEZL,	// Branch Likely if R1 (rs) >= 0, 		offset is imm16 << 2 added to next instruction address. If the contents of GPR rs are greater than or equal to zero (sign bit is 0), branch to the effective target address after the instruction in the delay slot is executed. If the branch is not taken, the instruction in the delay slot is not executed.
	MIPS_BEQL,	// Branch Likely if R1 (rs) == R2 (rt),	offset is imm16 << 2 added to next instruction address. If the contents of GPR rs and GPR rt are equal, branch to the target address after the instruction in the delay slot is executed. If the branch is not taken, the instruction in the delay slot is not executed.
	MIPS_BNEL,	// Branch Likely if R1 (rs) != R2 (rt),	offset is imm16 << 2 added to next instruction address. If the contents of GPR rs and GPR rt are not equal, branch to the effective target address after the instruction in the delay slot is executed. If the branch is not taken, the instruction in the delay slot is not executed.
	MIPS_BLEZL,	// Branch Likely if R1 (rs) <= 0, 		offset is imm16 << 2 added to next instruction address. If the contents of GPR rs are less than or equal to zero (sign bit is 1 or value is zero), branch to the effective target address after the instruction in the delay slot is executed.	If the branch is not taken, the instruction in the delay slot is not executed.
	MIPS_BGTZL,	// Branch Likely if R1 (rs) >= 0, 		offset is imm16 << 2 added to next instruction address. If the contents of GPR rs are greater than zero (sign bit is 0 but value not zero), branch to the effective target address after the instruction in the delay slot is executed. If the branch is not taken, the instruction in the delay slot is not executed.

	//--------------------------

	MIPS_BLTZAL	// Branch Link if R1 (rs) < 0, 			offset is imm16 << 2 added to next instruction address. Place the return address link in GPR 31. The return link is the address of the second instruction following the branch (not the branch itself), where execution would continue after a procedure call. If the contents of GPR rs are less than zero (sign bit is 1), branch to the effective target address after the instruction in the delay slot is executed. GPR 31 must not be used for the source register rs
		= OPS_BRANCH + OPS_LINK + STRIP(MIPS_BGTZL) + 1,
	MIPS_BGEZAL,	// Branch Link if R1 (rs) >= 0, 		offset is imm16 << 2 added to next instruction address. Place the return address link in GPR 31. The return link is the address of the second instruction following the branch, where execution would continue after a procedure call. GPR 31 must not be used for the source register rs
	MIPS_BLTZALL // Branch Link Likely if R1 (rs) < 0, 	offset is imm16 << 2 added to next instruction address. Place the return address link in GPR 31. The return link is the address of the second instruction following the branch (not the branch itself), where execution would continue after a procedure call. If the contents of GPR rs are less than zero (sign bit is 1), branch to the effective target address after the instruction in the delay slot is executed. GPR 31 must not be used for the source register rs
		= OPS_BRANCH + OPS_LINK + OPS_LIKELY + STRIP(MIPS_BGEZAL)+1,
	MIPS_BGEZALL,// Branch Link Likely if R1 (rs) >= 0, 	offset is imm16 << 2 added to next instruction address. Place the return address link in GPR 31. If the contents of GPR rs are greater than or equal to zero (sign bit is 0), branch to the effective target address after the instruction in the delay slot is executed. If the branch	is not taken, the instruction in the delay slot is not executed. The return link is the address of the second instruction following the branch, where execution would continue after a procedure call. GPR 31 must not be used for the source register rs

	//---------------------------

	MIPS_SB 		// memory[ R1 (base) + imm16] = R2 (rt)				The least-significant 8-bit byte of GPR rt is stored in memory at the location specified by the effective address. The 16-bit signed offset is added to the contents of GPR base to form the effective address.
		= OPS_STR + STRIP(MIPS_BGEZALL) + 1,
	MIPS_SH,		// memory[ R1 (base) + imm16] = R2 (rt)					The least-significant 16-bit halfword of register rt is stored in memory at the location specified by the aligned effective address. The 16-bit signed offset is added to the contents of GPR base to form the effective address.
	MIPS_SWL,	// memory[ R1 (base) + imm16] = partial-upper R2 (rt)				The 16-bit signed offset is added to the contents of GPR base to form an effective address (EffAddr). EffAddr is the address of the most-significant of four consecutive bytes forming a word in memory (W) starting at an arbitrary byte boundary. A part of W, the most-significant one to four bytes, is in the aligned word containing EffAddr. The same number of the most-significant (left) bytes from the word in GPR rt are stored into these bytes of W.
	MIPS_SW,		// memory[ R1 (base) + imm16] = R2 (rt)					The least-significant 32-bit word of register rt is stored in memory at the location specified by the aligned effective address. The 16-bit signed offset is added to the contents of GPR base to form the effective address.
	MIPS_SDL,	// memory[ R1 (base) + imm16] = partial-upper R2 (rt)	The 16-bit signed offset is added to the contents of GPR base to form an effective address (EffAddr). EffAddr is the address of the most-significant of eight consecutive bytes forming a doubleword in memory (DW) starting at an arbitrary byte boundary. A part of DW, the most-significant one to eight bytes, is in the aligned doubleword containing EffAddr. The same number of most-significant (left) bytes of GPR rt are stored into these bytes of DW.
	MIPS_SDR,	// memory[ R1 (base) + imm16] = partial-lower R2 (rt)	The 16-bit signed offset is added to the contents of GPR base to form an effective address (EffAddr). EffAddr is the address of the least-significant of eight consecutive bytes forming a doubleword in memory (DW) starting at an arbitrary byte boundary. A part of DW, the least-significant one to eight bytes, is in the aligned doubleword containing EffAddr. The same number of least-significant (right) bytes of GPR rt are stored into these bytes of DW.
	MIPS_SWR,	// memory[ R1 (base) + imm16] = partial-lower R2 (rt)	The 16-bit signed offset is added to the contents of GPR base to form an effective address (EffAddr). EffAddr is the address of the least-significant of four consecutive bytes forming a word in memory (W) starting at an arbitrary byte boundary. A part of W, the least-significant one to four bytes, is in the aligned word containing EffAddr. The same number of the least-significant (right) bytes from the word in GPR rt are stored into these bytes of W.

	//---------------------------

	MIPS_LDL 	// Rd1 (rt) = Rd1 | memory[ (R1 (base) + imm16)(bytes ((base+imm)&7) to 8)]		To load the most-significant part of a doubleword from an unaligned memory address.
		= OPS_LDR + STRIP(MIPS_SWR) + 1,
	MIPS_LDR,	// Rd1 (rt) = Rd1 | memory[ (R1 (base) + imm16)(bytes (8 to ((base+imm)&7 + 8)))]
	MIPS_LB,		// Rd1 (rt) = memory[ R1 (base) + imm16]			The contents of the 8-bit byte at the memory location specified by the effective address are fetched, sign-extended, and placed in GPR rt. The 16-bit signed offset is added to the contents of GPR base to form the effective address.
	MIPS_LH,		// Rd1 (rt) = memory[ R1 (base) + imm16]			The contents of the 16-bit halfword at the memory location specified by the aligned effective address are fetched, sign-extended, and placed in GPR rt. The 16-bit signed offset is added to the contents of GPR base to form the effective address. The effective address must be naturally aligned. If the least-significant bit of the address is non-zero, an Address Error exception occurs.
	MIPS_LWL,	// Rd1 (rt) = Rd1 | memory[ (R1 (base) + imm16)(bytes ((base+imm)&3) to 4)]
	MIPS_LW,		// Rd1 (rt) = memory[ R1 (base) + imm16]			The contents of the 32-bit word at the memory location specified by the aligned effective address are fetched, sign-extended to the GPR register length if necessary, and placed in GPR rt. The 16-bit signed offset is added to the contents of GPR base to form the effective address.
	MIPS_LBU, 	// Rd1 (rt) = memory[ R1 (base) + imm16]			The contents of the 8-bit byte at the memory location specified by the effective address are fetched, sign-extended, and placed in GPR rt. The 16-bit signed offset is added to the contents of GPR base to form the effective address.
	MIPS_LHU,	// Rd1 (rt) = memory[ R1 (base) + imm16]			The contents of the 16-bit halfword at the memory location specified by the aligned effective address are fetched, zero-extended, and placed in GPR rt. The 16-bit signed offset is added to the contents of GPR base to form the effective address. The effective address must be naturally aligned. If the least-significant bit of the address is non-zero, an Address Error exception occurs.
	MIPS_LWR,	// Rd1 (rt) = Rd1 | memory[ (R1 (base) + imm16)(bytes ( 4 to (base+imm +4)&3))]
	MIPS_LWU,	// Rd1 (rt) = memory[ R1 (base) + imm16]			The contents of the 32-bit word at the memory location specified by the aligned effective address are fetched, zero-extended, and placed in GPR rt. The 16-bit signed offset is added to the contents of GPR base to form the effective address.

	//---------------------------
	// ARM
	//---------------------------

	ARM_BFC= STRIP(MIPS_LWU) + 1,
	ARM_BFI,
	ARM_CLZ,
	ARM_PKHBT,
	ARM_PKHTB,
	ARM_RBIT,
	ARM_REV,
	ARM_REV16,
	ARM_REVSH,
	ARM_AND,		// R1 (Rd) = R2 (Rn) & Op2
	ARM_B,			// pc = pc+<offset>
	ARM_BL,			// lr = pc-4, pc = pc+<offset>
	ARM_BLX,		// lr = pc-4, pc = R1 (Rm)
	ARM_BX,			// pc = R1 (Rm)
	ARM_EOR,
	ARM_SUB,		// Rd1 (Rd) = R1 - Op2
	ARM_RSB,		// Rd1 (Rd) = Op2 - R1
	ARM_ADD,		// Rd1 (Rd) = R1 (Rn) + Op2
	ARM_ADC,		// Rd1 (Rd) = R1 (Rn) + Op2 + Carry
	ARM_ASR,		// Rd1 (Rd) = R1 (Rn) >> #<imm>
	ARM_SBC,		// Rd1 (Rd) = R1 (Rn) + Op2 + Carry
	ARM_RSC,
	ARM_TST,		// R1 (Rn) & Op2
	ARM_TEQ,		// R1 (Rn) ^ Op2
	ARM_CMP,		// R1 (Rn) - Op2
	ARM_CMN,		// R1 (Rn) + Op2
	ARM_ORR,		// Rd1 (Rd) = R1 (Rn) | Op2
	ARM_MOV,		// Rd1 (Rd) = Op2
	ARM_BIC,		// Rd1 (Rd) = R1 (Rn) AND ~Op2
	ARM_MVN,		// Rd1 (Rd) = ~Op2
	ARM_LDR,		// Rd1 (Rt) = memory[ R2 (Rn) + R3 (Rm) ]
	ARM_STR,		// memory [ R2 (Rn) + R3 (Rm) ] = R1 (Rt)
	ARM_LDRD,		// Rd1 (Rt), Rd2 (Rt2) = memory[ R2 (Rn) + R3 (Rm) ]
	ARM_STRD,		// memory [ R2 (Rn) + R3 (Rm) ] = R1 (Rt)
	ARM_LDR_LIT,	// Rd1 (Rt) = memory[ R2 (Rn) + imm ]
	ARM_STR_LIT,	// memory [ R2 (Rn) + imm ] = R1 (Rt)
	ARM_LDRD_LIT,	// Rd1 (Rt), Rd2 (Rt2) = memory[ R2 (Rn) + imm ]
	ARM_STRD_LIT,	// memory [ R2 (Rn) + imm ] = R1 (Rt)
	ARM_LDM,		// Rmask (<registers>) = memory [ Rn ], if (W) Rn +-= count of registers  (+ if U, - if ~U)
	ARM_STM,		// memory [ Rn ] = Rmask (<registers>), if (W) Rn +-= count of registers  (+ if U, - if ~U)

	ARM_LDRH,		// Rd1 (Rd
	ARM_STRH,
	ARM_LDRSH,
	ARM_STRSH,

	ARM_LDRB,
	ARM_STRB,
	ARM_LDRSB,
	ARM_STRSB,

	ARM_MRS,
	ARM_MSR,

	// VFP Instructions

	ARM_VCMP,
	ARM_VCVT,
	ARM_VDIV,
	ARM_VLDM,
	ARM_VLDR,
	ARM_VMOV,
	ARM_VMRS,
	ARM_VMSR,
	ARM_VPOP,
	ARM_VPUSH,
	ARM_VSQRT,
	ARM_VSTM32,
	ARM_VSTM64,

	//---------------------------
	// Next Instruction Set ...
	//---------------------------


	sizeof_mips_op_t
} Instruction_e;

typedef enum
{
	LOGICAL_LEFT,
	LOGICAL_RIGHT,
	ARITHMETIC_RIGHT,
	ROTATE_RIGHT
} Shift_e;

typedef enum
{
	EQ, 	// Z = 1			Equal
	NE, 	// Z = 0			Not Equal
	CS, 	// C = 1 			Unsigned Higher or same
	CC, 	// C = 0 			Unsigned Lower
	MI, 	// N = 1			Negative
	PL, 	// N = 0			Positive or Zero
	VS, 	// V = 1			Overflow
	VC, 	// V = 0			No Overflow
	HI, 	// C = 1 && Z = 0 	Unsigned Higher
	LS,		// C = 0 || Z = 1 	Unsigned Lower or equal
	GE,		// N = V 			Signed Greater or equal
	LT,		// N != V 			Signed less than
	GT,		// Z = 0 && N = V 	Signed greater then
	LE,		// Z = 1 || N! = V	Signed less than or equal
	AL,		// Always
	NV,		// Never
	LTZ = MI,
	GEZ = PL,
	LEZ = LE,
	GTZ	= GT,
	AL_B = NV + 1
} Condition_e;

typedef struct _Instruction
{
	struct _Instruction * nextInstruction;
	Instruction_e instruction;
	Condition_e cond;
	Shift_e shiftType;

	struct _Instruction * branchToThisInstruction;

	// ------------------- immediates --------------------------

	union{
		int32_t offset;
		int32_t immediate;
		uint8_t C0;
	};

	union{
		uint32_t shift;
		uint32_t rotate;
	};

	//--------------------- registers -----------------------------

	uint32_t Rmask;				// for arm LDM/STM

	reg_t Rd1;	// Output 1
	reg_t Rd2; 	// Output 2

	reg_t R1;  // Input 1
	reg_t R2;  // Input 2
	reg_t R3;  // Input 3

#if USE_INSTRUCTION_INIT_REGS
	reg_t Rd1_init;
	reg_t Rd2_init;
	reg_t R1_init;
	reg_t R2_init;
	reg_t R3_init;
#endif
	//----------------------- control bits ------------------------

	uint8_t A:1;			// Accumulate
	uint8_t B:1;			// Byte/Word bit, 1 = byte
	uint8_t I:1;			// Immediate, also used by ARM_B for absolute(1) or relative(0) branching
	uint8_t Ln:1;			// Link bit for branch
	uint8_t PR:1;			// Pre/Post increment, 1 for pre
	uint8_t S:1;			// Set condition flags
	uint8_t U:1;			// Up/Down, set for inc, clear for decrement
	uint8_t W:1;			// Writeback bit set to write to base register

	size_t outputAddress;

#if USE_INSTRUCTION_COMMENTS
	#define COMMENT_LENGTH (200)
	char comment[COMMENT_LENGTH];
#endif
} Instruction_t;

//-------------------------------------------------------------------

struct _code_seg;

Instruction_t* newInstrCopy(const Instruction_t* ins);

Instruction_t* Instr(Instruction_t* ins, const Instruction_e ins_e, const Condition_e cond, const regID_t Rd1, const regID_t R1, const regID_t R2);

Instruction_t* InstrI(Instruction_t* ins, const Instruction_e ins_e, const Condition_e cond, const regID_t Rd1, const regID_t R1, const regID_t R2, const int32_t imm);

Instruction_t* InstrS(Instruction_t* ins, const Instruction_e ins_e, const Condition_e cond, const regID_t Rd1, const regID_t R1, const regID_t R2);

Instruction_t* InstrIS(Instruction_t* ins, const Instruction_e ins_e, const Condition_e cond, const regID_t Rd1, const regID_t R1, const regID_t R2, const int32_t imm);

Instruction_t* InstrIntB(Instruction_t* ins, const Condition_e cond, const Instruction_t* find_ins);

Instruction_t* InstrB(Instruction_t* ins, const Condition_e cond, const int32_t offset, const uint32_t absolute);

Instruction_t* InstrBX(Instruction_t* ins, const Condition_e cond, const regID_t reg);


Instruction_t* InstrBL(Instruction_t* ins, const Condition_e cond, const int32_t offset, const uint32_t absolute);

Instruction_t* InstrPUSH(Instruction_t* ins, const Condition_e cond, const uint32_t Rmask);

Instruction_t* newEmptyInstr();

Instruction_t* newInstr(const Instruction_e ins, const Condition_e cond, const regID_t Rd1, const regID_t R1, const regID_t R2);

Instruction_t* newInstrI(const Instruction_e ins, const Condition_e cond, const regID_t Rd1, const regID_t R1, const regID_t R2, const int32_t imm);

Instruction_t* newInstrS(const Instruction_e ins, const Condition_e cond, const regID_t Rd1, const regID_t R1, const regID_t R2);

Instruction_t* newInstrIS(const Instruction_e ins, 	const Condition_e cond, const regID_t Rd1, const regID_t R1, const regID_t R2, const int32_t imm);

Instruction_t* newInstrIntB(const Condition_e cond, const Instruction_t* ins);

Instruction_t* newInstrIntBL(const Condition_e cond, const Instruction_t* ins);

Instruction_t* newInstrB(const Condition_e cond, const int32_t offset, const uint32_t absolute);

Instruction_t* newInstrBX(const Condition_e cond, const regID_t reg);

Instruction_t* newInstrBL(const Condition_e cond, const int32_t offset, const uint32_t absolute);

Instruction_t* newInstrPUSH(const Condition_e cond, const uint32_t Rmask);

Instruction_t* newInstrPOP(const Condition_e cond, const uint32_t Rmask);


void CodeSeg_print(const struct _code_seg * const codeSegment);

void printf_Intermediate(const Instruction_t* const ins, uint8_t heading);

void Intermediate_Literals_print(const struct _code_seg * const codeSegment);

Instruction_t* InstrFree(struct _code_seg * const codeSegment, Instruction_t* ins);

#endif



