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

#define OPS_JUMP 	(0x01000)
#define OPS_BRANCH 	(0x02000)
#define OPS_CALL 	(0x04000)	//BRANCH and LINK

#define OPS_LIKELY	(0x10000)
#define OPS_STR  	(0x20000)
#define OPS_LDR  	(0x40000)

#define STRIP(x) (x & 0xfff)

//Offsets into General Purpose Register space
#define REG_NOT_USED (0xffff)
#define REG_TEMP	 (0x1000)
#define REG_HOST	 (0x4000)

#define FP_REG_OFFSET	(256)
#define C0_REG_OFFSET	(512)
#define REG_MIPS	 	(640)

//These are the HOST registers. Translation MUST not change them
#define REG_HOST_FP	 (REG_HOST | 0xb)
#define REG_HOST_SP	 (REG_HOST | 0x0d)
#define REG_HOST_LR	 (REG_HOST | 0x0e)
#define REG_HOST_PC	 (REG_HOST | 0x0f)

#define REG_HOST_R0	 (REG_HOST | 0x00)
#define REG_HOST_R1	 (REG_HOST | 0x01)
#define REG_HOST_R2	 (REG_HOST | 0x02)
#define REG_HOST_R3	 (REG_HOST | 0x03)

//The following is for Register Mask operations
#define REG_HOST_STM_FP 		(0x0800)
#define REG_HOST_STM_SP			(0x2000)
#define REG_HOST_STM_LR 		(0x4000)
#define REG_HOST_STM_PC 		(0x8000)
#define REG_HOST_STM_GENERAL 	(0x17FF)
#define REG_HOST_STM_EABI 		(0x000F)

//-----------------------------------------------------------------------------

// MIPS R4300 Co processor 0 Registers		// Value also forms the offset from FP when its at 0x8000 0000
#define REG_CONTEXT  	(C0_REG_OFFSET + 16)
#define REG_BADVADDR 	(C0_REG_OFFSET + 32)
#define REG_COUNT    	(C0_REG_OFFSET + 36)
#define REG_ENTRYHI  	(C0_REG_OFFSET + 40)
#define REG_COMPARE  	(C0_REG_OFFSET + 44)
#define REG_STATUS   	(C0_REG_OFFSET + 48)
#define REG_CAUSE    	(C0_REG_OFFSET + 52)
#define REG_EPC		 	(C0_REG_OFFSET + 56)

// MIPS R4300 Special CPU Registers			// Value also forms the offset from FP when its at 0x8000 0000
#define REG_PC       	(REG_MIPS + 0x00)
#define REG_FCR0	 	(REG_MIPS + 0x04)
#define REG_FCR31    	(REG_MIPS + 0x08)
#define REG_MULTHI  	(REG_MIPS + 0x0c)
#define REG_MULTLO   	(REG_MIPS + 0x10)
#define REG_LLBIT    	(REG_MIPS + 0x14)

//Temorary Registers
#define REG_TEMP_MEM1 	(REG_TEMP | 0x00)
#define REG_TEMP_MEM2 	(REG_TEMP | 0x01)




typedef int16_t reg_t;

