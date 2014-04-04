/*
 * mips_h
 *
 *  Created on: 24 Mar 2014
 *      Author: ric_rpr
 */

#ifndef MIPS_H_
#define MIPS_H_

#include <stdint.h>

#define OPS_JUMP 0x1000
#define OPS_BRANCH 0x2000
#define OPS_STR  0x4000
#define OPS_LDR  0x8000

#define STRIP(x) (x & 0xfff)

typedef struct _code_seg_t
{
	struct _code_seg_t* pNext_code_seg;
	uint32_t pCodeStart, pBranch;
	uint32_t uiNumInstructions;
} code_seg_t;


typedef enum {
	INVALID,
	TGEI,
	TGEIU,
	TLTI,
	TLTIU,
	TEQI,
	TNEI,
	ADDI,
	ADDIU,
	SLTI,
	SLTIU,
	ANDI,
	ORI,
	XORI,
	LUI,
	cop0,
	MFC0,
	MTC0,
	tlb,
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
	DADDI,
	DADDIU,
	CACHE,
	LL,
	LWC1,
	LLD,
	LDC1,
	LD,
	SC,
	SWC1,
	SCD,
	SDC1,
	SD,

	//---------------------------
	J = OPS_JUMP + STRIP(SD)+1,
	JAL= OPS_BRANCH + STRIP(J)+1,
	BLTZ ,
	BGEZ,
	BLTZL,
	BGEZL,
	BEQ,
	BNE,
	BLEZ,
	BGTZ,
	BLTZAL,
	BGEZAL,
	BLTZALL,
	BGEZALL,
	BEQL,
	BNEL,
	BLEZL,
	BGTZL,

	//---------------------------

	SB = OPS_STR + STRIP(BGTZL)+1,
	SH,
	SWL,
	SW,
	SDL,
	SDR,
	SWR,

	//---------------------------

	LDL = OPS_LDR + STRIP(SWR) + 1,
	LDR,
	LB,
	LH,
	LWL,
	LW,
	LBU,
	LHU,
	LWR,
	LWU,
	sizeof_mips_op_t = STRIP(LWU) +1
} mips_op_t;

/*
 * Provides the length of the code segment starting at uiMIPSword.
 * It will only scan up to uiNumInstructions. if it fails to jump within
 * uinumInstructions then the function will report the segment as invalid.
 *
 * On hitting a branch or jump it will stop searching and return the instruction count.
 *
 * if pJumpToAddress is provided then it will be populated with the address the
 * branch or jump would go to.
 */
int32_t ops_validCodeSegment(uint32_t* uiMIPSword,
		uint32_t uiNumInstructions,
		uint32_t* pJumpToAddress);

/*
 * Provides the registers used in an instruction in the form:
 * 	(reg_out1 << 24 | reg_out2 << 16 | reg_in1 << 8 | reg_in2)
 *
 * 	Note: This function will not indicate if the word is a valid instruction
 */
int32_t ops_regs_used(uint32_t uiMIPSword);

/*
 * Converts a raw word into an enumeration type
 */
mips_op_t ops_type(uint32_t uiMIPSword);

/*
 * Return the Jump/Branch instruction offset for the raw word provided
 *
 * Return:
 * 		offset or 0x7FFFFFF if the instruction is not a Jump nor Branch
 * */
int32_t ops_JumpAddressOffset(uint32_t uiMIPSword);

/*
 * Provides printf() output for the raw word
 */
void ops_decode(uint32_t x, uint32_t uiMIPSword);

#endif /* MIPS_H_ */
