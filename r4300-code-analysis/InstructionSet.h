/*
 * InstructionSet.h
 *
 *  Created on: 2 Jun 2014
 *      Author: rjhender
 */

#ifndef INSTRUCTIONSET_H_
#define INSTRUCTIONSET_H_

#include <stdint.h>

//-------------------------------------------------------------------

#define OPS_JUMP 			(0x01000)
#define OPS_LINK 			(0x02000)
#define OPS_BRANCH 			(0x04000)

#define OPS_LIKELY			(0x10000)
#define OPS_STR  			(0x20000)
#define OPS_LDR  			(0x40000)

#define STRIP(x) 			(x & 0xfff)

//Register IDs
#define REG_NOT_USED 		(0xffff)
#define REG_FP		 		(0x0020)
#define REG_WIDE	 		(0x0040)			// 64-32 bit part of (register&0x3F).
#define REG_CO		 		(0x0080)
#define REG_SPECIAL	 		(REG_CO) + 32

#define REG_TEMP			(0x0100)
#define REG_HOST		 	(0x0200)


#define REG_CONTEXT  		(REG_CO + 4)
#define REG_BADVADDR 		(REG_CO + 8)
#define REG_COUNT    		(REG_CO + 9)
#define REG_ENTRYHI  		(REG_CO + 10)
#define REG_COMPARE  		(REG_CO + 11)
#define REG_STATUS   		(REG_CO + 12)
#define REG_CAUSE    		(REG_CO + 13)
#define REG_EPC		 		(REG_CO + 14)

// MIPS R4300 Special CPU Registers			// Value also forms the offset from FP when its at 0x8000 0000
#define REG_PC       		(REG_SPECIAL + 1)
#define REG_FCR0	 		(REG_SPECIAL + 2)
#define REG_FCR31    		(REG_SPECIAL + 3)
#define REG_MULTHI  		(REG_SPECIAL + 4)
#define REG_MULTLO   		(REG_SPECIAL + 5)
#define REG_LLBIT    		(REG_SPECIAL + 6)

//Temorary Registers
#define REG_TEMP_MEM1 		(REG_TEMP | 0x00)
#define REG_TEMP_MEM2 		(REG_TEMP | 0x01)
#define REG_TEMP_GEN1 		(REG_TEMP | 0x02)
#define REG_TEMP_GEN2 		(REG_TEMP | 0x03)
#define REG_TEMP_GEN3 		(REG_TEMP | 0x04)
#define REG_TEMP_GEN4 		(REG_TEMP | 0x05)
#define REG_TEMP_STR_CONST 	(REG_TEMP | 0x06)
#define REG_TEMP_JR1		(REG_TEMP | 0x07)
#define REG_TEMP_CALL2C		(REG_TEMP | 0x08)

//These are the HOST registers. Translation MUST not change them
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
#define REG_EMU_FP			REG_HOST_FP
#define REG_EMU_FLAGS		REG_HOST_R12

// emulation flag bits
#define REG_EMU_FLAG_DS		(0x00)

//The following is for Register Mask operations
#define REG_HOST_STM_FP 		(0x0800)
#define REG_HOST_STM_SP			(0x2000)
#define REG_HOST_STM_LR 		(0x4000)
#define REG_HOST_STM_PC 		(0x8000)
#define REG_HOST_STM_GENERAL 	(0x17FF)
#define REG_HOST_STM_EABI 		(0x000F)
#define REG_HOST_STM_EABI2      (0x5FF0)
#define REG_HOST_STM_ALL        (0x7FFF)

