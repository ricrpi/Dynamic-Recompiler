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

#define OPS_JUMP 	0x01000
#define OPS_BRANCH 	0x02000
#define OPS_CALL 	0x04000	//BRANCH and LINK

#define OPS_LIKELY	0x10000
#define OPS_STR  	0x20000
#define OPS_LDR  	0x40000

#define STRIP(x) (x & 0xfff)


#define FP_REG_OFFSET	64
#define C0_REG_OFFSET	128

#define REG_HOST_FP	 (0x400a)
#define REG_HOST_SP	 (0x400b)
#define REG_HOST_LR	 (0x400c)
#define REG_HOST_PC	 (0x400d)

#define REG_HOST_STM_FP (0x1000)
#define REG_HOST_STM_SP (0x2000)
#define REG_HOST_STM_LR (0x4000)
#define REG_HOST_STM_PC (0x8000)

#define REG_PC       (0x4010)
#define REG_FCR0	 (0x4011)
#define REG_FCR31    (0x4012)
#define REG_MULTHI   (0x4013)
#define REG_MULTLO   (0x4014)
#define REG_LLBIT    (0x4015)


#define REG_CONTEXT  (C0_REG_OFFSET + 4)
#define REG_BADVADDR (C0_REG_OFFSET + 8)
#define REG_COUNT    (C0_REG_OFFSET + 9)
#define REG_ENTRYHI  (C0_REG_OFFSET + 10)
#define REG_COMPARE  (C0_REG_OFFSET + 11)
#define REG_STATUS   (C0_REG_OFFSET + 12)
#define REG_CAUSE    (C0_REG_OFFSET + 13)
#define REG_EPC		 (C0_REG_OFFSET + 14)

#define REG_NOT_USED (0xffff)


//#define INIT_INSTRUCTION {0, 0,AL,0,0,0,0,REG_NOT_USED, REG_NOT_USED, REG_NOT_USED, REG_NOT_USED, 0}

typedef int16_t reg_t;

typedef enum
{
	UNKNOWN,
	IGNORE,
	INVALID,

	SLL,
    SRL,
    SRA,
    SLLV,
    SRLV,
    SRAV,
    SYSCALL,
    BREAK,
    SYNC,
    MFHI,
    MTHI,
    MFLO,
    MTLO,
    DSLLV,
    DSRLV,
    DSRAV,
    MULT,
    MULTU,
    DIV,
    DIVU,
    DMULT,
    DMULTU,
    DDIV,
    DDIVU,
    ADD,
    ADDU,
    SUB,
    SUBU,
    AND,
    OR,
    XOR,
    NOR,
    SLT,
    SLTU,
    DADD,
    DADDU,
    DSUB,
    DSUBU,
    TGE,
    TGEU,
    TLT,
    TLTU,
    TEQ,
    TNE,
    DSLL,
    DSRL,
    DSRA,
    DSLL32,
    DSRL32,
    DSRA32,
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
	J = OPS_JUMP + STRIP(SD) + 1,
	JR,

	//---------------------------

	BLTZ = OPS_BRANCH + STRIP(JR) + 1,
	BGEZ,
	BEQ,
	BNE,
	BLEZ,
	BGTZ,

	BLTZL = OPS_BRANCH + OPS_LIKELY + STRIP(BGTZ) + 1,
	BGEZL,
	BEQL,
	BNEL,
	BLEZL,
	BGTZL,

	//--------------------------

	JAL= OPS_CALL + STRIP(BGTZL) + 1,
	JALR,
	BLTZAL,
	BGEZAL,

	BLTZALL = OPS_CALL + OPS_LIKELY + STRIP(BGEZAL)+1,
	BGEZALL,

	//---------------------------

	SB = OPS_STR + STRIP(BGEZALL) + 1,
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
	ARM_LDM,
	ARM_STM,
	sizeof_mips_op_t
} Instruction_e;

static const char* Instruction_ascii[sizeof_mips_op_t+1] =
{
	"UNKNOWN",
	"IGNORE",
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
	"ldm",
	"stm",
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
	EQ,
	NE,
	CS,
	CC,
	MI,
	PL,
	VS,
	VC,
	HI,
	LS,
	GE,
	LT,
	GT,
	LE,
	AL,
	NV
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

Instruction_t* newInstr();

#endif