typedef enum
{
	UNKNOWN,
	LITERAL,
	INVALID,

	SLL,
    SRL,
    SRA,
    SLLV,
    SRLV,
    SRAV,
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
    SUB,	// Rd1 (rd) = R1 (rs) - R2 (rt) 	To add 32-bit integers. If overflow occurs, then trap.
    SUBU,	// Rd1 (rd) = R1 (rs) - R2 (rt) 	To add 32-bit integers.
    AND,	// Rd1 (rd) = R1 (rs) & R2 (rt) 	To do a bitwise logical AND.
    OR,		// Rd1 (rd) = R1 (rs) | R2 (rt)		The contents of GPR rs are combined with the contents of GPR rt in a bitwise logical OR operation. The result is placed into GPR rd.
    XOR,	// Rd1 (rd) = R1 (rs) ^ R2 (rt)
    NOR,	// Rd1 (rd) = R1 (rs) | ~R2 (rt)	The contents of GPR rs are combined with the contents of GPR rt in a bitwise logical NOR operation. The result is placed into GPR rd.
    SLT,
    SLTU,
    DADD,	// Rd1 (rd) = R1 (rs) + R2 (rt) 	To add 64-bit integers. If overflow occurs, then trap.
    DADDU,	// Rd1 (rd) = R1 (rs) + R2 (rt) 	To add 64-bit integers.
    DSUB,	// Rd1 (rd) = R1 (rs) - R2 (rt) 	To add 64-bit integers. If overflow occurs, then trap.
    DSUBU,	// Rd1 (rd) = R1 (rs) - R2 (rt) 	To add 64-bit integers.
    TGE,
    TGEU,
    TLT,
    TLTU,
    TEQ,
    TNE,
    DSLL,	// Rd1 (rd) = R1 (rt) << imm5 	To left shift a doubleword by a fixed amount  0 to 31 bits.
    DSRL,	// Rd1 (rd) = R1 (rt) >> imm5 	To right shift a doubleword by a fixed amount  0 to 31 bits.
    DSRA,	// Rd1 (rd) = R1 (rt) >> imm5 (arithmetic). To right shift a doubleword by a fixed amount  0 to 31 bits.
    DSLL32,	// Rd1 (rd) = R1 (rt) << (32 + imm5) To left shift a doubleword by a fixed amount  32 to 63 bits.
    DSRL32, // Rd1 (rd) = R1 (rt) >> (32 + imm5) To right shift a doubleword by a fixed amount  32 to 63 bits.
    DSRA32, // Rd1 (rd) = R1 (rt) >> (32 + imm5) (arithmetic). To right shift a doubleword by a fixed amount  32 to 63 bits.
	TGEI,
	TGEIU,
	TLTI,
	TLTIU,
	TEQI,
	TNEI,
	ADDI,	// Rd1 (rt) = R1 (rs) + imm16 		To add a constant to a 32-bit integer. If overflow occurs, then trap.
	ADDIU,	// Rd1 (rt) = R1 (rs) + imm16 		To add a constant to a 32-bit integer.
	SLTI,
	SLTIU,
	ANDI,	// Rd1 (rt) = R1 (rs) & imm16 		To do a bitwise logical AND with a constant.
	ORI,
	XORI,
	LUI,	// Rd1 (rt) = imm16 << 16 			The 16-bit immediate is shifted left 16 bits and concatenated with 16 bits of low-order zeros. The 32-bit result is sign-extended and placed into GPR rt.
	MFC0,
	MTC0,
	TLBR,
	TLBWI,
	TLBWR,
	TLBP,
	ERET,
	MFC1,
	DMFC1,
	CFC1,
	MTC1,
	DMTC1,
	CTC1,
	BC1,
	BC1F,
	BC1T,
	BC1FL,
	BC1TL,
	ADD_S,
	SUB_S,
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
	CEIL_W_S,
	FLOOR_W_S,
	CVT_D_S,
	CVT_W_S,
	CVT_L_S,
	C_F_S,
	C_UN_S,
	C_EQ_S,
	C_UEQ_S,
	C_OLT_S,
	C_ULT_S,
	C_OLE_S,
	C_ULE_S,
	C_SF_S,
	C_NGLE_S,
	C_SEQ_S,
	C_NGL_S,
	C_LT_S,
	C_NGE_S,
	C_LE_S,
	C_NGT_S,
	ADD_D,
	SUB_D,
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
	CVT_D_W,
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
	SC,
	SWC1,
	SCD,
	SDC1,
	SD,

	//---------------------------
	J 	// PC = PC(31:27) | (PC + imm26)(26:0) 		This is a PC-region branch (not PC-relative); the effective target address is in the “current” 256 MB aligned region. The low 28 bits of the target address is the instr_index field shifted left 2 bits. The remaining upper bits are the corresponding bits of the address of the instruction in the delay slot (not the branch itself). Jump to the effective target address. Execute the instruction following the jump, in the branch delay slot, before jumping.
		= OPS_JUMP + STRIP(SD) + 1,
	JR,	// PC = R1 (rs) 							Jump to the effective target address in GPR rs. Execute the instruction following the jump, in the branch delay slot, before jumping.

	//---------------------------

	BLTZ 	// Branch if R1 (rs) < 0, 				offset is imm16 << 2 added to next instruction address. If the contents of GPR rs are less than zero (sign bit is 1), branch to the effective target address after the instruction in the delay slot is executed.
		= OPS_BRANCH + STRIP(JR) + 1,
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

	JAL		// Place the return address link in GPR 31. The return link is the address of the second instruction following the branch, where execution would continue after a procedure call. This is a PC-region branch (not PC-relative); the effective target address is in the “current” 256 MB aligned region. The low 28 bits of the target address is the instr_index field shifted left 2 bits. The remaining upper bits are the corresponding bits of the address of the instruction in the delay slot (not the branch itself). Jump to the effective target address. Execute the instruction following the jump, in the branch delay slot, before jumping.
		= OPS_CALL + STRIP(BGTZL) + 1,
	JALR,	// Rd = PC + 8, PC = R1 (rs)			Place the return address link in GPR rd. The return link is the address of the second instruction following the branch, where execution would continue after a procedure call. Jump to the effective target address in GPR rs. Execute the instruction following the jump, in the branch delay slot, before jumping.
	BLTZAL,	// Branch Link if R1 (rs) < 0, 			offset is imm16 << 2 added to next instruction address. Place the return address link in GPR 31. The return link is the address of the second instruction following the branch (not the branch itself), where execution would continue after a procedure call. If the contents of GPR rs are less than zero (sign bit is 1), branch to the effective target address after the instruction in the delay slot is executed. GPR 31 must not be used for the source register rs
	BGEZAL,	// Branch Link if R1 (rs) >= 0, 		offset is imm16 << 2 added to next instruction address. Place the return address link in GPR 31. The return link is the address of the second instruction following the branch, where execution would continue after a procedure call. GPR 31 must not be used for the source register rs
	BLTZALL // Branch Link Likely if R1 (rs) < 0, 	offset is imm16 << 2 added to next instruction address. Place the return address link in GPR 31. The return link is the address of the second instruction following the branch (not the branch itself), where execution would continue after a procedure call. If the contents of GPR rs are less than zero (sign bit is 1), branch to the effective target address after the instruction in the delay slot is executed. GPR 31 must not be used for the source register rs

		= OPS_CALL + OPS_LIKELY + STRIP(BGEZAL)+1,
	BGEZALL,// Branch Link Likely if R1 (rs) >= 0, 	offset is imm16 << 2 added to next instruction address. Place the return address link in GPR 31. If the contents of GPR rs are greater than or equal to zero (sign bit is 0), branch to the effective target address after the instruction in the delay slot is executed. If the branch	is not taken, the instruction in the delay slot is not executed. The return link is the address of the second instruction following the branch, where execution would continue after a procedure call. GPR 31 must not be used for the source register rs

	//---------------------------

	SB = OPS_STR + STRIP(BGEZALL) + 1,
	SH,
	SWL,
	SW,
	SDL,
	SDR,
	SWR,

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
	ARM_PKHBT,
	ARM_PKHTB,
	ARM_RBIT,
	ARM_REV,
	ARM_REV16,
	ARM_REVSH,
	ARM_AND,
	ARM_BRANCH,
	ARM_EOR,
	ARM_SUB,
	ARM_RSB,
	ARM_ADD,
	ARM_ADC,
	ARM_SBC,
	ARM_RSC,
	ARM_TST,		// Rn AND Op2
	ARM_TEQ,		// Rn EOR Op2
	ARM_CMP,		// Rn - Op2
	ARM_CMN,		// Rn + Op2
	ARM_ORR,
	ARM_MOV,
	ARM_BIC,		// Rd = Rn AND ~Op2
	ARM_MVN,		// Rd = ~Op2ARM_LDR
	ARM_LDR,
	ARM_STR,
	ARM_LDR_LIT,
	ARM_STD_LIT,
	ARM_LDM,
	ARM_STM,
	ARM_MRS,
	ARM_MSR,
	sizeof_mips_op_t
} Instruction_e;