#define REG_T_SIZE	 (REG_HOST + 0x10)

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
	UNKNOWN,
	LITERAL,
	INVALID,
	NO_OP,

	SLL,	// Rd1 (rd) = R1 (rt) << imm5 				The contents of the low-order 32-bit word of GPR rt are shifted left, inserting zeroes into the emptied bits; the word result is placed in GPR rd. The bit shift count is specified by sa. If rd is a 64-bit register, the result word is sign-extended.
    SRL,	// Rd1 (rd) = R1 (rt) >> imm5				The contents of the low-order 32-bit word of GPR rt are shifted right, inserting zeros into the emptied bits; the word result is placed in GPR rd. The bit shift count is specified by sa. If rd is a 64-bit register, the result word is sign-extended.
    SRA,	// Rd1 (rd) = R1 (rt) >> imm5 (arithmetic)	The contents of the low-order 32-bit word of GPR rt are shifted right, duplicating the sign-bit (bit 31) in the emptied bits; the word result is placed in GPR rd. The bit shift count is specified by sa. If rd is a 64-bit register, the result word is sign-extended.
    SLLV,	// Rd1 (rd) = R1 (rt) << R2 (rs)			The contents of the low-order 32-bit word of GPR rt are shifted left, inserting zeroes into the emptied bits; the result word is placed in GPR rd. The bit shift count is specified by the low-order five bits of GPR rs. If rd is a 64-bit register, the result word is sign-extended.
    SRLV,	// Rd1 (rd) = R1 (rt) >> R2 (rs)			The contents of the low-order 32-bit word of GPR rt are shifted right, inserting zeros into the emptied bits; the word result is placed in GPR rd. The bit shift count is specified by the low-order five bits of GPR rs. If rd is a 64-bit register, the result word is sign-extended.
    SRAV,	// Rd1 (rd) = R1 (rt) >> R2 (rs) (arithmetic) The contents of the low-order 32-bit word of GPR rt are shifted right, duplicating the sign-bit (bit 31) in the emptied bits; the word result is placed in GPR rd. The bit shift count is specified by the low-order five bits of GPR rs. If rd is a 64-bit register, the result word is sign-extended.
    SYSCALL,
    BREAK,	// A breakpoint exception occurs, immediately and unconditionally transferring control to the exception handler. The code field is available for use as software parameters, but is retrieved by the exception handler only by loading the contents of the memory word containing the instruction.
    SYNC,
    MFHI,	// Rd1 (rd) = HI 					To copy the special purpose HI register to a GPR.
    MTHI,	// HI = R1 (rs)						To copy a GPR to the special purpose HI register.
    MFLO,	// Rd1 (rd) = LO 					To copy the special purpose LO register to a GPR.
    MTLO,	// LO = R1 (rs)						To copy a GPR to the special purpose LO register.
    DSLLV,	// Rd1 (rd) = R1 (rt) << R2 (rs) 	To left shift a doubleword by a variable number of bits. The bit shift count in the range 0 to 63 is specified by the low-order six bits in GPR rs
    DSRLV,	// Rd1 (rd) = R1 (rt) << R2 (rs) 	To right shift a doubleword by a variable number of bits. The bit shift count in the range 0 to 63 is specified by the low-order six bits in GPR rs
    DSRAV,	// Rd1 (rd) = R1 (rt) << R2 (rs) (arithmetic). To right shift a doubleword by a variable number of bits. The bit shift count in the range 0 to 63 is specified by the low-order six bits in GPR rs
    MULT,	// LO, HI = R1 (rs) * R2 (rt)		The 32-bit word value in GPR rt is multiplied by the 32-bit value in GPR rs, treating both operands as signed values, to produce a 64-bit result. The low-order 32-bit word of the result is placed into special register LO, and the high-order 32-bit word is placed into special register HI.
    MULTU,	// LO, HI = R1 (rs) * R2 (rt)		The 32-bit word value in GPR rt is multiplied by the 32-bit value in GPR rs, treating both operands as unsigned values, to produce a 64-bit result. The low-order 32-bit word of the result is placed into special register LO, and the high-order 32-bit word is placed into special register HI.
    DIV,	// (LO, HI) = R1 (rs) / R2 (rt) 	To divide 32-bit signed integers. The 32-bit word value in GPR rs is divided by the 32-bit value in GPR rt, treating both operands as signed values. The 32-bit quotient is placed into special register LO and the 32-bit remainder is placed into special register HI.
    DIVU,	// (LO, HI) = R1 (rs) / R2 (rt) 	To divide 32-bit unsigned integers. The 32-bit word value in GPR rs is divided by the 32-bit value in GPR rt, treating both operands as unsigned values. The 32-bit quotient is placed into special register LO and the 32-bit remainder is placed into special register HI.
    DMULT,	// (LO, HI) = R1 (rs) * R2 (rt) 	To multiply 64-bit signed integers. The 64-bit doubleword value in GPR rt is multiplied by the 64-bit value in GPR rs, treating both operands as signed values, to produce a 128-bit result. The low-order 64-bit doubleword of the result is placed into special register LO, and the high-order 64-bit doubleword is placed into special register HI.
    DMULTU,	// (LO, HI) = R1 (rs) * R2 (rt) 	To multiply 64-bit unsigned integers. The 64-bit doubleword value in GPR rt is multiplied by the 64-bit value in GPR rs, treating both operands as unsigned values, to produce a 128-bit result. The low-order 64-bit doubleword of the result is placed into special register LO, and the high-order 64-bit doubleword is placed into special register HI.
    DDIV,	// (LO, HI) = R1 (rs) / R2 (rt) 	To divide 64-bit signed integers. The 64-bit doubleword in GPR rs is divided by the 64-bit doubleword in GPR rt, treating both operands as signed values. The 64-bit quotient is placed into special register LO and the 64-bit remainder is placed into special register HI.
    DDIVU,	// (LO, HI) = R1 (rs) / R2 (rt) 	To divide 64-bit unsigned integers. The 64-bit doubleword in GPR rs is divided by the 64-bit doubleword in GPR rt, treating both operands as unsigned values. The 64-bit quotient is placed into special register LO and the 64-bit remainder is placed into special register HI.
    ADD,	// Rd1 (rd) = R1 (rs) + R2 (rt) 	To add 32-bit integers. If overflow occurs, then trap.
    ADDU,	// Rd1 (rd) = R1 (rs) + R2 (rt) 	To add 32-bit integers.
    SUB,	// Rd1 (rd) = R1 (rs) - R2 (rt) 	The 32-bit word value in GPR rt is subtracted from the 32-bit value in GPR rs to produce a 32-bit result. If the subtraction results in 32-bit 2’s complement arithmetic overflow then the destination register is not modified and an Integer Overflow exception occurs. If it does not overflow, the 32-bit result is placed into GPR rd.
    SUBU,	// Rd1 (rd) = R1 (rs) - R2 (rt) 	The 32-bit word value in GPR rt is subtracted from the 32-bit value in GPR rs and the 32-bit arithmetic result is placed into GPR rd. No integer overflow exception occurs under any circumstances.
    AND,	// Rd1 (rd) = R1 (rs) & R2 (rt) 	To do a bitwise logical AND.
    OR,		// Rd1 (rd) = R1 (rs) | R2 (rt)		The contents of GPR rs are combined with the contents of GPR rt in a bitwise logical OR operation. The result is placed into GPR rd.
    XOR,	// Rd1 (rd) = R1 (rs) ^ R2 (rt)		Combine the contents of GPR rs and GPR rt in a bitwise logical exclusive OR operation and place the result into GPR rd.
    NOR,	// Rd1 (rd) = R1 (rs) | ~R2 (rt)	The contents of GPR rs are combined with the contents of GPR rt in a bitwise logical NOR operation. The result is placed into GPR rd.
    SLT,	// Rd1 (rd) = R1 (rs) < R2 (rt)		Compare the contents of GPR rs and GPR rt as signed integers and record the Boolean result of the comparison in GPR rd. If GPR rs is less than GPR rt the result is 1 (true), otherwise 0 (false).
    SLTU,	// Rd1 (rd) = R1 (rs) < R2 (rt)		Compare the contents of GPR rs and GPR rt as unsigned integers and record the Boolean result of the comparison in GPR rd. If GPR rs is less than GPR rt the result is 1 (true), otherwise 0 (false).
    DADD,	// Rd1 (rd) = R1 (rs) + R2 (rt) 	To add 64-bit integers. If overflow occurs, then trap.
    DADDU,	// Rd1 (rd) = R1 (rs) + R2 (rt) 	To add 64-bit integers.
    DSUB,	// Rd1 (rd) = R1 (rs) - R2 (rt) 	To add 64-bit integers. If overflow occurs, then trap.
    DSUBU,	// Rd1 (rd) = R1 (rs) - R2 (rt) 	To add 64-bit integers.
    TGE,	// TRAP when R1 (rs) >= R2 (rt)		Compare the contents of GPR rs and GPR rt as signed integers; if GPR rs is greater than or equal to GPR rt then take a Trap exception.
    TGEU,	// TRAP when R1 (rs) >= R2 (rt)		Compare the contents of GPR rs and GPR rt as unsigned integers; if GPR rs is greater than or equal to GPR rt then take a Trap exception.
    TLT,	// TRAP when R1 (rs) < R2 (rt)		Compare the contents of GPR rs and GPR rt as signed integers; if GPR rs is less than GPR rt then take a Trap exception.
    TLTU,	// TRAP when R1 (rs) < R2 (rt)		Compare the contents of GPR rs and GPR rt as unsigned integers; if GPR rs is less than GPR rt then take a Trap exception.
    TEQ,	// TRAP when R1 (rs) == R2 (rt)		Compare the contents of GPR rs and GPR rt as signed integers; if GPR rs is equal to GPR rt then take a Trap exception. The contents of the code field are ignored by hardware and may be used to encode information for system software. To retrieve the information, system software must load the instruction word from memory.
    TNE,	// TRAP when R1 (rs) != R2 (rt)		Compare the contents of GPR rs and GPR rt as signed integers; if GPR rs is not equal to GPR rt then take a Trap exception.
    DSLL,	// Rd1 (rd) = R1 (rt) << imm5 		To left shift a doubleword by a fixed amount  0 to 31 bits.
    DSRL,	// Rd1 (rd) = R1 (rt) >> imm5 		To right shift a doubleword by a fixed amount  0 to 31 bits.
    DSRA,	// Rd1 (rd) = R1 (rt) >> imm5 (arithmetic). To right shift a doubleword by a fixed amount  0 to 31 bits.
    DSLL32,	// Rd1 (rd) = R1 (rt) << (32 + imm5) To left shift a doubleword by a fixed amount  32 to 63 bits.
    DSRL32, // Rd1 (rd) = R1 (rt) >> (32 + imm5) To right shift a doubleword by a fixed amount  32 to 63 bits.
    DSRA32, // Rd1 (rd) = R1 (rt) >> (32 + imm5) (arithmetic). To right shift a doubleword by a fixed amount  32 to 63 bits.
	TGEI,	// TRAP when R1 (rs) >= imm16		Compare the contents of GPR rs and the 16-bit signed immediate as signed integers; if GPR rs is greater than or equal to immediate then take a Trap exception.
	TGEIU,	// TRAP when R1 (rs) >= imm16		Compare the contents of GPR rs and the 16-bit sign-extended immediate as unsigned integers; if GPR rs is greater than or equal to immediate then take a Trap exception. Because the 16-bit immediate is sign-extended before comparison, the instruction is able to represent the smallest or largest unsigned numbers. The representable values are at the minimum [0, 32767] or maximum [max_unsigned-32767, max_unsigned] end of the unsigned range.
	TLTI,	// TRAP when R1 (rs) < imm16		Compare the contents of GPR rs and the 16-bit signed immediate as signed integers; if GPR rs is less than immediate then take a Trap exception.
	TLTIU,	// TRAP when R1 (rs) < imm16		Compare the contents of GPR rs and the 16-bit sign-extended immediate as unsigned integers; if GPR rs is less than immediate then take a Trap exception. Because the 16-bit immediate is sign-extended before comparison, the instruction is able to represent the smallest or largest unsigned numbers. The representable values are at the minimum [0, 32767] or maximum [max_unsigned-32767, max_unsigned] end of the unsigned range.
	TEQI,	// TRAP when R1 (rs) == imm16		Compare the contents of GPR rs and the 16-bit signed immediate as signed integers; if GPR rs is equal to immediate then take a Trap exception.
	TNEI,	// TRAP when R1 (rs) != imm16		Compare the contents of GPR rs and the 16-bit signed immediate as signed integers; if GPR rs is not equal to immediate then take a Trap exception.
	ADDI,	// Rd1 (rt) = R1 (rs) + imm16 		To add a constant to a 32-bit integer. If overflow occurs, then trap.
	ADDIU,	// Rd1 (rt) = R1 (rs) + imm16 		To add a constant to a 32-bit integer.
	SLTI,	// Rd1 (rt) = R1 (rs) + imm16		Compare the contents of GPR rs and the 16-bit signed immediate as signed integers and record the Boolean result of the comparison in GPR rt. If GPR rs is less than immediate the result is 1 (true), otherwise 0 (false).
	SLTIU,	// Rd1 (rt) = R1 (rs) + imm16		Compare the contents of GPR rs and the sign-extended 16-bit immediate as unsigned integers and record the Boolean result of the comparison in GPR rt. If GPR rs is less than immediate the result is 1 (true), otherwise 0 (false). Because the 16-bit immediate is sign-extended before comparison, the instruction is able to represent the smallest or largest unsigned numbers. The representable values are at the minimum [0, 32767] or maximum [max_unsigned-32767, max_unsigned] end of the unsigned range.
	ANDI,	// Rd1 (rt) = R1 (rs) & imm16 		To do a bitwise logical AND with a constant.
	ORI,	// Rd1 (rt) = R1 (rs) | imm16 		To do a bitwise logical OR with a constant.
	XORI,	// Rd1 (rt) = R1 (rs) ^ imm16 		Combine the contents of GPR rs and the 16-bit zero-extended immediate in a bitwise logical exclusive OR operation and place the result into GPR rt.
	LUI,	// Rd1 (rt) = imm16 << 16 			The 16-bit immediate is shifted left 16 bits and concatenated with 16 bits of low-order zeros. The 32-bit result is sign-extended and placed into GPR rt.
	MFC0,	// Rd1 = R1
	MTC0,	// Rd1 = R1
	TLBR,
	TLBWI,
	TLBWR,
	TLBP,
	ERET,		// Return from Exception
	MFC1,
	DMFC1,
	CFC1,		// Rd1 (rt) = R1 (fs)				Copy FPU fs into rt
	MTC1,
	DMTC1,
	CTC1,		// Rd1 (fs) = R1 (rt)				Copy from GPR rt to a FPU control register fs
	BC1F,		// Branch on FP false
	BC1T,
	BC1FL,
	BC1TL,
	ADD_S,		// Rd1 (fd) = R1 (fs) + R2 (ft)
	SUB_S,		// Rd1 (fd) = R1 (fs) - R2 (ft)
	MUL_S,
	DIV_S,
	SQRT_S,
	ABS_S,
	MOV_S,
	NEG_S,
	ROUND_L_S,
	TRUNC_L_S,
	CEIL_L_S,
	FLOOR_L_S,
	ROUND_W_S,
	TRUNC_W_S,
	CEIL_W_S,		// Rd1 (fd) = convert( R1 (fs) )		The value in FPR fs in format fmt, is converted to a value in 32-bit word fixed-point format rounding toward +∞ (rounding mode 2). The result is placed in FPR fd. When the source value is Infinity, NaN, or rounds to an integer outside the range -231 to 231-1, the result cannot be represented correctly and an IEEE Invalid Operation condition exists. The result depends on the FP exception model currently active.
	FLOOR_W_S,
	CVT_D_S,		// Rd1 (fd) = convert( R1 (fs) )		The value in FPR fs in format fmt is converted to a value in double floating-point format rounded according to the current rounding mode in FCSR. The result is placed in FPR fd.
	CVT_W_S,
	CVT_L_S,
	C_F_S,		// FP Compare	False
	C_UN_S,		// FP Compare	Unordered
	C_EQ_S,		// FP Compare	Equal
	C_UEQ_S,	// FP Compare	Unordered or Equal
	C_OLT_S,	// FP Compare	Ordered or Less Than
	C_ULT_S,	// FP Compare	Unordered or Less Than
	C_OLE_S,	// FP Compare	Ordered or Less Than or Equal
	C_ULE_S,	// FP Compare	Unordered or Less Than or Equal
	C_SF_S,		// FP Compare	Signaling False
	C_NGLE_S,	// FP Compare	Not Greater than or Less Than or Equal
	C_SEQ_S,	// FP Compare	Signaling Equal
	C_NGL_S,	// FP Compare	Not Greater Than or Less Than
	C_LT_S,		// FP Compare	Less Than
	C_NGE_S,	// FP Compare	Not Greater Than or Equal
	C_LE_S,		// FP Compare	Less Than or Equal
	C_NGT_S,	// FP Compare	Not Greater Than
	ADD_D,		// Rd1 (fd) = R1 (fs) + R2 (ft)
	SUB_D,		// Rd1 (fd) = R1 (fs) - R2 (ft)
	MUL_D,
	DIV_D,
	SQRT_D,
	ABS_D,
	MOV_D,
	NEG_D,
	ROUND_L_D,
	TRUNC_L_D,
	CEIL_L_D,
	FLOOR_L_D,
	ROUND_W_D,
	TRUNC_W_D,
	CEIL_W_D,
	FLOOR_W_D,
	CVT_S_D,
	CVT_W_D,
	CVT_L_D,
	C_F_D,
	C_UN_D,
	C_EQ_D,
	C_UEQ_D,
	C_OLT_D,
	C_ULT_D,
	C_OLE_D,
	C_ULE_D,
	C_SF_D,
	C_NGLE_D,
	C_SEQ_D,
	C_NGL_D,
	C_LT_D,
	C_NGE_D,
	C_LE_D,
	C_NGT_D,
	CVT_S_W,
	CVT_D_W,	// Rd1 (fd) = convert( R1 (fs) )		The value in FPR fs in format fmt is converted to a value in double floating-point format rounded according to the current rounding mode in FCSR. The result is placed in FPR fd.
	CVT_S_L,
	CVT_D_L,
	DADDI,	// Rd1 (rt) = R1 (rs) + imm16 		To add a constant to a 64-bit integer. If overflow occurs, then trap.
	DADDIU,	// Rd1 (rt) = R1 (rs) + imm16 		To add a constant to a 64-bit integer.
	CACHE,
	LL,		// Rd1 (rt) = memory[ R1 (base) + imm16]	The LL and SC instructions provide primitives to implement atomic Read-Modify-Write (RMW) operations for cached memory locations. The 16-bit signed offset is added to the contents of GPR base to form an effective address.
	LWC1,	// Rd1 (rt) = memory[ R1 (base) + imm16]	The contents of the 32-bit word at the memory location specified by the aligned effective address are fetched and made available to coprocessor unit zz. The 16-bit signed offset is added to the contents of GPR base to form the effective address.
	LLD,	// Rd1 (rt) = memory[ R1 (base) + imm16]	The LL and SC instructions provide primitives to implement atomic Read-Modify-Write (RMW) operations for cached memory locations. The 16-bit signed offset is added to the contents of GPR base to form an effective address.
	LDC1,	// Rd1 (rt) = memory[ R1 (base) + imm16]	The contents of the 64-bit doubleword at the memory location specified by the aligned effective address are fetched and made available to coprocessor unit 1. The 16-bit signed offset is added to the contents of GPR base to form the effective address. The manner in which each coprocessor uses the data is defined by the individual coprocessor specifications. The usual operation would place the data into coprocessor general register rt.
	LD,		// Rd1 (rt) = memory[ R1 (base) + imm16]	The contents of the 64-bit doubleword at the memory location specified by the aligned effective address are fetched and placed in GPR rt. The 16-bit signed offset is added to the contents of GPR base to form the effective address. The effective address must be naturally aligned. If any of the three least-significant bits of the address are non-zero, an Address Error exception occurs.
	SC,		// memory[ R1 (base) + imm16] = R2 (rt)		The SC completes the RMW sequence begun by the preceding LL instruction executed on the processor. If it would complete the RMW sequence atomically, then the least-significant 32-bit word of GPR rt is stored into memory at the location specified by the aligned effective address and a one, indicating success, is written into GPR rt. Otherwise, memory is not modified and a zero, indicating failure, is written into GPR rt.
	SWC1,	// memory[ R1 (base) + imm16] = R2 (rt)		Coprocessor unit zz supplies a 32-bit word which is stored at the memory location specified by the aligned effective address. The 16-bit signed offset is added to the contents of GPR base to form the effective address.
	SCD,	// memory[ R1 (base) + imm16] = R2 (rt)		The SCD completes the RMW sequence begun by the preceding LLD instruction executed on the processor. If it would complete the RMW sequence atomically, then the 64-bit doubleword of GPR rt is stored into memory at the location specified by the aligned effective address and a one, indicating success, is written into GPR rt. Otherwise, memory is not modified and a zero, indicating failure, is written into GPR rt.
	SDC1,	// memory[ R1 (base) + imm16] = R2 (rt)		Coprocessor unit zz supplies a 64-bit doubleword which is stored at the memory location specified by the aligned effective address. The 16-bit signed offset is added to the contents of GPR base to form the effective address.
	SD,		//

	//---------------------------
	J 		// PC = PC(31:27) | (PC + imm26)(26:0) 		This is a PC-region branch (not PC-relative); the effective target address is in the “current” 256 MB aligned region. The low 28 bits of the target address is the instr_index field shifted left 2 bits. The remaining upper bits are the corresponding bits of the address of the instruction in the delay slot (not the branch itself). Jump to the effective target address. Execute the instruction following the jump, in the branch delay slot, before jumping.
		= OPS_JUMP + STRIP(SD) + 1,
	JR,		// PC = R1 (rs) 							Jump to the effective target address in GPR rs. Execute the instruction following the jump, in the branch delay slot, before jumping.
	JAL		// Place the return address link in GPR 31. The return link is the address of the second instruction following the branch, where execution would continue after a procedure call. This is a PC-region branch (not PC-relative); the effective target address is in the “current” 256 MB aligned region. The low 28 bits of the target address is the instr_index field shifted left 2 bits. The remaining upper bits are the corresponding bits of the address of the instruction in the delay slot (not the branch itself). Jump to the effective target address. Execute the instruction following the jump, in the branch delay slot, before jumping.
		= OPS_JUMP + OPS_LINK + STRIP(JR) + 1,
	JALR,	// Rd = PC + 8, PC = R1 (rs)			Place the return address link in GPR rd. The return link is the address of the second instruction following the branch, where execution would continue after a procedure call. Jump to the effective target address in GPR rs. Execute the instruction following the jump, in the branch delay slot, before jumping.

	//---------------------------

	BLTZ 	// Branch if R1 (rs) < 0, 				offset is imm16 << 2 added to next instruction address. If the contents of GPR rs are less than zero (sign bit is 1), branch to the effective target address after the instruction in the delay slot is executed.
		= OPS_BRANCH + STRIP(JALR) + 1,
	BGEZ,	// Branch if R1 (rs) >= 0, 				offset is imm16 << 2 added to next instruction address. If the contents of GPR rs are greater than or equal to zero (sign bit is 0), branch to the effective target address after the instruction in the delay slot is executed.
	BEQ,	// Branch if R1 (rs) == R2 (rt), 		offset is imm16 << 2 added to next instruction address. If the contents of GPR rs and GPR rt are equal, branch to the effective target address after the instruction in the delay slot is executed.
	BNE,	// Branch if R1 (rs) != R2 (rt), 		offset is imm16 << 2 added to next instruction address. If the contents of GPR rs and GPR rt are not equal, branch to the effective target address after the instruction in the delay slot is executed.
	BLEZ,	// Branch if R1 (rs) <= 0, 				offset is imm16 << 2 added to next instruction address. If the contents of GPR rs are less than or equal to zero (sign bit is 1 or value is zero), branch to the effective target address after the instruction in the delay slot is executed.
	BGTZ,	// Branch if R1 (rs) > 0,				offset is imm16 << 2 added to next instruction address. If the contents of GPR rs are greater than zero (sign bit is 0 but value not zero), branch to the effective target address after the instruction in the delay slot is executed.

	BLTZL	// Branch Likely if R1 (rs) < 0, 		offset is imm16 << 2 added to next instruction address. If the contents of GPR rs are less than zero (sign bit is 1), branch to the effective target address after the instruction in the delay slot is executed. If the branch is not taken, the instruction in the delay slot is not executed.
		= OPS_BRANCH + OPS_LIKELY + STRIP(BGTZ) + 1,
	BGEZL,	// Branch Likely if R1 (rs) >= 0, 		offset is imm16 << 2 added to next instruction address. If the contents of GPR rs are greater than or equal to zero (sign bit is 0), branch to the effective target address after the instruction in the delay slot is executed. If the branch is not taken, the instruction in the delay slot is not executed.
	BEQL,	// Branch Likely if R1 (rs) == R2 (rt),	offset is imm16 << 2 added to next instruction address. If the contents of GPR rs and GPR rt are equal, branch to the target address after the instruction in the delay slot is executed. If the branch is not taken, the instruction in the delay slot is not executed.
	BNEL,	// Branch Likely if R1 (rs) != R2 (rt),	offset is imm16 << 2 added to next instruction address. If the contents of GPR rs and GPR rt are not equal, branch to the effective target address after the instruction in the delay slot is executed. If the branch is not taken, the instruction in the delay slot is not executed.
	BLEZL,	// Branch Likely if R1 (rs) <= 0, 		offset is imm16 << 2 added to next instruction address. If the contents of GPR rs are less than or equal to zero (sign bit is 1 or value is zero), branch to the effective target address after the instruction in the delay slot is executed.	If the branch is not taken, the instruction in the delay slot is not executed.
	BGTZL,	// Branch Likely if R1 (rs) >= 0, 		offset is imm16 << 2 added to next instruction address. If the contents of GPR rs are greater than zero (sign bit is 0 but value not zero), branch to the effective target address after the instruction in the delay slot is executed. If the branch is not taken, the instruction in the delay slot is not executed.

	//--------------------------

	BLTZAL	// Branch Link if R1 (rs) < 0, 			offset is imm16 << 2 added to next instruction address. Place the return address link in GPR 31. The return link is the address of the second instruction following the branch (not the branch itself), where execution would continue after a procedure call. If the contents of GPR rs are less than zero (sign bit is 1), branch to the effective target address after the instruction in the delay slot is executed. GPR 31 must not be used for the source register rs
		= OPS_BRANCH + OPS_LINK + STRIP(BGTZL) + 1,
	BGEZAL,	// Branch Link if R1 (rs) >= 0, 		offset is imm16 << 2 added to next instruction address. Place the return address link in GPR 31. The return link is the address of the second instruction following the branch, where execution would continue after a procedure call. GPR 31 must not be used for the source register rs
	BLTZALL // Branch Link Likely if R1 (rs) < 0, 	offset is imm16 << 2 added to next instruction address. Place the return address link in GPR 31. The return link is the address of the second instruction following the branch (not the branch itself), where execution would continue after a procedure call. If the contents of GPR rs are less than zero (sign bit is 1), branch to the effective target address after the instruction in the delay slot is executed. GPR 31 must not be used for the source register rs
		= OPS_BRANCH + OPS_LINK + OPS_LIKELY + STRIP(BGEZAL)+1,
	BGEZALL,// Branch Link Likely if R1 (rs) >= 0, 	offset is imm16 << 2 added to next instruction address. Place the return address link in GPR 31. If the contents of GPR rs are greater than or equal to zero (sign bit is 0), branch to the effective target address after the instruction in the delay slot is executed. If the branch	is not taken, the instruction in the delay slot is not executed. The return link is the address of the second instruction following the branch, where execution would continue after a procedure call. GPR 31 must not be used for the source register rs

	//---------------------------

	SB 		// memory[ R1 (base) + imm16] = R2 (rt)				The least-significant 8-bit byte of GPR rt is stored in memory at the location specified by the effective address. The 16-bit signed offset is added to the contents of GPR base to form the effective address.
		= OPS_STR + STRIP(BGEZALL) + 1,
	SH,		// memory[ R1 (base) + imm16] = R2 (rt)					The least-significant 16-bit halfword of register rt is stored in memory at the location specified by the aligned effective address. The 16-bit signed offset is added to the contents of GPR base to form the effective address.
	SWL,	// memory[ R1 (base) + imm16] = partial-upper R2 (rt)				The 16-bit signed offset is added to the contents of GPR base to form an effective address (EffAddr). EffAddr is the address of the most-significant of four consecutive bytes forming a word in memory (W) starting at an arbitrary byte boundary. A part of W, the most-significant one to four bytes, is in the aligned word containing EffAddr. The same number of the most-significant (left) bytes from the word in GPR rt are stored into these bytes of W.
	SW,		// memory[ R1 (base) + imm16] = R2 (rt)					The least-significant 32-bit word of register rt is stored in memory at the location specified by the aligned effective address. The 16-bit signed offset is added to the contents of GPR base to form the effective address.
	SDL,	// memory[ R1 (base) + imm16] = partial-upper R2 (rt)	The 16-bit signed offset is added to the contents of GPR base to form an effective address (EffAddr). EffAddr is the address of the most-significant of eight consecutive bytes forming a doubleword in memory (DW) starting at an arbitrary byte boundary. A part of DW, the most-significant one to eight bytes, is in the aligned doubleword containing EffAddr. The same number of most-significant (left) bytes of GPR rt are stored into these bytes of DW.
	SDR,	// memory[ R1 (base) + imm16] = partial-lower R2 (rt)	The 16-bit signed offset is added to the contents of GPR base to form an effective address (EffAddr). EffAddr is the address of the least-significant of eight consecutive bytes forming a doubleword in memory (DW) starting at an arbitrary byte boundary. A part of DW, the least-significant one to eight bytes, is in the aligned doubleword containing EffAddr. The same number of least-significant (right) bytes of GPR rt are stored into these bytes of DW.
	SWR,	// memory[ R1 (base) + imm16] = partial-lower R2 (rt)	The 16-bit signed offset is added to the contents of GPR base to form an effective address (EffAddr). EffAddr is the address of the least-significant of four consecutive bytes forming a word in memory (W) starting at an arbitrary byte boundary. A part of W, the least-significant one to four bytes, is in the aligned word containing EffAddr. The same number of the least-significant (right) bytes from the word in GPR rt are stored into these bytes of W.

	//---------------------------

	LDL 	// Rd1 (rt) = Rd1 | memory[ (R1 (base) + imm16)(bytes ((base+imm)&7) to 8)]		To load the most-significant part of a doubleword from an unaligned memory address.
		= OPS_LDR + STRIP(SWR) + 1,
	LDR,	// Rd1 (rt) = Rd1 | memory[ (R1 (base) + imm16)(bytes (8 to ((base+imm)&7 + 8)))]
	LB,		// Rd1 (rt) = memory[ R1 (base) + imm16]			The contents of the 8-bit byte at the memory location specified by the effective address are fetched, sign-extended, and placed in GPR rt. The 16-bit signed offset is added to the contents of GPR base to form the effective address.
	LH,		// Rd1 (rt) = memory[ R1 (base) + imm16]			The contents of the 16-bit halfword at the memory location specified by the aligned effective address are fetched, sign-extended, and placed in GPR rt. The 16-bit signed offset is added to the contents of GPR base to form the effective address. The effective address must be naturally aligned. If the least-significant bit of the address is non-zero, an Address Error exception occurs.
	LWL,	// Rd1 (rt) = Rd1 | memory[ (R1 (base) + imm16)(bytes ((base+imm)&3) to 4)]
	LW,		// Rd1 (rt) = memory[ R1 (base) + imm16]			The contents of the 32-bit word at the memory location specified by the aligned effective address are fetched, sign-extended to the GPR register length if necessary, and placed in GPR rt. The 16-bit signed offset is added to the contents of GPR base to form the effective address.
	LBU, 	// Rd1 (rt) = memory[ R1 (base) + imm16]			The contents of the 8-bit byte at the memory location specified by the effective address are fetched, sign-extended, and placed in GPR rt. The 16-bit signed offset is added to the contents of GPR base to form the effective address.
	LHU,	// Rd1 (rt) = memory[ R1 (base) + imm16]			The contents of the 16-bit halfword at the memory location specified by the aligned effective address are fetched, zero-extended, and placed in GPR rt. The 16-bit signed offset is added to the contents of GPR base to form the effective address. The effective address must be naturally aligned. If the least-significant bit of the address is non-zero, an Address Error exception occurs.
	LWR,	// Rd1 (rt) = Rd1 | memory[ (R1 (base) + imm16)(bytes ( 4 to (base+imm +4)&3))]
	LWU,	// Rd1 (rt) = memory[ R1 (base) + imm16]			The contents of the 32-bit word at the memory location specified by the aligned effective address are fetched, zero-extended, and placed in GPR rt. The 16-bit signed offset is added to the contents of GPR base to form the effective address.

	//---------------------------
	// ARM assembler
	//---------------------------

	ARM_BFC= STRIP(LWU) + 1,
	ARM_BFI,
	ARM_CLZ,
	ARM_PKHBT,
	ARM_PKHTB,
	ARM_RBIT,
	ARM_REV,
	ARM_REV16,
	ARM_REVSH,
	ARM_AND,		// R1 (Rd) = R2 (Rn) & Op2
	ARM_B,
	ARM_EOR,
	ARM_SUB,		// Rd` (Rd) = R1 - Op2
	ARM_RSB,		// Rd1 (Rd) = Op2 - R1
	ARM_ADD,		// Rd1 (Rd) = R1 (Rn) + Op2
	ARM_ADC,		// Rd1 (Rd) = R1 (Rn) + Op2 + Carry
	ARM_ASR,		// Rd1 (Rd) = R1 (Rn) >> #<imm>
	ARM_SBC,		// Rd1 (Rd) = R1 (Rn) + Op2 + Carry
	ARM_RSC,
	ARM_TST,		// Rn & Op2
	ARM_TEQ,		// Rn ^ Op2
	ARM_CMP,		// Rn - Op2
	ARM_CMN,		// Rn + Op2
	ARM_ORR,		// Rd1 (Rd) = R1 (Rn) | Op2
	ARM_MOV,		// Rd1 (Rd) = Op2
	ARM_BIC,		// Rd1 (Rd) = R1 (Rn) AND ~Op2
	ARM_MVN,		// Rd1 (Rd) = ~Op2
	ARM_LDR,		// R1 (Rt) = memory[ R2 (Rn) + R3 (Rm) ]
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
	EQ, // Z = 1
	NE, // Z = 0
	CS, // C = 1
	CC, // C = 0
	MI, // N = 1
	PL, // N = 0
	VS, // V = 1
	VC, // V = 0
	HI, // C = 1 && Z = 0
	LS,	// C = 0 || Z = 1
	GE,	// N = V
	LT,	// N != V
	GT,	// Z = 0 && N = V
	LE,	// Z = 1 || N! = V
	AL,	// Always
	NV	// Never
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

	//----------------------- control bits ------------------------

	uint8_t A:1;			// Accumulate
	uint8_t B:1;			// Byte/Word bit, 1 = byte
	uint8_t I:1;			// Immediate, also used by ARM_B for absolute(1) or relative(0) branching
	uint8_t Ln:1;			// Link bit for branch
	uint8_t PR:1;			// Pre/Post increment, 1 for pre
	uint8_t S:1;			// Set condition flags
	uint8_t U:1;			// Up/Down, set for inc, clear for decrement
	uint8_t W:1;			// Writeback bit set to write to base register

} Instruction_t;

