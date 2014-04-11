/*
 * mips.c
 *
 *  Created on: 24 Mar 2014
 *      Author: ric_rpi
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "memory.h"
#include "mips.h"

#define INDEX "%08x"

#define I_OP "s%u, s%u, #%d"
#define OP_I(val) \
		((val & 0x03E00000) >> 21), \
		((val & 0x001F0000) >> 16), \
		(int)(val<<16)/(1<<16)

// Technically these are I OPs but I want to format the output differently
#define B_OP "r%u, s%u, #%d => 0x%08x, \traw 0x%08x"
#define OP_B(val) \
		((val & 0x03E00000) >> 21), \
		((val & 0x001F0000) >> 16), \
		(int)(val<<16)/(1<<16), \
		(x + 4 + (int)(val<<16)/(1<<16) * 4), \
		(val)// calculate offset from current position

// Technically these are I OPs but I want to format the output differently
#define BV_OP "r%u, 0x%02x, #%d => 0x%08x, \traw 0x%08x"
#define OP_BV(val) \
		((val & 0x03E00000) >> 21), \
		((val & 0x001F0000) >> 16), \
		(int)(val<<16)/(1<<16), \
		(x + 4 + (int)(val<<16)/(1<<16) * 4), \
		(val)// calculate offset from current position


#define J_OP "0x%08x // PC = 0x%08x, \traw 0x%08x"
#define OP_J(val) \
		(val & 0x03FFFFFF), \
		((x+4) & 0xF0000000) + ((val & 0x03FFFFFF)*4), \
		(val)

#define R_OP ""
#define OP_R(val) (val)

#define L_OP "r%u, [r%u, #%d]"
#define OP_L(val) \
		((val>>21)&0x1f), \
		((val>>16)&0x1f), \
		((int)(val<<16)/(1<<16))


static uint32_t mip_ops[sizeof_mips_op_t];

void count_ops(uint32_t* data, uint32_t len)
{
	memset(mip_ops, 0, sizeof_mips_op_t);

	int x;

	for(x=0;x<len;x++)
	{
		mip_ops[STRIP(ops_type(data[x]))]++;
	}
}


int32_t ops_validCodeSegment(uint32_t* puiCodeBase, uint32_t uiStart, uint32_t uiNumInstructions, uint32_t* pCodeLen, uint32_t* pJumpToAddress)
{
	int x;

	for(x=uiStart; x<uiNumInstructions; x++)
	{
		int op = ops_type(puiCodeBase[x]);

		if (INVALID == op)
		{
			if (pJumpToAddress) *pJumpToAddress = 0;
			if (pCodeLen) *pCodeLen = x + 1;
			return 0;
		}
		else if ( (op & OPS_JUMP) == OPS_JUMP)
		{
			if (pJumpToAddress) *pJumpToAddress = ops_JumpAddressOffset(puiCodeBase[x]);
			if (pCodeLen) *pCodeLen = x - uiStart + 1;
			return 1;
		}
		else if ( (op & OPS_BRANCH) == OPS_BRANCH)
		{
			if (pJumpToAddress) *pJumpToAddress = (uint32_t)(x) + ops_JumpAddressOffset(puiCodeBase[x]);
			if (pCodeLen) *pCodeLen = x - uiStart + 1;
			return 1;
		}
	}
	if (pCodeLen) *pCodeLen = uiNumInstructions + 1;

	return 0;
}

int32_t ops_JumpAddressOffset(uint32_t uiMIPSword)
{
	uint32_t op=uiMIPSword>>26;
	uint32_t op2;

	switch(op)
	{
	case 0x00: //printf("special\n");
		op2=uiMIPSword&0x3f;

		switch(op2)
		{
		case 0x00: 	return (int32_t)(uiMIPSword<<16)/(1<<16); //printf(INDEX "\tBLTZ   \t" B_OP "\n",x, OP_B(val)); return;	// I
		case 0x01: 	return (int32_t)(uiMIPSword<<16)/(1<<16); //printf(INDEX "\tBGEZ   \t" B_OP "\n",x, OP_B(val)); return;	// I
		case 0x02: 	return (int32_t)(uiMIPSword<<16)/(1<<16); //printf(INDEX "\tBLTZL\n",x); 	return;
		case 0x03: 	return (int32_t)(uiMIPSword<<16)/(1<<16); //printf(INDEX "\tBGEZL\n",x); 	return;

		//case 0x09:  return (val<<16)/(1<<16); //printf(INDEX "\tBLTZAL \t" B_OP "\n",x, OP_B(val)); return;	// I and link
		case 0x10:  return (int32_t)(uiMIPSword<<16)/(1<<16); //printf(INDEX "\tBLTZAL \t" B_OP "\n",x, OP_B(val)); return;	// I and link
		case 0x11: 	return (int32_t)(uiMIPSword<<16)/(1<<16); //printf(INDEX "\tBGEZAL \t" B_OP "\n",x, OP_B(val)); return;	// I and link
		case 0x12: 	return (int32_t)(uiMIPSword<<16)/(1<<16); //printf(INDEX "\tBLTZALL\t" B_OP "\n",x, OP_B(val)); return;	// I and link likely
		case 0x13: 	return (int32_t)(uiMIPSword<<16)/(1<<16); //printf(INDEX "\tBGEZALL\t" B_OP "\n",x, OP_B(val)); return;	// I and link likely
		}break;

	case 0x02: 	return (uiMIPSword<<6)/(1<<6); //printf(INDEX "\tJ      \t" J_OP "\n", x, OP_J(val)); return;	// J
	case 0x03: 	return (uiMIPSword<<6)/(1<<6);   //printf(INDEX "\tJAL    \t" J_OP "\n", x, OP_J(val)); return;	// J
	case 0x04: 	return (int32_t)(uiMIPSword<<16)/(1<<16); //printf(INDEX "\tBEQ    \t" I_OP "\n", x, OP_I(val)); return;	// I
	case 0x05: 	return (int32_t)(uiMIPSword<<16)/(1<<16); //printf(INDEX "\tBNE    \t" I_OP "\n", x, OP_I(val)); return;	// I
	case 0x06: 	return (int32_t)(uiMIPSword<<16)/(1<<16); //printf(INDEX "\tBLEZ   \t\n", x ); return;
	case 0x07: 	return (int32_t)(uiMIPSword<<16)/(1<<16); //printf(INDEX "\tBGTZ   \t\n", x ); return;

	case 0x14: return (int32_t)(uiMIPSword<<16)/(1<<16); //printf(INDEX "\tBEQL  \n",x); return;
	case 0x15: return (int32_t)(uiMIPSword<<16)/(1<<16); //printf(INDEX "\tBNEL  \n",x); return;
	case 0x16: return (int32_t)(uiMIPSword<<16)/(1<<16); //printf(INDEX "\tBLEZL \n",x); return;
	case 0x17: return (int32_t)(uiMIPSword<<16)/(1<<16); //printf(INDEX "\tBGTZL \n",x); return;
	}
	printf("%d invalid\n",__LINE__);
	return 0x7FFFFFF;
}


mips_op_t ops_type(uint32_t uiMIPSword)
{
	uint32_t op=uiMIPSword>>26;
	uint32_t op2;

	switch(op)
	{
	case 0x00: //printf("special\n");
		op2=uiMIPSword&0x3f;

		switch(op2)
		{
		case 0x00: 	return BLTZ;	// I
		case 0x01: 	return BGEZ;	// I
		case 0x02: 	return BLTZL;
		case 0x03: 	return BGEZL;
		case 0x08: 	return TGEI;
		case 0x09: 	return TGEIU;
		case 0x0A: 	return TLTI;
		case 0x0B: 	return TLTIU;
		case 0x0C: 	return TEQI;
		case 0x0E: 	return TNEI;
		case 0x10: 	return BLTZAL;	// I and link
		case 0x11: 	return BGEZAL;	// I and link
		case 0x12: 	return BLTZALL;	// I and link likely
		case 0x13: 	return BGEZALL;	// I and link likely
		}

		break;

	case 0x02: 	return J;	// J
	case 0x03: 	return JAL; // J
	case 0x04: 	return BEQ;	// I
	case 0x05: 	return BNE;	// I
	case 0x06: 	return BLEZ;
	case 0x07: 	return BGTZ;
	case 0x08: 	return ADDI;	// I
	case 0x09: 	return ADDIU;	// I
	case 0x0A: 	return SLTI;	// I
	case 0x0B: 	return SLTIU;	// I
	case 0x0C: 	return ANDI; 	// I
	case 0x0D: 	return ORI;	// I
	case 0x0E: 	return XORI;	// I
	case 0x0F: 	return LUI;	// I
	case 0x10: 	//return cop0\n",x);
		op2=(uiMIPSword>>21)&0x1f;
		switch(op2)
		{
		case 0x00: return MFC0;
		case 0x04: return MTC0;
		case 0x10: //return tlb;
			switch(uiMIPSword&0x3f)
			{
			case 0x01: return TLBR;
			case 0x02: return TLBWI;
			case 0x06: return TLBWR;
			case 0x08: return TLBP;
			case 0x18: return ERET;
			}
		}
		break;

	case 0x11: //return cop1\n",x);
		op2=(uiMIPSword>>21)&0x1f;
		switch(op2)
		{
		case 0x00: return MFC1;
		case 0x01: return DMFC1;
		case 0x02: return CFC1;
		case 0x04: return MTC1;
		case 0x05: return DMTC1;
		case 0x06: return CTC1;
		case 0x08: //return BC1;
			switch((uiMIPSword>>16)&0x3)
			{
			case 0x00: return BC1F;
			case 0x01: return BC1T;
			case 0x02: return BC1FL;
			case 0x03: return BC1TL;
			}break;

		case 0x10: //return C1.S\n",x);
			switch(uiMIPSword&0x3f)
			{
			case 0x00: return ADD_S;
			case 0x01: return SUB_S;
			case 0x02: return MUL_S;
			case 0x03: return DIV_S;
			case 0x04: return SQRT_S;
			case 0x05: return ABS_S;
			case 0x06: return MOV_S;
			case 0x07: return NEG_S;
			case 0x08: return ROUND_L_S;
			case 0x09: return TRUNC_L_S;
			case 0x0A: return CEIL_L_S;
			case 0x0B: return FLOOR_L_S;
			case 0x0C: return ROUND_W_S;
			case 0x0D: return TRUNC_W_S;
			case 0x0E: return CEIL_W_S;
			case 0x0F: return FLOOR_W_S;
			case 0x21: return CVT_D_S;
			case 0x24: return CVT_W_S;
			case 0x25: return CVT_L_S;
			case 0x30: return C_F_S;
			case 0x31: return C_UN_S;
			case 0x32: return C_EQ_S;
			case 0x33: return C_UEQ_S;
			case 0x34: return C_OLT_S;
			case 0x35: return C_ULT_S;
			case 0x36: return C_OLE_S;
			case 0x37: return C_ULE_S;
			case 0x38: return C_SF_S;
			case 0x39: return C_NGLE_S;
			case 0x3A: return C_SEQ_S;
			case 0x3B: return C_NGL_S;
			case 0x3C: return C_LT_S;
			case 0x3D: return C_NGE_S;
			case 0x3E: return C_LE_S;
			case 0x3F: return C_NGT_S;
			}break;
		case 0x11: //return C1_D\n",x);
			switch(uiMIPSword&0x3f)
			{
			case 0x00: return ADD_D;
			case 0x01: return SUB_D;
			case 0x02: return MUL_D;
			case 0x03: return DIV_D;
			case 0x04: return SQRT_D;
			case 0x05: return ABS_D;
			case 0x06: return MOV_D;
			case 0x07: return NEG_D;
			case 0x08: return ROUND_L_D;
			case 0x09: return TRUNC_L_D;
			case 0x0A: return CEIL_L_D;
			case 0x0B: return FLOOR_L_D;
			case 0x0C: return ROUND_W_D;
			case 0x0D: return TRUNC_W_D;
			case 0x0E: return CEIL_W_D;
			case 0x0F: return FLOOR_W_D;
			case 0x20: return CVT_S_D;
			case 0x24: return CVT_W_D;
			case 0x25: return CVT_L_D;
			case 0x30: return C_F_D;
			case 0x31: return C_UN_D;
			case 0x32: return C_EQ_D;
			case 0x33: return C_UEQ_D;
			case 0x34: return C_OLT_D;
			case 0x35: return C_ULT_D;
			case 0x36: return C_OLE_D;
			case 0x37: return C_ULE_D;
			case 0x38: return C_SF_D;
			case 0x39: return C_NGLE_D;
			case 0x3A: return C_SEQ_D;
			case 0x3B: return C_NGL_D;
			case 0x3C: return C_LT_D;
			case 0x3D: return C_NGE_D;
			case 0x3E: return C_LE_D;
			case 0x3F: return C_NGT_D;
			} break;
		case 0x14: //return C1_W\n",x);
			switch(uiMIPSword&0x3f)
			{
			case 0x20: return CVT_S_W;
			case 0x21: return CVT_D_W;
			}
			break;

		case 0x15: //return C1_L\n",x);
			switch(uiMIPSword&0x3f)
			{
			case 0x20: return CVT_S_L;
			case 0x21: return CVT_D_L;
			}
			break;
		}break;

	case 0x14: return BEQL;
	case 0x15: return BNEL;
	case 0x16: return BLEZL;
	case 0x17: return BGTZL;
	case 0x18: return DADDI;
	case 0x19: return DADDIU;
	case 0x1A: return LDL;
	case 0x1B: return LDR;
	case 0x20: return LB;	// Load Byte
	case 0x21: return LH;	// Load Halfword
	case 0x22: return LWL;
	case 0x23: return LW;	// Load Word
	case 0x24: return LBU;	// Load Unsigned Byte
	case 0x25: return LHU;	// Load Halfword unsigned
	case 0x26: return LWR;
	case 0x27: return LWU;	// Load Word unsigned
	case 0x28: return SB;	// I
	case 0x29: return SH;	// I
	case 0x2A: return SWL;
	case 0x2B: return SW;	// I
	case 0x2C: return SDL;
	case 0x2D: return SDR;
	case 0x2E: return SWR;
	case 0x2F: return CACHE;
	case 0x30: return LL;	// Load Linked Word atomic Read-Modify-Write ops
	case 0x31: return LWC1;	// Load Word to co processor 1
	case 0x34: return LLD;	// Load Linked Dbl Word atomic Read-Modify-Write ops
	case 0x35: return LDC1;
	case 0x37: return LD; 	// Load Double word
	case 0x38: return SC;	// Store Linked Word atomic Read-Modify-Write ops
	case 0x39: return SWC1;	// Store Word from co processor 1 to memory
	case 0x3C: return SCD;	// Store Conditional Double Word
	case 0x3D: return SDC1;
	case 0x3F: return SD; 	// Store Double word
	}

	return INVALID;
}

void ops_decode(uint32_t x, uint32_t uiMIPSword)
{
	uint32_t op=uiMIPSword>>26;
	uint32_t op2;

	switch(op)
	{
	case 0x00: //printf("special\n");
		op2=uiMIPSword&0x3f;

		switch(op2)
		{
		case 0x00: 	printf(INDEX "\tBLTZ   \t" BV_OP "\n", x, OP_BV(uiMIPSword)); return;	// I
		case 0x01: 	printf(INDEX "\tBGEZ   \t" BV_OP "\n", x, OP_BV(uiMIPSword)); return;	// I
		case 0x02: 	printf(INDEX "\tBLTZL  \t" BV_OP "\n", x, OP_BV(uiMIPSword)); return;	// I
		case 0x03: 	printf(INDEX "\tBGEZL  \t" BV_OP "\n", x, OP_BV(uiMIPSword)); return;	// I
		case 0x08: 	printf(INDEX "\tTGEI\n",x); 	return;
		case 0x09: 	printf(INDEX "\tTGEIU\n",x); 	return;
		case 0x0A: 	printf(INDEX "\tTLTI\n",x); 	return;
		case 0x0B: 	printf(INDEX "\tTLTIU\n",x); 	return;
		case 0x0C: 	printf(INDEX "\tTEQI\n",x); 	return;
		case 0x0E: 	printf(INDEX "\tTNEI\n",x); 	return;
		case 0x10: 	printf(INDEX "\tBLTZAL \t" BV_OP "\n", x, OP_BV(uiMIPSword)); return;	// I and link
		case 0x11: 	printf(INDEX "\tBGEZAL \t" BV_OP "\n", x, OP_BV(uiMIPSword)); return;	// I and link
		case 0x12: 	printf(INDEX "\tBLTZALL\t" BV_OP "\n", x, OP_BV(uiMIPSword)); return;	// I and link likely
		case 0x13: 	printf(INDEX "\tBGEZALL\t" BV_OP "\n", x, OP_BV(uiMIPSword)); return;	// I and link likely
		}

		break;

	case 0x02: 	printf(INDEX "\tJ      \t" J_OP "\n", x, OP_J(uiMIPSword)); return;	// J
	case 0x03: 	printf(INDEX "\tJAL    \t" J_OP "\n", x, OP_J(uiMIPSword)); return;	// J
	case 0x04: 	printf(INDEX "\tBEQ    \t" B_OP "\n", x, OP_B(uiMIPSword)); return;	// I
	case 0x05: 	printf(INDEX "\tBNE    \t" B_OP "\n", x, OP_B(uiMIPSword)); return;	// I
	case 0x06: 	printf(INDEX "\tBLEZ   \t" BV_OP "\n", x, OP_BV(uiMIPSword)); return;	// I
	case 0x07: 	printf(INDEX "\tBGTZ   \t" BV_OP "\n", x, OP_BV(uiMIPSword)); return;	// I
	case 0x08: 	printf(INDEX "\tADDI   \t" I_OP "\n", x, OP_I(uiMIPSword)); return;	// I
	case 0x09: 	printf(INDEX "\tADDIU  \t" I_OP "\n", x, OP_I(uiMIPSword)); return;	// I
	case 0x0A: 	printf(INDEX "\tSLTI   \t" I_OP "\n", x, OP_I(uiMIPSword)); return;	// I
	case 0x0B: 	printf(INDEX "\tSLTIU  \t" I_OP "\n", x, OP_I(uiMIPSword)); return;	// I
	case 0x0C: 	printf(INDEX "\tANDI   \t" I_OP "\n", x, OP_I(uiMIPSword)); return;	// I
	case 0x0D: 	printf(INDEX "\tORI    \t" I_OP "\n", x, OP_I(uiMIPSword)); return;	// I
	case 0x0E: 	printf(INDEX "\tXORI   \t" I_OP "\n", x, OP_I(uiMIPSword)); return;	// I
	case 0x0F: 	printf(INDEX "\tLUI    \t" I_OP "\n", x, OP_I(uiMIPSword)); return;	// I
	case 0x10: 	//printf(INDEX "\tcop0\n",x);
		op2=(uiMIPSword>>21)&0x1f;
		switch(op2)
		{
		case 0x00: printf(INDEX "\tMFC0\n",x); return;
		case 0x04: printf(INDEX "\tMTC0\n",x); return;
		case 0x10: printf(INDEX "\ttlb\n",x);
			switch(uiMIPSword&0x3f)
			{
			case 0x01: printf(INDEX "\tTLBR\n",x); return;
			case 0x02: printf(INDEX "\tTLBWI\n",x); return;
			case 0x06: printf(INDEX "\tTLBWR\n",x); return;
			case 0x08: printf(INDEX "\tTLBP\n",x); return;
			case 0x18: printf(INDEX "\tERET\n",x); return;
			}
		}
		break;

	case 0x11: //printf(INDEX "\tcop1\n",x);
		op2=(uiMIPSword>>21)&0x1f;
		switch(op2)
		{
		case 0x00: printf(INDEX "\tMFC1\n",x); return;
		case 0x01: printf(INDEX "\tDMFC1\n",x); return;
		case 0x02: printf(INDEX "\tCFC1\n",x); return;
		case 0x04: printf(INDEX "\tMTC1\n",x); return;
		case 0x05: printf(INDEX "\tDMTC1\n",x); return;
		case 0x06: printf(INDEX "\tCTC1\n",x); return;
		case 0x08: printf(INDEX "\tBC1\n",x);
			switch((uiMIPSword>>16)&0x3)
			{
			case 0x00: printf(INDEX "\tBC1F",x); return;
			case 0x01: printf(INDEX "\tBC1T",x); return;
			case 0x02: printf(INDEX "\tBC1FL",x); return;
			case 0x03: printf(INDEX "\tBC1TL",x); return;
			}break;

		case 0x10: //printf(INDEX "\tC1.S\n",x);
			switch(uiMIPSword&0x3f)
			{
			case 0x00: printf(INDEX "\tADD.S\n",x); return;
			case 0x01: printf(INDEX "\tSUB.S\n",x); return;
			case 0x02: printf(INDEX "\tMUL.S\n",x); return;
			case 0x03: printf(INDEX "\tDIV.S\n",x); return;
			case 0x04: printf(INDEX "\tSQRT.S\n",x); return;
			case 0x05: printf(INDEX "\tABS.S\n",x); return;
			case 0x06: printf(INDEX "\tMOV.S\n",x); return;
			case 0x07: printf(INDEX "\tNEG.S\n",x); return;
			case 0x08: printf(INDEX "\tROUND.L.S\n",x); return;
			case 0x09: printf(INDEX "\tTRUNC.L.S\n",x); return;
			case 0x0A: printf(INDEX "\tCEIL.L.S\n",x); return;
			case 0x0B: printf(INDEX "\tFLOOR.L.S\n",x); return;
			case 0x0C: printf(INDEX "\tROUND.W.S\n",x); return;
			case 0x0D: printf(INDEX "\tTRUNC.W.S\n",x); return;
			case 0x0E: printf(INDEX "\tCEIL.W.S\n",x); return;
			case 0x0F: printf(INDEX "\tFLOOR.W.S\n",x); return;
			case 0x21: printf(INDEX "\tCVT.D.S\n",x); return;
			case 0x24: printf(INDEX "\tCVT.W.S\n",x); return;
			case 0x25: printf(INDEX "\tCVT.L.S\n",x); return;
			case 0x30: printf(INDEX "\tC.F.S\n",x); return;
			case 0x31: printf(INDEX "\tC.UN.S\n",x); return;
			case 0x32: printf(INDEX "\tC.EQ.S\n",x); return;
			case 0x33: printf(INDEX "\tC.UEQ.S\n",x); return;
			case 0x34: printf(INDEX "\tC.OLT.S\n",x); return;
			case 0x35: printf(INDEX "\tC.ULT.S\n",x); return;
			case 0x36: printf(INDEX "\tC.OLE.S\n",x); return;
			case 0x37: printf(INDEX "\tC.ULE.S\n",x); return;
			case 0x38: printf(INDEX "\tC.SF.S\n",x); return;
			case 0x39: printf(INDEX "\tC.NGLE.S\n",x); return;
			case 0x3A: printf(INDEX "\tC.SEQ.S\n",x); return;
			case 0x3B: printf(INDEX "\tC.NGL.S\n",x); return;
			case 0x3C: printf(INDEX "\tC.LT.S\n",x); return;
			case 0x3D: printf(INDEX "\tC.NGE.S\n",x); return;
			case 0x3E: printf(INDEX "\tC.LE.S\n",x); return;
			case 0x3F: printf(INDEX "\tC.NGT.S\n",x); return;
			}
			break;
		case 0x11: //printf(INDEX "\tC1.D\n",x);
			switch(uiMIPSword&0x3f)
			{
			case 0x00: printf(INDEX "\tADD.D\n",x); return;
			case 0x01: printf(INDEX "\tSUB.D\n",x); return;
			case 0x02: printf(INDEX "\tMUL.D\n",x); return;
			case 0x03: printf(INDEX "\tDIV.D\n",x); return;
			case 0x04: printf(INDEX "\tSQRT.D\n",x); return;
			case 0x05: printf(INDEX "\tABS.D\n",x); return;
			case 0x06: printf(INDEX "\tMOV.D\n",x); return;
			case 0x07: printf(INDEX "\tNEG.D\n",x); return;
			case 0x08: printf(INDEX "\tROUND.L.D\n",x); return;
			case 0x09: printf(INDEX "\tTRUNC.L.D\n",x); return;
			case 0x0A: printf(INDEX "\tCEIL.L.D\n",x); return;
			case 0x0B: printf(INDEX "\tFLOOR.L.D\n",x); return;
			case 0x0C: printf(INDEX "\tROUND.W.D\n",x); return;
			case 0x0D: printf(INDEX "\tTRUNC.W.D\n",x); return;
			case 0x0E: printf(INDEX "\tCEIL.W.D\n",x); return;
			case 0x0F: printf(INDEX "\tFLOOR.W.D\n",x); return;
			case 0x20: printf(INDEX "\tCVT.S.D\n",x); return;
			case 0x24: printf(INDEX "\tCVT.W.D\n",x); return;
			case 0x25: printf(INDEX "\tCVT.L.D\n",x); return;
			case 0x30: printf(INDEX "\tC.F.D\n",x); return;
			case 0x31: printf(INDEX "\tC.UN.D\n",x); return;
			case 0x32: printf(INDEX "\tC.EQ.D\n",x); return;
			case 0x33: printf(INDEX "\tC.UEQ.D\n",x); return;
			case 0x34: printf(INDEX "\tC.OLT.D\n",x); return;
			case 0x35: printf(INDEX "\tC.ULT.D\n",x); return;
			case 0x36: printf(INDEX "\tC.OLE.D\n",x); return;
			case 0x37: printf(INDEX "\tC.ULE.D\n",x); return;
			case 0x38: printf(INDEX "\tC.SF.D\n",x); return;
			case 0x39: printf(INDEX "\tC.NGLE.D\n",x); return;
			case 0x3A: printf(INDEX "\tC.SEQ.D\n",x); return;
			case 0x3B: printf(INDEX "\tC.NGL.D\n",x); return;
			case 0x3C: printf(INDEX "\tC.LT.D\n",x); return;
			case 0x3D: printf(INDEX "\tC.NGE.D\n",x); return;
			case 0x3E: printf(INDEX "\tC.LE.D\n",x); return;
			case 0x3F: printf(INDEX "\tC.NGT.D\n",x); return;
			}
			break;
		case 0x14: //printf(INDEX "\tC1.W\n",x);
			switch(uiMIPSword&0x3f)
			{
			case 0x20: printf(INDEX "\tCVT.S.W\n",x); return;
			case 0x21: printf(INDEX "\tCVT.D.W\n",x); return;
			}
			break;

		case 0x15: //printf(INDEX "\tC1.L\n",x);
			switch(uiMIPSword&0x3f)
			{
			case 0x20: printf(INDEX "\tCVT.S.L\n",x); return;
			case 0x21: printf(INDEX "\tCVT.D.L\n",x); return;
			}
			break;
		}
		break;

	case 0x14: printf(INDEX "\tBEQL  \n",x); return;
	case 0x15: printf(INDEX "\tBNEL  \n",x); return;
	case 0x16: printf(INDEX "\tBLEZL \n",x); return;
	case 0x17: printf(INDEX "\tBGTZL \n",x); return;
	case 0x18: printf(INDEX "\tDADDI \n",x); return;
	case 0x19: printf(INDEX "\tDADDIU\n",x); return;
	case 0x1A: printf(INDEX "\tLDL   \n",x); return;
	case 0x1B: printf(INDEX "\tLDR   \n",x); return;
	case 0x20: printf(INDEX "\tLB    \t" L_OP "\n", x, OP_L(uiMIPSword)); return;	// Load Byte
	case 0x21: printf(INDEX "\tLH    \t" L_OP "\n", x, OP_L(uiMIPSword)); return;	// Load Halfword
	case 0x22: printf(INDEX "\tLWL   \n",x); return;
	case 0x23: printf(INDEX "\tLW    \t" L_OP "\n", x, OP_L(uiMIPSword)); return;	// Load Word
	case 0x24: printf(INDEX "\tLBU   \t" L_OP "\n", x, OP_L(uiMIPSword)); return;	// Load Unsigned Byte
	case 0x25: printf(INDEX "\tLHU   \t" L_OP "\n", x, OP_L(uiMIPSword)); return;	// Load Halfword unsigned
	case 0x26: printf(INDEX "\tLWR   \n",x); return;
	case 0x27: printf(INDEX "\tLWU   \t" L_OP "\n", x, OP_L(uiMIPSword)); return;	// Load Word unsigned
	case 0x28: printf(INDEX "\tSB    \t" I_OP "\n", x, OP_I(uiMIPSword)); return;	// I
	case 0x29: printf(INDEX "\tSH    \t" I_OP "\n", x, OP_I(uiMIPSword)); return;	// I
	case 0x2A: printf(INDEX "\tSWL   \n",x); return;
	case 0x2B: printf(INDEX "\tSW    \t" I_OP "\n", x, OP_I(uiMIPSword)); return;	// I
	case 0x2C: printf(INDEX "\tSDL   \n",x); return;
	case 0x2D: printf(INDEX "\tSDR   \n",x); return;
	case 0x2E: printf(INDEX "\tSWR   \n",x); return;
	case 0x2F: printf(INDEX "\tCACHE \n",x); return;
	case 0x30: printf(INDEX "\tLL    \t" L_OP "\n", x, OP_L(uiMIPSword)); return;	// Load Linked Word atomic Read-Modify-Write ops
	case 0x31: printf(INDEX "\tLWC1  \t" L_OP "\n", x, OP_L(uiMIPSword)); return;	// Load Word to co processor 1
	case 0x34: printf(INDEX "\tLLD   \t" L_OP "\n", x, OP_L(uiMIPSword)); return;	// Load Linked Dbl Word atomic Read-Modify-Write ops
	case 0x35: printf(INDEX "\tLDC1  \n",x); return;
	case 0x37: printf(INDEX "\tLD    \t" L_OP "\n", x, OP_L(uiMIPSword)); return; 	// Load Double word
	case 0x38: printf(INDEX "\tSC    \t" L_OP "\n", x, OP_L(uiMIPSword)); return;	// Store Linked Word atomic Read-Modify-Write ops
	case 0x39: printf(INDEX "\tSWC1  \t" L_OP "\n", x, OP_L(uiMIPSword)); return;	// Store Word from co processor 1 to memory
	case 0x3C: printf(INDEX "\tSCD   \t" L_OP "\n", x, OP_L(uiMIPSword)); return;	// Store Conditional Double Word
	case 0x3D: printf(INDEX "\tSDC1  \n",x); return;
	case 0x3F: printf(INDEX "\tSD    \t" L_OP "\n", x, OP_L(uiMIPSword)); return; 	// Store Double word
	}

	printf(INDEX "\t...\t0x%08X\n",x, uiMIPSword); return;
}