static const char* Instruction_ascii[sizeof_mips_op_t+1] =
{
	"UNKNOWN",
	".word",
	"INVALID",

	"SLL",
    "SRL",
    "SRA",
    "SLLV",
    "SRLV",
    "SRAV",
    "SYSCALL",
    "BREAK",
    "SYNC",
    "MFHI",
    "MTHI",
    "MFLO",
    "MTLO",
    "DSLLV",
    "DSRLV",
    "DSRAV",
    "MULT",
    "MULTU",
    "DIV",
    "DIVU",
    "DMULT",
    "DMULTU",
    "DDIV",
    "DDIVU",
    "ADD",
    "ADDU",
    "SUB",
    "SUBU",
    "AND",
    "OR",
    "XOR",
    "NOR",
    "SLT",
    "SLTU",
    "DADD",
    "DADDU",
    "DSUB",
    "DSUBU",
    "TGE",
    "TGEU",
    "TLT",
    "TLTU",
    "TEQ",
    "TNE",
    "DSLL",
    "DSRL",
    "DSRA",
    "DSLL32",
    "DSRL32",
    "DSRA32",
	"TGEI",
	"TGEIU",
	"TLTI",
	"TLTIU",
	"TEQI",
	"TNEI",
	"ADDI",
	"ADDIU",
	"SLTI",
	"SLTIU",
	"ANDI",
	"ORI",
	"XORI",
	"LUI",
	"MFC0",
	"MTC0",
	"TLBR",
	"TLBWI",
	"TLBWR",
	"TLBP",
	"ERET",
	"MFC1",
	"DMFC1",
	"CFC1",
	"MTC1",
	"DMTC1",
	"CTC1",
	"BC1",
	"BC1F",
	"BC1T",
	"BC1FL",
	"BC1TL",
	"ADD_S",
	"SUB_S",
	"MUL_S",
	"DIV_S",
	"SQRT_S",
	"ABS_S",
	"MOV_S",
	"NEG_S",
	"ROUND_L_S",
	"TRUNC_L_S",
	"CEIL_L_S",
	"FLOOR_L_S",
	"ROUND_W_S",
	"TRUNC_W_S",
	"CEIL_W_S",
	"FLOOR_W_S",
	"CVT_D_S",
	"CVT_W_S",
	"CVT_L_S",
	"C_F_S",
	"C_UN_S",
	"C_EQ_S",
	"C_UEQ_S",
	"C_OLT_S",
	"C_ULT_S",
	"C_OLE_S",
	"C_ULE_S",
	"C_SF_S",
	"C_NGLE_S",
	"C_SEQ_S",
	"C_NGL_S",
	"C_LT_S",
	"C_NGE_S",
	"C_LE_S",
	"C_NGT_S",
	"ADD_D",
	"SUB_D",
	"MUL_D",
	"DIV_D",
	"SQRT_D",
	"ABS_D",
	"MOV_D",
	"NEG_D",
	"ROUND_L_D",
	"TRUNC_L_D",
	"CEIL_L_D",
	"FLOOR_L_D",
	"ROUND_W_D",
	"TRUNC_W_D",
	"CEIL_W_D",
	"FLOOR_W_D",
	"CVT_S_D",
	"CVT_W_D",
	"CVT_L_D",
	"C_F_D",
	"C_UN_D",
	"C_EQ_D",
	"C_UEQ_D",
	"C_OLT_D",
	"C_ULT_D",
	"C_OLE_D",
	"C_ULE_D",
	"C_SF_D",
	"C_NGLE_D",
	"C_SEQ_D",
	"C_NGL_D",
	"C_LT_D",
	"C_NGE_D",
	"C_LE_D",
	"C_NGT_D",
	"CVT_S_W",
	"CVT_D_W",
	"CVT_S_L",
	"CVT_D_L",
	"DADDI",
	"DADDIU",
	"CACHE",
	"LL",
	"LWC1",
	"LLD",
	"LDC1",
	"LD",
	"SC",
	"SWC1",
	"SCD",
	"SDC1",
	"SD",

	//---------------------------
	"J",
	"JR",

	//---------------------------

	"BLTZ",
	"BGEZ",
	"BEQ",
	"BNE",
	"BLEZ",
	"BGTZ",

	"BLTZL",
	"BGEZL",
	"BEQL",
	"BNEL",
	"BLEZL",
	"BGTZL",

	//--------------------------

	"JAL",
	"JALR",
	"BLTZAL",
	"BGEZAL",

	"BLTZALL",
	"BGEZALL",

	//---------------------------

	"SB",
	"SH",
	"SWL",
	"SW",
	"SDL",
	"SDR",
	"SWR",

	//---------------------------

	"LDL",
	"LDR",
	"LB",
	"LH",
	"LWL",
	"LW",
	"LBU",
	"LHU",
	"LWR",
	"LWU",

	//---------------------------
	// ARM assembler
	//---------------------------

	"bfc",
	"bfi",
	"pkhbt",
	"pkhtb",
	"rbit",
	"rev",
	"rev16",
	"revsh",
	"and",
	"b",
	"eor",
	"sub",
	"rsb",
	"add",
	"adc",
	"sbc",
	"rsc",
	"tst",
	"teq",
	"cmp",
	"cmn",
	"orr",
	"mov",
	"bic",
	"mvn",
	"ldr",
	"str",
	"ldr",
	"str",
	"ldm",
	"stm",
	"mrs",
	"msr",
	"[size of Instruction_t]"
};

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

	// ------------------- immediates --------------------------

	union{
		int32_t offset;
		int32_t immediate;
	};

	union{
		int32_t shift;
	int32_t rotate;
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
	uint8_t I:1;			// Immediate
	uint8_t Ln:1;			// Link bit for branch
	uint8_t PR:1;			// Pre/Post increment, 1 for pre
	uint8_t S:1;			// Set condition flags
	uint8_t U:1;			// Up/Down, set for inc, clear for decrement
	uint8_t W:1;			// Writeback bit set to write to base register

} Instruction_t;

//-------------------------------------------------------------------

struct _code_seg;

void Intermediate_print(struct _code_seg *codeSegment);

Instruction_t* newEmptyInstr();

Instruction_t* newInstr(Instruction_e ins, Condition_e cond,reg_t Rd1, reg_t R1, reg_t R2, int32_t imm);
Instruction_t* newInstrS(Instruction_e ins, Condition_e cond,reg_t Rd1, reg_t R1, reg_t R2, int32_t imm);

Instruction_t* newInstrPUSH(Condition_e cond, uint32_t Rmask);

Instruction_t* newInstrPOP(Condition_e cond, uint32_t Rmask);

#endif