//-------------------------------------------------------------------

struct _code_seg;

Instruction_t* newEmptyInstr();

Instruction_t* newInstr(const Instruction_e ins, const Condition_e cond, const regID_t Rd1, const regID_t R1, const regID_t R2);

Instruction_t* newInstrI(const Instruction_e ins, const Condition_e cond, const regID_t Rd1, const regID_t R1, const regID_t R2, const int32_t imm);

Instruction_t* newInstrS(const Instruction_e ins, const Condition_e cond, const regID_t Rd1, const regID_t R1, const regID_t R2);

Instruction_t* newInstrIS(const Instruction_e ins, 	const Condition_e cond, const regID_t Rd1, const regID_t R1, const regID_t R2, const int32_t imm);

Instruction_t* newInstrPUSH(const Condition_e cond, const uint32_t Rmask);

Instruction_t* newInstrPOP(const Condition_e cond, const uint32_t Rmask);

Instruction_t* Instr(Instruction_t* ins, const Instruction_e ins_e, const Condition_e cond, const regID_t Rd1, const regID_t R1, const regID_t R2);

Instruction_t* InstrI(Instruction_t* ins, const Instruction_e ins_e, const Condition_e cond, const regID_t Rd1, const regID_t R1, const regID_t R2, const int32_t imm);

void Intermediate_print(const struct _code_seg * const codeSegment);

void Intermediate_Literals_print(const struct _code_seg * const codeSegment);

Instruction_t* InstrFree(struct _code_seg * const codeSegment, Instruction_t* ins);

#endif



