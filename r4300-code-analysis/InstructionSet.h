/*
 * InstructionSet.h
 *
 *  Created on: 2 Jun 2014
 *      Author: rjhender
 */


#define OPS_JUMP 	0x01000
#define OPS_BRANCH 	0x02000
#define OPS_CALL 	0x04000	//BRANCH and LINK

#define OPS_LIKELY	0x10000
#define OPS_STR  	0x20000
#define OPS_LDR  	0x40000

#define STRIP(x) (x & 0xfff)

typedef enum
{
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
	//cop0,
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

	LDM = STRIP(LWU) + 1,
	SDM,
	ADC,
	RSB,
	RSC,
	BFC,
	BFI,
	PKHBT,
	PKHTB,
	RBIT,
	REV,
	REV16,
	REVSH,
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

#define REG_NOT_USED (0xffff)
#define REG_HOST_FP	 (0x100b)
#define INIT_INSTRUCTION {0,AL,0,0,0,0,REG_NOT_USED, REG_NOT_USED, REG_NOT_USED, REG_NOT_USED, 0}

typedef struct
{
	Instruction_e instruction;
	Condition_e cond;
	Shift_e shiftType;

	union{
		int32_t offset;
		int32_t immediate;
	};

	union{
	int32_t shift;
	int32_t rotate;
	};

	uint32_t Rmask;				// for arm LDM/STM

	int32_t Rd;

	union{
		int32_t Rn;				// ARMs first Input Register
		int32_t Rt; 			// MIPs first input Register
	};

	uint32_t Rs, Rm;		// Register output, input 1-3

	uint8_t I:1;
	uint8_t PR:1;
	uint8_t B:1;
	uint8_t S:1;
	uint8_t U:1;
	uint8_t W:1;
	uint8_t RESERVED:2;

} Instruction_t;

