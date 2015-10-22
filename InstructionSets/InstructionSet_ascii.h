#ifndef INSTRUCTIONSET_ASCC
#define INSTRUCTIONSET_ASCC

#include "InstructionSet.h"

static const char* Instruction_ascii[sizeof_mips_op_t+1] = {
	
		 //  Dynamic Recompiler  Generic instructions
	"DR_UNKNOWN",
	"DR_LITERAL",
	"DR_INVALID",
	"DR_NO_OP",
	"DR_INT_BRANCH",		 //  Intermediae Branch to the Instruction (->branchToThisInstruction)
	"DR_INT_BRANCH_LINK",		 //  Intermediae Branch to the Instruction (->branchToThisInstruction)
	
		 //  MIPS
	
	"SLL",		 //  Rd1 (rd) = R1 (rt) << imm5 				The contents of the low-order 32-bit word of GPR rt are shifted left, inserting zeroes into the emptied bits; the word result is placed in GPR rd. The bit shift count is specified by sa. If rd is a 64-bit register, the result word is sign-extended.
	"SRL",		 //  Rd1 (rd) = R1 (rt) >> imm5				The contents of the low-order 32-bit word of GPR rt are shifted right, inserting zeros into the emptied bits; the word result is placed in GPR rd. The bit shift count is specified by sa. If rd is a 64-bit register, the result word is sign-extended.
	"SRA",		 //  Rd1 (rd) = R1 (rt) >> imm5 (arithmetic)	The contents of the low-order 32-bit word of GPR rt are shifted right, duplicating the sign-bit (bit 31) in the emptied bits; the word result is placed in GPR rd. The bit shift count is specified by sa. If rd is a 64-bit register, the result word is sign-extended.
	"SLLV",		 //  Rd1 (rd) = R1 (rt) << R2 (rs)			The contents of the low-order 32-bit word of GPR rt are shifted left, inserting zeroes into the emptied bits; the result word is placed in GPR rd. The bit shift count is specified by the low-order five bits of GPR rs. If rd is a 64-bit register, the result word is sign-extended.
	"SRLV",		 //  Rd1 (rd) = R1 (rt) >> R2 (rs)			The contents of the low-order 32-bit word of GPR rt are shifted right, inserting zeros into the emptied bits; the word result is placed in GPR rd. The bit shift count is specified by the low-order five bits of GPR rs. If rd is a 64-bit register, the result word is sign-extended.
	"SRAV",		 //  Rd1 (rd) = R1 (rt) >> R2 (rs) (arithmetic) The contents of the low-order 32-bit word of GPR rt are shifted right, duplicating the sign-bit (bit 31) in the emptied bits; the word result is placed in GPR rd. The bit shift count is specified by the low-order five bits of GPR rs. If rd is a 64-bit register, the result word is sign-extended.
	"SYSCALL",
	"BREAK",		 //  A breakpoint exception occurs, immediately and unconditionally transferring control to the exception handler. The code field is available for use as software parameters, but is retrieved by the exception handler only by loading the contents of the memory word containing the instruction.
	"SYNC",
	"MFHI",		 //  Rd1 (rd) = HI 					To copy the special purpose HI register to a GPR.
	"MTHI",		 //  HI = R1 (rs)						To copy a GPR to the special purpose HI register.
	"MFLO",		 //  Rd1 (rd) = LO 					To copy the special purpose LO register to a GPR.
	"MTLO",		 //  LO = R1 (rs)						To copy a GPR to the special purpose LO register.
	"DSLLV",		 //  Rd1 (rd) = R1 (rt) << R2 (rs) 	To left shift a doubleword by a variable number of bits. The bit shift count in the range 0 to 63 is specified by the low-order six bits in GPR rs
	"DSRLV",		 //  Rd1 (rd) = R1 (rt) << R2 (rs) 	To right shift a doubleword by a variable number of bits. The bit shift count in the range 0 to 63 is specified by the low-order six bits in GPR rs
	"DSRAV",		 //  Rd1 (rd) = R1 (rt) << R2 (rs) (arithmetic). To right shift a doubleword by a variable number of bits. The bit shift count in the range 0 to 63 is specified by the low-order six bits in GPR rs
	"MULT",		 //  LO, HI = R1 (rs) * R2 (rt)		The 32-bit word value in GPR rt is multiplied by the 32-bit value in GPR rs, treating both operands as signed values, to produce a 64-bit result. The low-order 32-bit word of the result is placed into special register LO, and the high-order 32-bit word is placed into special register HI.
	"MULTU",		 //  LO, HI = R1 (rs) * R2 (rt)		The 32-bit word value in GPR rt is multiplied by the 32-bit value in GPR rs, treating both operands as unsigned values, to produce a 64-bit result. The low-order 32-bit word of the result is placed into special register LO, and the high-order 32-bit word is placed into special register HI.
	"DIV",		 //  (LO, HI) = R1 (rs)  R2 (rt) 	To divide 32-bit signed integers. The 32-bit word value in GPR rs is divided by the 32-bit value in GPR rt, treating both operands as signed values. The 32-bit quotient is placed into special register LO and the 32-bit remainder is placed into special register HI.
	"DIVU",		 //  (LO, HI) = R1 (rs)  R2 (rt) 	To divide 32-bit unsigned integers. The 32-bit word value in GPR rs is divided by the 32-bit value in GPR rt, treating both operands as unsigned values. The 32-bit quotient is placed into special register LO and the 32-bit remainder is placed into special register HI.
	"DMULT",		 //  (LO, HI) = R1 (rs) * R2 (rt) 	To multiply 64-bit signed integers. The 64-bit doubleword value in GPR rt is multiplied by the 64-bit value in GPR rs, treating both operands as signed values, to produce a 128-bit result. The low-order 64-bit doubleword of the result is placed into special register LO, and the high-order 64-bit doubleword is placed into special register HI.
	"DMULTU",		 //  (LO, HI) = R1 (rs) * R2 (rt) 	To multiply 64-bit unsigned integers. The 64-bit doubleword value in GPR rt is multiplied by the 64-bit value in GPR rs, treating both operands as unsigned values, to produce a 128-bit result. The low-order 64-bit doubleword of the result is placed into special register LO, and the high-order 64-bit doubleword is placed into special register HI.
	"DDIV",		 //  (LO, HI) = R1 (rs)  R2 (rt) 	To divide 64-bit signed integers. The 64-bit doubleword in GPR rs is divided by the 64-bit doubleword in GPR rt, treating both operands as signed values. The 64-bit quotient is placed into special register LO and the 64-bit remainder is placed into special register HI.
	"DDIVU",		 //  (LO, HI) = R1 (rs)  R2 (rt) 	To divide 64-bit unsigned integers. The 64-bit doubleword in GPR rs is divided by the 64-bit doubleword in GPR rt, treating both operands as unsigned values. The 64-bit quotient is placed into special register LO and the 64-bit remainder is placed into special register HI.
	"ADD",		 //  Rd1 (rd) = R1 (rs) + R2 (rt) 	To add 32-bit integers. If overflow occurs, then trap.
	"ADDU",		 //  Rd1 (rd) = R1 (rs) + R2 (rt) 	To add 32-bit integers.
	"SUB",		 //  Rd1 (rd) = R1 (rs) - R2 (rt) 	The 32-bit word value in GPR rt is subtracted from the 32-bit value in GPR rs to produce a 32-bit result. If the subtraction results in 32-bit 2’s complement arithmetic overflow then the destination register is not modified and an Integer Overflow exception occurs. If it does not overflow, the 32-bit result is placed into GPR rd.
	"SUBU",		 //  Rd1 (rd) = R1 (rs) - R2 (rt) 	The 32-bit word value in GPR rt is subtracted from the 32-bit value in GPR rs and the 32-bit arithmetic result is placed into GPR rd. No integer overflow exception occurs under any circumstances.
	"AND",		 //  Rd1 (rd) = R1 (rs) & R2 (rt) 	To do a bitwise logical AND.
	"OR",			 //  Rd1 (rd) = R1 (rs) | R2 (rt)		The contents of GPR rs are combined with the contents of GPR rt in a bitwise logical OR operation. The result is placed into GPR rd.
	"XOR",		 //  Rd1 (rd) = R1 (rs) ^ R2 (rt)		Combine the contents of GPR rs and GPR rt in a bitwise logical exclusive OR operation and place the result into GPR rd.
	"NOR",		 //  Rd1 (rd) = R1 (rs) | ~R2 (rt)	The contents of GPR rs are combined with the contents of GPR rt in a bitwise logical NOR operation. The result is placed into GPR rd.
	"SLT",		 //  Rd1 (rd) = R1 (rs) < R2 (rt)		Compare the contents of GPR rs and GPR rt as signed integers and record the Boolean result of the comparison in GPR rd. If GPR rs is less than GPR rt the result is 1 (true), otherwise 0 (false).
	"SLTU",		 //  Rd1 (rd) = R1 (rs) < R2 (rt)		Compare the contents of GPR rs and GPR rt as unsigned integers and record the Boolean result of the comparison in GPR rd. If GPR rs is less than GPR rt the result is 1 (true), otherwise 0 (false).
	"DADD",		 //  Rd1 (rd) = R1 (rs) + R2 (rt) 	To add 64-bit integers. If overflow occurs, then trap.
	"DADDU",		 //  Rd1 (rd) = R1 (rs) + R2 (rt) 	To add 64-bit integers.
	"DSUB",		 //  Rd1 (rd) = R1 (rs) - R2 (rt) 	To add 64-bit integers. If overflow occurs, then trap.
	"DSUBU",		 //  Rd1 (rd) = R1 (rs) - R2 (rt) 	To add 64-bit integers.
	"TGE",		 //  TRAP when R1 (rs) >= R2 (rt)		Compare the contents of GPR rs and GPR rt as signed integers; if GPR rs is greater than or equal to GPR rt then take a Trap exception.
	"TGEU",		 //  TRAP when R1 (rs) >= R2 (rt)		Compare the contents of GPR rs and GPR rt as unsigned integers; if GPR rs is greater than or equal to GPR rt then take a Trap exception.
	"TLT",		 //  TRAP when R1 (rs) < R2 (rt)		Compare the contents of GPR rs and GPR rt as signed integers; if GPR rs is less than GPR rt then take a Trap exception.
	"TLTU",		 //  TRAP when R1 (rs) < R2 (rt)		Compare the contents of GPR rs and GPR rt as unsigned integers; if GPR rs is less than GPR rt then take a Trap exception.
	"TEQ",		 //  TRAP when R1 (rs) == R2 (rt)		Compare the contents of GPR rs and GPR rt as signed integers; if GPR rs is equal to GPR rt then take a Trap exception. The contents of the code field are ignored by hardware and may be used to encode information for system software. To retrieve the information, system software must load the instruction word from memory.
	"TNE",		 //  TRAP when R1 (rs) != R2 (rt)		Compare the contents of GPR rs and GPR rt as signed integers; if GPR rs is not equal to GPR rt then take a Trap exception.
	"DSLL",		 //  Rd1 (rd) = R1 (rt) << imm5 		To left shift a doubleword by a fixed amount  0 to 31 bits.
	"DSRL",		 //  Rd1 (rd) = R1 (rt) >> imm5 		To right shift a doubleword by a fixed amount  0 to 31 bits.
	"DSRA",		 //  Rd1 (rd) = R1 (rt) >> imm5 (arithmetic). To right shift a doubleword by a fixed amount  0 to 31 bits.
	"DSLL32",		 //  Rd1 (rd) = R1 (rt) << (32 + imm5) To left shift a doubleword by a fixed amount  32 to 63 bits.
	"DSRL32", 	 //  Rd1 (rd) = R1 (rt) >> (32 + imm5) To right shift a doubleword by a fixed amount  32 to 63 bits.
	"DSRA32", 	 //  Rd1 (rd) = R1 (rt) >> (32 + imm5) (arithmetic). To right shift a doubleword by a fixed amount  32 to 63 bits.
	"TGEI",		 //  TRAP when R1 (rs) >= imm16		Compare the contents of GPR rs and the 16-bit signed immediate as signed integers; if GPR rs is greater than or equal to immediate then take a Trap exception.
	"TGEIU",		 //  TRAP when R1 (rs) >= imm16		Compare the contents of GPR rs and the 16-bit sign-extended immediate as unsigned integers; if GPR rs is greater than or equal to immediate then take a Trap exception. Because the 16-bit immediate is sign-extended before comparison, the instruction is able to represent the smallest or largest unsigned numbers. The representable values are at the minimum [0, 32767] or maximum [max_unsigned-32767, max_unsigned] end of the unsigned range.
	"TLTI",		 //  TRAP when R1 (rs) < imm16		Compare the contents of GPR rs and the 16-bit signed immediate as signed integers; if GPR rs is less than immediate then take a Trap exception.
	"TLTIU",		 //  TRAP when R1 (rs) < imm16		Compare the contents of GPR rs and the 16-bit sign-extended immediate as unsigned integers; if GPR rs is less than immediate then take a Trap exception. Because the 16-bit immediate is sign-extended before comparison, the instruction is able to represent the smallest or largest unsigned numbers. The representable values are at the minimum [0, 32767] or maximum [max_unsigned-32767, max_unsigned] end of the unsigned range.
	"TEQI",		 //  TRAP when R1 (rs) == imm16		Compare the contents of GPR rs and the 16-bit signed immediate as signed integers; if GPR rs is equal to immediate then take a Trap exception.
	"TNEI",		 //  TRAP when R1 (rs) != imm16		Compare the contents of GPR rs and the 16-bit signed immediate as signed integers; if GPR rs is not equal to immediate then take a Trap exception.
	"ADDI",		 //  Rd1 (rt) = R1 (rs) + imm16 		To add a constant to a 32-bit integer. If overflow occurs, then trap.
	"ADDIU",		 //  Rd1 (rt) = R1 (rs) + imm16 		To add a constant to a 32-bit integer.
	"SLTI",		 //  Rd1 (rt) = R1 (rs) + imm16		Compare the contents of GPR rs and the 16-bit signed immediate as signed integers and record the Boolean result of the comparison in GPR rt. If GPR rs is less than immediate the result is 1 (true), otherwise 0 (false).
	"SLTIU",		 //  Rd1 (rt) = R1 (rs) + imm16		Compare the contents of GPR rs and the sign-extended 16-bit immediate as unsigned integers and record the Boolean result of the comparison in GPR rt. If GPR rs is less than immediate the result is 1 (true), otherwise 0 (false). Because the 16-bit immediate is sign-extended before comparison, the instruction is able to represent the smallest or largest unsigned numbers. The representable values are at the minimum [0, 32767] or maximum [max_unsigned-32767, max_unsigned] end of the unsigned range.
	"ANDI",		 //  Rd1 (rt) = R1 (rs) & imm16 		To do a bitwise logical AND with a constant.
	"ORI",		 //  Rd1 (rt) = R1 (rs) | imm16 		To do a bitwise logical OR with a constant.
	"XORI",		 //  Rd1 (rt) = R1 (rs) ^ imm16 		Combine the contents of GPR rs and the 16-bit zero-extended immediate in a bitwise logical exclusive OR operation and place the result into GPR rt.
	"LUI",		 //  Rd1 (rt) = imm16 << 16 			The 16-bit immediate is shifted left 16 bits and concatenated with 16 bits of low-order zeros. The 32-bit result is sign-extended and placed into GPR rt.
	"MFC0",		 //  Rd1 = R1
	"MTC0",		 //  Rd1 = R1
	"TLBR",
	"TLBWI",
	"TLBWR",
	"TLBP",
	"ERET",			 //  Return from Exception
	"MFC1",
	"DMFC1",
	"CFC1",			 //  Rd1 (rt) = R1 (fs)				Copy FPU fs into rt
	"MTC1",
	"DMTC1",
	"CTC1",			 //  Rd1 (fs) = R1 (rt)				Copy from GPR rt to a FPU control register fs
	"BC1F",			 //  Branch on FP false
	"BC1T",
	"BC1FL",
	"BC1TL",
	"ADD_S",			 //  Rd1 (fd) = R1 (fs) + R2 (ft)
	"SUB_S",			 //  Rd1 (fd) = R1 (fs) - R2 (ft)
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
	"CEIL_W_S",			 //  Rd1 (fd) = convert( R1 (fs) )		The value in FPR fs in format fmt, is converted to a value in 32-bit word fixed-point format rounding toward +∞ (rounding mode 2). The result is placed in FPR fd. When the source value is Infinity, NaN, or rounds to an integer outside the range -231 to 231-1, the result cannot be represented correctly and an IEEE Invalid Operation condition exists. The result depends on the FP exception model currently active.
	"FLOOR_W_S",
	"CVT_D_S",			 //  Rd1 (fd) = convert( R1 (fs) )		The value in FPR fs in format fmt is converted to a value in double floating-point format rounded according to the current rounding mode in FCSR. The result is placed in FPR fd.
	"CVT_W_S",
	"CVT_L_S",
	"C_F_S",			 //  FP Compare	False
	"C_UN_S",			 //  FP Compare	Unordered
	"C_EQ_S",			 //  FP Compare	Equal
	"C_UEQ_S",		 //  FP Compare	Unordered or Equal
	"C_OLT_S",		 //  FP Compare	Ordered or Less Than
	"C_ULT_S",		 //  FP Compare	Unordered or Less Than
	"C_OLE_S",		 //  FP Compare	Ordered or Less Than or Equal
	"C_ULE_S",		 //  FP Compare	Unordered or Less Than or Equal
	"C_SF_S",			 //  FP Compare	Signaling False
	"C_NGLE_S",		 //  FP Compare	Not Greater than or Less Than or Equal
	"C_SEQ_S",		 //  FP Compare	Signaling Equal
	"C_NGL_S",		 //  FP Compare	Not Greater Than or Less Than
	"C_LT_S",			 //  FP Compare	Less Than
	"C_NGE_S",		 //  FP Compare	Not Greater Than or Equal
	"C_LE_S",			 //  FP Compare	Less Than or Equal
	"C_NGT_S",		 //  FP Compare	Not Greater Than
	"ADD_D",			 //  Rd1 (fd) = R1 (fs) + R2 (ft)
	"SUB_D",			 //  Rd1 (fd) = R1 (fs) - R2 (ft)
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
	"CVT_D_W",		 //  Rd1 (fd) = convert( R1 (fs) )		The value in FPR fs in format fmt is converted to a value in double floating-point format rounded according to the current rounding mode in FCSR. The result is placed in FPR fd.
	"CVT_S_L",
	"CVT_D_L",
	"DADDI",		 //  Rd1 (rt) = R1 (rs) + imm16 		To add a constant to a 64-bit integer. If overflow occurs, then trap.
	"DADDIU",		 //  Rd1 (rt) = R1 (rs) + imm16 		To add a constant to a 64-bit integer.
	"CACHE",
	"LL",			 //  Rd1 (rt) = memory[ R1 (base) + imm16]	The LL and SC instructions provide primitives to implement atomic Read-Modify-Write (RMW) operations for cached memory locations. The 16-bit signed offset is added to the contents of GPR base to form an effective address.
	"LWC1",		 //  Rd1 (rt) = memory[ R1 (base) + imm16]	The contents of the 32-bit word at the memory location specified by the aligned effective address are fetched and made available to coprocessor unit zz. The 16-bit signed offset is added to the contents of GPR base to form the effective address.
	"LLD",		 //  Rd1 (rt) = memory[ R1 (base) + imm16]	The LL and SC instructions provide primitives to implement atomic Read-Modify-Write (RMW) operations for cached memory locations. The 16-bit signed offset is added to the contents of GPR base to form an effective address.
	"LDC1",		 //  Rd1 (rt) = memory[ R1 (base) + imm16]	The contents of the 64-bit doubleword at the memory location specified by the aligned effective address are fetched and made available to coprocessor unit 1. The 16-bit signed offset is added to the contents of GPR base to form the effective address. The manner in which each coprocessor uses the data is defined by the individual coprocessor specifications. The usual operation would place the data into coprocessor general register rt.
	"LD",			 //  Rd1 (rt) = memory[ R1 (base) + imm16]	The contents of the 64-bit doubleword at the memory location specified by the aligned effective address are fetched and placed in GPR rt. The 16-bit signed offset is added to the contents of GPR base to form the effective address. The effective address must be naturally aligned. If any of the three least-significant bits of the address are non-zero, an Address Error exception occurs.
	"SC",			 //  memory[ R1 (base) + imm16] = R2 (rt)		The SC completes the RMW sequence begun by the preceding LL instruction executed on the processor. If it would complete the RMW sequence atomically, then the least-significant 32-bit word of GPR rt is stored into memory at the location specified by the aligned effective address and a one, indicating success, is written into GPR rt. Otherwise, memory is not modified and a zero, indicating failure, is written into GPR rt.
	"SWC1",		 //  memory[ R1 (base) + imm16] = R2 (rt)		Coprocessor unit zz supplies a 32-bit word which is stored at the memory location specified by the aligned effective address. The 16-bit signed offset is added to the contents of GPR base to form the effective address.
	"SCD",		 //  memory[ R1 (base) + imm16] = R2 (rt)		The SCD completes the RMW sequence begun by the preceding LLD instruction executed on the processor. If it would complete the RMW sequence atomically, then the 64-bit doubleword of GPR rt is stored into memory at the location specified by the aligned effective address and a one, indicating success, is written into GPR rt. Otherwise, memory is not modified and a zero, indicating failure, is written into GPR rt.
	"SDC1",		 //  memory[ R1 (base) + imm16] = R2 (rt)		Coprocessor unit zz supplies a 64-bit doubleword which is stored at the memory location specified by the aligned effective address. The 16-bit signed offset is added to the contents of GPR base to form the effective address.
	"SD",		
	
		 // ---------------------------
	"J" 			 //  PC = PC(31:27) | (PC + imm26)(26:0) 		This is a PC-region branch (not PC-relative); the effective target address is in the “current” 256 MB aligned region. The low 28 bits of the target address is the instr_index field shifted left 2 bits. The remaining upper bits are the corresponding bits of the address of the instruction in the delay slot (not the branch itself). Jump to the effective target address. Execute the instruction following the jump, in the branch delay slot, before jumping.
	,
	"JR",			 //  PC = R1 (rs) 							Jump to the effective target address in GPR rs. Execute the instruction following the jump, in the branch delay slot, before jumping.
	"JAL"			 //  Place the return address link in GPR 31. The return link is the address of the second instruction following the branch, where execution would continue after a procedure call. This is a PC-region branch (not PC-relative); the effective target address is in the “current” 256 MB aligned region. The low 28 bits of the target address is the instr_index field shifted left 2 bits. The remaining upper bits are the corresponding bits of the address of the instruction in the delay slot (not the branch itself). Jump to the effective target address. Execute the instruction following the jump, in the branch delay slot, before jumping.
	,
	"JALR",		 //  Rd = PC + 8, PC = R1 (rs)			Place the return address link in GPR rd. The return link is the address of the second instruction following the branch, where execution would continue after a procedure call. Jump to the effective target address in GPR rs. Execute the instruction following the jump, in the branch delay slot, before jumping.
	
		 // ---------------------------
	
	"BLTZ" 		 //  Branch if R1 (rs) < 0, 				offset is imm16 << 2 added to next instruction address. If the contents of GPR rs are less than zero (sign bit is 1), branch to the effective target address after the instruction in the delay slot is executed.
	,
	"BGEZ",		 //  Branch if R1 (rs) >= 0, 				offset is imm16 << 2 added to next instruction address. If the contents of GPR rs are greater than or equal to zero (sign bit is 0), branch to the effective target address after the instruction in the delay slot is executed.
	"BEQ",		 //  Branch if R1 (rs) == R2 (rt), 		offset is imm16 << 2 added to next instruction address. If the contents of GPR rs and GPR rt are equal, branch to the effective target address after the instruction in the delay slot is executed.
	"BNE",		 //  Branch if R1 (rs) != R2 (rt), 		offset is imm16 << 2 added to next instruction address. If the contents of GPR rs and GPR rt are not equal, branch to the effective target address after the instruction in the delay slot is executed.
	"BLEZ",		 //  Branch if R1 (rs) <= 0, 				offset is imm16 << 2 added to next instruction address. If the contents of GPR rs are less than or equal to zero (sign bit is 1 or value is zero), branch to the effective target address after the instruction in the delay slot is executed.
	"BGTZ",		 //  Branch if R1 (rs) > 0,				offset is imm16 << 2 added to next instruction address. If the contents of GPR rs are greater than zero (sign bit is 0 but value not zero), branch to the effective target address after the instruction in the delay slot is executed.
	
	"BLTZL"		 //  Branch Likely if R1 (rs) < 0, 		offset is imm16 << 2 added to next instruction address. If the contents of GPR rs are less than zero (sign bit is 1), branch to the effective target address after the instruction in the delay slot is executed. If the branch is not taken, the instruction in the delay slot is not executed.
	,
	"BGEZL",		 //  Branch Likely if R1 (rs) >= 0, 		offset is imm16 << 2 added to next instruction address. If the contents of GPR rs are greater than or equal to zero (sign bit is 0), branch to the effective target address after the instruction in the delay slot is executed. If the branch is not taken, the instruction in the delay slot is not executed.
	"BEQL",		 //  Branch Likely if R1 (rs) == R2 (rt),	offset is imm16 << 2 added to next instruction address. If the contents of GPR rs and GPR rt are equal, branch to the target address after the instruction in the delay slot is executed. If the branch is not taken, the instruction in the delay slot is not executed.
	"BNEL",		 //  Branch Likely if R1 (rs) != R2 (rt),	offset is imm16 << 2 added to next instruction address. If the contents of GPR rs and GPR rt are not equal, branch to the effective target address after the instruction in the delay slot is executed. If the branch is not taken, the instruction in the delay slot is not executed.
	"BLEZL",		 //  Branch Likely if R1 (rs) <= 0, 		offset is imm16 << 2 added to next instruction address. If the contents of GPR rs are less than or equal to zero (sign bit is 1 or value is zero), branch to the effective target address after the instruction in the delay slot is executed.	If the branch is not taken, the instruction in the delay slot is not executed.
	"BGTZL",		 //  Branch Likely if R1 (rs) >= 0, 		offset is imm16 << 2 added to next instruction address. If the contents of GPR rs are greater than zero (sign bit is 0 but value not zero), branch to the effective target address after the instruction in the delay slot is executed. If the branch is not taken, the instruction in the delay slot is not executed.
	
		 // --------------------------
	
	"BLTZAL"		 //  Branch Link if R1 (rs) < 0, 			offset is imm16 << 2 added to next instruction address. Place the return address link in GPR 31. The return link is the address of the second instruction following the branch (not the branch itself), where execution would continue after a procedure call. If the contents of GPR rs are less than zero (sign bit is 1), branch to the effective target address after the instruction in the delay slot is executed. GPR 31 must not be used for the source register rs
	,
	"BGEZAL",		 //  Branch Link if R1 (rs) >= 0, 		offset is imm16 << 2 added to next instruction address. Place the return address link in GPR 31. The return link is the address of the second instruction following the branch, where execution would continue after a procedure call. GPR 31 must not be used for the source register rs
	"BLTZALL" 	 //  Branch Link Likely if R1 (rs) < 0, 	offset is imm16 << 2 added to next instruction address. Place the return address link in GPR 31. The return link is the address of the second instruction following the branch (not the branch itself), where execution would continue after a procedure call. If the contents of GPR rs are less than zero (sign bit is 1), branch to the effective target address after the instruction in the delay slot is executed. GPR 31 must not be used for the source register rs
	,
	"BGEZALL",	 //  Branch Link Likely if R1 (rs) >= 0, 	offset is imm16 << 2 added to next instruction address. Place the return address link in GPR 31. If the contents of GPR rs are greater than or equal to zero (sign bit is 0), branch to the effective target address after the instruction in the delay slot is executed. If the branch	is not taken, the instruction in the delay slot is not executed. The return link is the address of the second instruction following the branch, where execution would continue after a procedure call. GPR 31 must not be used for the source register rs
	
		 // ---------------------------
	
	"SB" 			 //  memory[ R1 (base) + imm16] = R2 (rt)				The least-significant 8-bit byte of GPR rt is stored in memory at the location specified by the effective address. The 16-bit signed offset is added to the contents of GPR base to form the effective address.
	,
	"SH",			 //  memory[ R1 (base) + imm16] = R2 (rt)					The least-significant 16-bit halfword of register rt is stored in memory at the location specified by the aligned effective address. The 16-bit signed offset is added to the contents of GPR base to form the effective address.
	"SWL",		 //  memory[ R1 (base) + imm16] = partial-upper R2 (rt)				The 16-bit signed offset is added to the contents of GPR base to form an effective address (EffAddr). EffAddr is the address of the most-significant of four consecutive bytes forming a word in memory (W) starting at an arbitrary byte boundary. A part of W, the most-significant one to four bytes, is in the aligned word containing EffAddr. The same number of the most-significant (left) bytes from the word in GPR rt are stored into these bytes of W.
	"SW",			 //  memory[ R1 (base) + imm16] = R2 (rt)					The least-significant 32-bit word of register rt is stored in memory at the location specified by the aligned effective address. The 16-bit signed offset is added to the contents of GPR base to form the effective address.
	"SDL",		 //  memory[ R1 (base) + imm16] = partial-upper R2 (rt)	The 16-bit signed offset is added to the contents of GPR base to form an effective address (EffAddr). EffAddr is the address of the most-significant of eight consecutive bytes forming a doubleword in memory (DW) starting at an arbitrary byte boundary. A part of DW, the most-significant one to eight bytes, is in the aligned doubleword containing EffAddr. The same number of most-significant (left) bytes of GPR rt are stored into these bytes of DW.
	"SDR",		 //  memory[ R1 (base) + imm16] = partial-lower R2 (rt)	The 16-bit signed offset is added to the contents of GPR base to form an effective address (EffAddr). EffAddr is the address of the least-significant of eight consecutive bytes forming a doubleword in memory (DW) starting at an arbitrary byte boundary. A part of DW, the least-significant one to eight bytes, is in the aligned doubleword containing EffAddr. The same number of least-significant (right) bytes of GPR rt are stored into these bytes of DW.
	"SWR",		 //  memory[ R1 (base) + imm16] = partial-lower R2 (rt)	The 16-bit signed offset is added to the contents of GPR base to form an effective address (EffAddr). EffAddr is the address of the least-significant of four consecutive bytes forming a word in memory (W) starting at an arbitrary byte boundary. A part of W, the least-significant one to four bytes, is in the aligned word containing EffAddr. The same number of the least-significant (right) bytes from the word in GPR rt are stored into these bytes of W.
	
		 // ---------------------------
	
	"LDL" 		 //  Rd1 (rt) = Rd1 | memory[ (R1 (base) + imm16)(bytes ((base+imm)&7) to 8)]		To load the most-significant part of a doubleword from an unaligned memory address.
	,
	"LDR",		 //  Rd1 (rt) = Rd1 | memory[ (R1 (base) + imm16)(bytes (8 to ((base+imm)&7 + 8)))]
	"LB",			 //  Rd1 (rt) = memory[ R1 (base) + imm16]			The contents of the 8-bit byte at the memory location specified by the effective address are fetched, sign-extended, and placed in GPR rt. The 16-bit signed offset is added to the contents of GPR base to form the effective address.
	"LH",			 //  Rd1 (rt) = memory[ R1 (base) + imm16]			The contents of the 16-bit halfword at the memory location specified by the aligned effective address are fetched, sign-extended, and placed in GPR rt. The 16-bit signed offset is added to the contents of GPR base to form the effective address. The effective address must be naturally aligned. If the least-significant bit of the address is non-zero, an Address Error exception occurs.
	"LWL",		 //  Rd1 (rt) = Rd1 | memory[ (R1 (base) + imm16)(bytes ((base+imm)&3) to 4)]
	"LW",			 //  Rd1 (rt) = memory[ R1 (base) + imm16]			The contents of the 32-bit word at the memory location specified by the aligned effective address are fetched, sign-extended to the GPR register length if necessary, and placed in GPR rt. The 16-bit signed offset is added to the contents of GPR base to form the effective address.
	"LBU", 		 //  Rd1 (rt) = memory[ R1 (base) + imm16]			The contents of the 8-bit byte at the memory location specified by the effective address are fetched, sign-extended, and placed in GPR rt. The 16-bit signed offset is added to the contents of GPR base to form the effective address.
	"LHU",		 //  Rd1 (rt) = memory[ R1 (base) + imm16]			The contents of the 16-bit halfword at the memory location specified by the aligned effective address are fetched, zero-extended, and placed in GPR rt. The 16-bit signed offset is added to the contents of GPR base to form the effective address. The effective address must be naturally aligned. If the least-significant bit of the address is non-zero, an Address Error exception occurs.
	"LWR",		 //  Rd1 (rt) = Rd1 | memory[ (R1 (base) + imm16)(bytes ( 4 to (base+imm +4)&3))]
	"LWU",		 //  Rd1 (rt) = memory[ R1 (base) + imm16]			The contents of the 32-bit word at the memory location specified by the aligned effective address are fetched, zero-extended, and placed in GPR rt. The 16-bit signed offset is added to the contents of GPR base to form the effective address.
	
		 // ---------------------------
		 //  ARM
		 // ---------------------------
	
	"bfc",
	"bfi",
	"clz",
	"pkhbt",
	"pkhtb",
	"rbit",
	"rev",
	"rev16",
	"revsh",
	"and",			 //  R1 (Rd) = R2 (Rn) & Op2
	"b",				 //  pc = pc+<offset>
	"bl",				 //  lr = pc-4, pc = pc+<offset>
	"blx",			 //  lr = pc-4, pc = R1 (Rm)
	"bx",				 //  pc = R1 (Rm)
	"eor",
	"sub",			 //  Rd1 (Rd) = R1 - Op2
	"rsb",			 //  Rd1 (Rd) = Op2 - R1
	"add",			 //  Rd1 (Rd) = R1 (Rn) + Op2
	"adc",			 //  Rd1 (Rd) = R1 (Rn) + Op2 + Carry
	"asr",			 //  Rd1 (Rd) = R1 (Rn) >> #<imm>
	"sbc",			 //  Rd1 (Rd) = R1 (Rn) + Op2 + Carry
	"rsc",
	"tst",			 //  R1 (Rn) & Op2
	"teq",			 //  R1 (Rn) ^ Op2
	"cmp",			 //  R1 (Rn) - Op2
	"cmn",			 //  R1 (Rn) + Op2
	"orr",			 //  Rd1 (Rd) = R1 (Rn) | Op2
	"mov",			 //  Rd1 (Rd) = Op2
	"bic",			 //  Rd1 (Rd) = R1 (Rn) AND ~Op2
	"mvn",			 //  Rd1 (Rd) = ~Op2
	"ldr",			 //  Rd1 (Rt) = memory[ R2 (Rn) + R3 (Rm) ]
	"str",			 //  memory [ R2 (Rn) + R3 (Rm) ] = R1 (Rt)
	"ldrd",			 //  Rd1 (Rt), Rd2 (Rt2) = memory[ R2 (Rn) + R3 (Rm) ]
	"strd",			 //  memory [ R2 (Rn) + R3 (Rm) ] = R1 (Rt)
	"ldr",		 //  Rd1 (Rt) = memory[ R2 (Rn) + imm ]
	"str",		 //  memory [ R2 (Rn) + imm ] = R1 (Rt)
	"ldrd",		 //  Rd1 (Rt), Rd2 (Rt2) = memory[ R2 (Rn) + imm ]
	"strd",		 //  memory [ R2 (Rn) + imm ] = R1 (Rt)
	"ldm",			 //  Rmask (<registers>) = memory [ Rn ], if (W) Rn +-= count of registers  (+ if U, - if ~U)
	"stm",			 //  memory [ Rn ] = Rmask (<registers>), if (W) Rn +-= count of registers  (+ if U, - if ~U)
	
	"ldrh",			 //  Rd1 (Rd
	"strh",
	"ldrsh",
	"strsh",
	
	"ldrb",
	"strb",
	"ldrsb",
	"strsb",
	
	"mrs",
	"msr",
	
		 //  VFP Instructions
	
	"vcmp",
	"vcvt",
	"vdiv",
	"vldm",
	"vldr",
	"vmov",
	"vmrs",
	"vmsr",
	"vpop",
	"vpush",
	"vsqrt",
	"vstm32",
	"vstm64",
	
		 // ---------------------------
		 //  Next Instruction Set ...
		 // ---------------------------
	
	
	"sizeof_mips_op_t"
};
#endif
