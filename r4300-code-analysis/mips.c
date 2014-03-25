/*
 * mips.c
 *
 *  Created on: 24 Mar 2014
 *      Author: ric_rpi
 */

#include <stdio.h>
#include <stdint.h>
#include "memory.h"

#define INDEX "%08x"

#define I_OP "s%u, s%u, %d"
#define OP_I(val) \
	((val & 0x03E00000) >> 21), \
	((val & 0x001F0000) >> 16), \
	(int)(val & 0xFFFF)

// Technically these are I OPs but I want to format the output differently
#define B_OP "s%u, 0x%2x, %d => 0x%x"
#define OP_B(val) \
	((val & 0x03E00000) >> 21), \
	((val & 0x001F0000) >> 16), \
	(int)(val<<16)/(1<<16), \
	(x+(int)(val<<16)/(1<<16) * 4)		// calculate offset from current position

#define J_OP "%d (0x%08x) => 0x%08x"
#define OP_J(val) \
	((((int)(val << 6))/(1<<6))), \
	(val & 0x03FFFFFF), \
	(x+((((int)(val << 6))/ (1<<6)))*4)

#define R_OP ""
#define OP_R(val) (val)


void decode(uint32_t x, uint32_t val)
{
	uint32_t op=val>>26;
	uint32_t op2;

	switch(op)
	{
	case 0x00: //printf("special\n");
		op2=val&0x3f;

		switch(op2)
		{
		case 0x00: 	printf(INDEX "\tBLTZ   \t" B_OP "\n",x, OP_B(val)); return;	// I
		case 0x01: 	printf(INDEX "\tBGEZ   \t" B_OP "\n",x, OP_B(val)); return;	// I
		case 0x02: 	printf(INDEX "\tBLTZL\n",x); 	return;
		case 0x03: 	printf(INDEX "\tBGEZL\n",x); 	return;
		case 0x08: 	printf(INDEX "\tTGEI\n",x); 	return;
		case 0x09: 	printf(INDEX "\tTGEIU\n",x); 	return;
		case 0x0A: 	printf(INDEX "\tTLTI\n",x); 	return;
		case 0x0B: 	printf(INDEX "\tTLTIU\n",x); 	return;
		case 0x0C: 	printf(INDEX "\tTEQI\n",x); 	return;
		case 0x0E: 	printf(INDEX "\tTNEI\n",x); 	return;
		case 0x10: 	printf(INDEX "\tBLTZAL \t" B_OP "\n",x, OP_B(val)); return;	// I and link
		case 0x11: 	printf(INDEX "\tBGEZAL \t" B_OP "\n",x, OP_B(val)); return;	// I and link
		case 0x12: 	printf(INDEX "\tBLTZALL\t" B_OP "\n",x, OP_B(val)); return;	// I and link likely
		case 0x13: 	printf(INDEX "\tBGEZALL\t" B_OP "\n",x, OP_B(val)); return;	// I and link likely
		}

		break;

	case 0x02: 	printf(INDEX "\tJ      \t" J_OP "\n", x, OP_J(val)); return;	// J
	case 0x03: 	printf(INDEX "\tJAL    \t" J_OP "\n", x, OP_J(val)); return;	// J
	case 0x04: 	printf(INDEX "\tBEQ    \t" I_OP "\n", x, OP_I(val)); return;	// I
	case 0x05: 	printf(INDEX "\tBNE    \t" I_OP "\n", x, OP_I(val)); return;	// I
	case 0x06: 	printf(INDEX "\tBLEZ   \t\n", x ); return;
	case 0x07: 	printf(INDEX "\tBGTZ   \t\n", x ); return;
	case 0x08: 	printf(INDEX "\tADDI   \t" I_OP "\n", x, OP_I(val)); return;	// I
	case 0x09: 	printf(INDEX "\tADDIU  \t" I_OP "\n", x, OP_I(val)); return;	// I
	case 0x0A: 	printf(INDEX "\tSLTI   \t" I_OP "\n", x, OP_I(val)); return;	// I
	case 0x0B: 	printf(INDEX "\tSLTIU  \t" I_OP "\n", x, OP_I(val)); return;	// I
	case 0x0C: 	printf(INDEX "\tANDI   \t" I_OP "\n", x, OP_I(val)); return;	// I
	case 0x0D: 	printf(INDEX "\tORI    \t" I_OP "\n", x, OP_I(val)); return;	// I
	case 0x0E: 	printf(INDEX "\tXORI   \t" I_OP "\n", x, OP_I(val)); return;	// I
	case 0x0F: 	printf(INDEX "\tLUI    \t" I_OP "\n", x, OP_I(val)); return;	// I
	case 0x10: 	printf(INDEX "\tcop0\n",x);
		op2=(val>>21)&0x1f;
		switch(op2)
		{
		case 0x00: printf(INDEX "\tMFC0\n",x); return;
		case 0x04: printf(INDEX "\tMTC0\n",x); return;
		case 0x10: printf(INDEX "\ttlb\n",x);
		switch(val&0x3f)
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
		op2=(val>>21)&0x1f;
		switch(op2)
		{
		case 0x00: printf(INDEX "\tMFC1\n",x); return;
		case 0x01: printf(INDEX "\tDMFC1\n",x); return;
		case 0x02: printf(INDEX "\tCFC1\n",x); return;
		case 0x04: printf(INDEX "\tMTC1\n",x); return;
		case 0x05: printf(INDEX "\tDMTC1\n",x); return;
		case 0x06: printf(INDEX "\tCTC1\n",x); return;
		case 0x08: printf(INDEX "\tBC1\n",x);
			switch((val>>16)&0x3)
			{
			case 0x00: printf(INDEX "\tBC1F",x); return;
			case 0x01: printf(INDEX "\tBC1T",x); return;
			case 0x02: printf(INDEX "\tBC1FL",x); return;
			case 0x03: printf(INDEX "\tBC1TL",x); return;
			}
			break;

		case 0x10: //printf(INDEX "\tC1.S\n",x);
			switch(val&0x3f)
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
			switch(val&0x3f)
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
			switch(val&0x3f)
			{
			case 0x20: printf(INDEX "\tCVT.S.W\n",x); return;
			case 0x21: printf(INDEX "\tCVT.D.W\n",x); return;
			}
			break;

		case 0x15: //printf(INDEX "\tC1.L\n",x);
			switch(val&0x3f)
			{
			case 0x20: printf(INDEX "\tCVT.S.L\n",x); return;
			case 0x21: printf(INDEX "\tCVT.D.L\n",x); return;
			}
			break;
		}
		break;

	case 0x14: printf(INDEX "\tBEQL\n",x); return;
	case 0x15: printf(INDEX "\tBNEL\n",x); return;
	case 0x16: printf(INDEX "\tBLEZL\n",x); return;
	case 0x17: printf(INDEX "\tBGTZL\n",x); return;
	case 0x18: printf(INDEX "\tDADDI\n",x); return;
	case 0x19: printf(INDEX "\tDADDIU\n",x); return;
	case 0x1A: printf(INDEX "\tLDL\n",x); return;
	case 0x1B: printf(INDEX "\tLDR\n",x); return;
	case 0x20: printf(INDEX "\tLB\n",x); return;
	case 0x21: printf(INDEX "\tLH\n",x); return;
	case 0x22: printf(INDEX "\tLWL\n",x); return;
	case 0x23: printf(INDEX "\tLW\n",x); return;
	case 0x24: printf(INDEX "\tLBU\n",x); return;
	case 0x25: printf(INDEX "\tLHU\n",x); return;
	case 0x26: printf(INDEX "\tLWR\n",x); return;
	case 0x27: printf(INDEX "\tLWU\n",x); return;
	case 0x28: printf(INDEX "\tSB   \t" I_OP "\n", x, OP_I(val)); return;	// I
	case 0x29: printf(INDEX "\tSH   \t" I_OP "\n", x, OP_I(val)); return;	// I
	case 0x2A: printf(INDEX "\tSWL\n",x); return;
	case 0x2B: printf(INDEX "\tSW   \t" I_OP "\n", x, OP_I(val)); return;	// I
	case 0x2C: printf(INDEX "\tSDL\n",x); return;
	case 0x2D: printf(INDEX "\tSDR\n",x); return;
	case 0x2E: printf(INDEX "\tSWR\n",x); return;
	case 0x2F: printf(INDEX "\tCACHE\n",x); return;
	case 0x30: printf(INDEX "\tLL\n",x); return;
	case 0x31: printf(INDEX "\tLWC1\n",x); return;
	case 0x34: printf(INDEX "\tLLD\n",x); return;
	case 0x35: printf(INDEX "\tLDC1\n",x); return;
	case 0x37: printf(INDEX "\tLD\n",x); return;
	case 0x38: printf(INDEX "\tSC\n",x); return;
	case 0x39: printf(INDEX "\tSWC1\n",x); return;
	case 0x3C: printf(INDEX "\tSCD\n",x); return;
	case 0x3D: printf(INDEX "\tSDC1\n",x); return;
	case 0x3F: printf(INDEX "\tSD\n",x); return;
	}

	printf(INDEX "\t...\t0x%08X\n",x, val); return;
}

