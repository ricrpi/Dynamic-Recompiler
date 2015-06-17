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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "memory.h"
#include "InstructionSetMIPS4.h"
#include "DebugDefines.h"

#define INDEX "0x%08x"

#define I_OP "s%02u, s%02u, #%02d"
#define OP_I(val) \
		((val & 0x001F0000) >> 16), \
		((val & 0x03E00000) >> 21), \
		(int)(val<<16)/(1<<16)

#define OP_UI(val) \
		((val & 0x001F0000) >> 16), \
		((val & 0x03E00000) >> 21), \
		(int)(val&0xffff)


// Technically these are I OPs but I want to format the output differently
#define B_OP "r%02u, s%02u, #%02d\t\t// branch to 0x%08x"
#define OP_B(val) \
		((val & 0x03E00000) >> 21), \
		((val & 0x001F0000) >> 16), \
		(int)(val<<16)/(1<<16), \
		(x + 4 + (int)(val<<16)/(1<<16) * 4) // calculate offset from current position

// Technically these are I OPs but I want to format the output differently
#define BV_OP "r%02u, 0x%02x, #%02d => 0x%08x, \traw 0x%08x"
#define OP_BV(val) \
		((val & 0x03E00000) >> 21), \
		((val & 0x001F0000) >> 16), \
		(int)(val<<16)/(1<<16), \
		(x + 4 + (int)(val<<16)/(1<<16) * 4), \
		(val)// calculate offset from current position

#define J_OP "0x%08x\t\t// val<<2 = 0x%08x, offset = %d"
#define OP_J(x, val) \
		(((x&0xf0000000)|(val<<2)) & 0x0FFFFFFF), \
		((val << 2) & 0x0FFFFFFF), \
		((((x&0xf0000000)|(val<<2)) & 0x0FFFFFFF) - x)

#define R_OP "r%02d, r%02d, r%02d"
#define OP_R(val) (val>>11)&0x1f, (val>>21)&0x1f, (val>>16)&0x1f

#define R2_OP "r%02d, r%02d"
#define OP_R2(val) (val>>21)&0x1f, (val>>16)&0x1f


#define L_OP "r%02u, [r%02u, #%02d]"
#define OP_L(val) \
		((val>>16)&0x1f), \
		((val>>21)&0x1f), \
		((int)(val<<16)/(1<<16))

int32_t ops_BranchOffset(const uint32_t* const instruction)
{
	uint32_t uiMIPSword = *instruction;
	uint32_t op=uiMIPSword>>26;
	uint32_t op2;

	switch(op)
	{
	case 0x01:
		op2=(uiMIPSword>>16)&0x1f;
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

	case 0x04: 	return (int32_t)(uiMIPSword<<16)/(1<<16); //printf(INDEX "\tBEQ    \t" I_OP "\n", x, OP_I(val)); return;	// I
	case 0x05: 	return (int32_t)(uiMIPSword<<16)/(1<<16); //printf(INDEX "\tBNE    \t" I_OP "\n", x, OP_I(val)); return;	// I
	case 0x06: 	return (int32_t)(uiMIPSword<<16)/(1<<16); //printf(INDEX "\tBLEZ   \t\n", x ); return;
	case 0x07: 	return (int32_t)(uiMIPSword<<16)/(1<<16); //printf(INDEX "\tBGTZ   \t\n", x ); return;
	case 0x11: //*uiCPUregs = cop1\n",x);
		op2=(uiMIPSword>>21)&0x1f;
		switch(op2)
		{
		case 0x08: //*uiCPUregs = BC1;
			switch((uiMIPSword>>16)&0x3)
			{
			case 0x00: return (int32_t)(uiMIPSword<<16)/(1<<16); // BC1F; return 0;
			case 0x01: return (int32_t)(uiMIPSword<<16)/(1<<16); // BC1T; return 0;
			case 0x02: return (int32_t)(uiMIPSword<<16)/(1<<16); // BC1FL; return 0;
			case 0x03: return (int32_t)(uiMIPSword<<16)/(1<<16); // BC1TL; return 0;
			}break;
		}break;
	case 0x14: return (int32_t)(uiMIPSword<<16)/(1<<16); //printf(INDEX "\tBEQL  \n",x); return;
	case 0x15: return (int32_t)(uiMIPSword<<16)/(1<<16); //printf(INDEX "\tBNEL  \n",x); return;
	case 0x16: return (int32_t)(uiMIPSword<<16)/(1<<16); //printf(INDEX "\tBLEZL \n",x); return;
	case 0x17: return (int32_t)(uiMIPSword<<16)/(1<<16); //printf(INDEX "\tBGTZL \n",x); return;
	}
	printf("InstructionSetMIPS4.h:%d invalid ops_JumpAddressOffset() instruction %d 0x%08x\n",__LINE__, uiMIPSword, uiMIPSword);
	return 0x7FFFFFF;
}

uint32_t ops_JumpAddress(const uint32_t* const instruction)
{
	uint32_t uiMIPSword = *instruction;
	uint32_t op=uiMIPSword>>26;
	uint32_t op2;

	switch(op)
	{
	case 0x00: //printf("special\n");
		op2=uiMIPSword&0x3f;

		switch(op2)
		{
		case 0x08: return 0; // JR; 	//cannot return whats in a register
		case 0x09: return 0; // JALR;	//cannot return whats in a register
		} break;

	case 0x02: 	//printf(INDEX "\tJ      \t" J_OP "\n", x, OP_J(val)); return;	// J
	case 0x03: 	return ((uiMIPSword)&0x03FFFFFF)*4;   //printf(INDEX "\tJAL    \t" J_OP "\n", x, OP_J(val)); return;	// J

	}
	printf("InstructionSetMIPS4.h:%d invalid ops_JumpAddress() instruction %d 0x%08x\n",__LINE__, uiMIPSword, uiMIPSword);
	return 0xFFFFFFFF;
}


uint32_t ops_regs_output(const uint32_t uiMIPSword, uint32_t * const puiCPUregs_out, uint32_t * const puiFPUregs_out, uint32_t * const puiSpecialregs_out)
{
	uint32_t op=uiMIPSword>>26;
	uint32_t op2;

	switch(op)
	{
		case 0x00:
			op2=uiMIPSword&0x3f;
			switch(op2)
			{
				case 0x00: *puiCPUregs_out |= MIPS_REG((uiMIPSword>>11)&0x1f); return 0; // SLL
				case 0x02: *puiCPUregs_out |= MIPS_REG((uiMIPSword>>11)&0x1f); return 0; // SRL
				case 0x03: *puiCPUregs_out |= MIPS_REG((uiMIPSword>>11)&0x1f); return 0; // SRA
				case 0x04: *puiCPUregs_out |= MIPS_REG((uiMIPSword>>11)&0x1f); return 0; // SLLV
				case 0x07: *puiCPUregs_out |= MIPS_REG((uiMIPSword>>11)&0x1f); return 0; // SRAV
				case 0x08: return 0; // JR
				case 0x09: *puiCPUregs_out |= MIPS_REG((uiMIPSword>>11)&0x1f); return 0; // JALR
				case 0x0C: return 0; // SYSCALL
				case 0x0D: return 0; // BREAK
				case 0x0F: return 0; // SYNC
				case 0x10: *puiCPUregs_out |= MIPS_REG((uiMIPSword>>11)&0x1f); return 0; // MFHI; return 0;
				case 0x11: *puiCPUregs_out |= MIPS_REG_HI; return 0; // MTHI; return 0;
				case 0x12: *puiCPUregs_out |= MIPS_REG((uiMIPSword>>11)&0x1f); return 0; // MFLO; return 0;
				case 0x13: *puiCPUregs_out |= MIPS_REG_LO; return 0; // MTLO; return 0;
				case 0x14: *puiCPUregs_out |= MIPS_REG((uiMIPSword>>11)&0x1f); return 0; // DSLLV; return 0;
				case 0x16: *puiCPUregs_out |= MIPS_REG((uiMIPSword>>11)&0x1f); return 0; // DSRLV; return 0;
				case 0x17: *puiCPUregs_out |= MIPS_REG((uiMIPSword>>11)&0x1f); return 0; // DSRAV; return 0;
				case 0x18: *puiSpecialregs_out |= MIPS_REG_HI | MIPS_REG_LO; return 0; // MULT; return 0;
				case 0x19: *puiSpecialregs_out |= MIPS_REG_HI | MIPS_REG_LO; return 0; // MULTU; return 0;
				case 0x1A: *puiSpecialregs_out |= MIPS_REG_HI | MIPS_REG_LO; return 0; // DIV; return 0;
				case 0x1B: *puiSpecialregs_out |= MIPS_REG_HI | MIPS_REG_LO; return 0; // DIVU; return 0;
				case 0x1C: *puiSpecialregs_out |= MIPS_REG_HI | MIPS_REG_LO; return 0; // DMULT; return 0;
				case 0x1D: *puiSpecialregs_out |= MIPS_REG_HI | MIPS_REG_LO; return 0; // DMULTU; return 0;
				case 0x1E: *puiSpecialregs_out |= MIPS_REG_HI | MIPS_REG_LO; return 0; // DDIV; return 0;
				case 0x1F: *puiSpecialregs_out |= MIPS_REG_HI | MIPS_REG_LO; return 0; // DDIVU; return 0;
				case 0x20: *puiCPUregs_out |= MIPS_REG((uiMIPSword>>11)&0x1f); return 0; // ADD; return 0;
				case 0x21: *puiCPUregs_out |= MIPS_REG((uiMIPSword>>11)&0x1f); return 0; // ADDU; return 0;
				case 0x22: *puiCPUregs_out |= MIPS_REG((uiMIPSword>>11)&0x1f); return 0; // SUB; return 0;
				case 0x23: *puiCPUregs_out |= MIPS_REG((uiMIPSword>>11)&0x1f); return 0; // SUBU; return 0;
				case 0x24: *puiCPUregs_out |= MIPS_REG((uiMIPSword>>11)&0x1f); return 0; // AND; return 0;
				case 0x25: *puiCPUregs_out |= MIPS_REG((uiMIPSword>>11)&0x1f); return 0; // OR; return 0;
				case 0x26: *puiCPUregs_out |= MIPS_REG((uiMIPSword>>11)&0x1f); return 0; // XOR; return 0;
				case 0x27: *puiCPUregs_out |= MIPS_REG((uiMIPSword>>11)&0x1f); return 0; // NOR; return 0;
				case 0x2A: *puiCPUregs_out |= MIPS_REG((uiMIPSword>>11)&0x1f); return 0; // SLT; return 0;
				case 0x2B: *puiCPUregs_out |= MIPS_REG((uiMIPSword>>11)&0x1f); return 0; // SLTU; return 0;
				case 0x2C: *puiCPUregs_out |= MIPS_REG((uiMIPSword>>11)&0x1f); return 0; // DADD; return 0;
				case 0x2D: *puiCPUregs_out |= MIPS_REG((uiMIPSword>>11)&0x1f); return 0; // DADDU
				case 0x2E: *puiCPUregs_out |= MIPS_REG((uiMIPSword>>11)&0x1f); return 0; // DSUB
				case 0x2F: *puiCPUregs_out |= MIPS_REG((uiMIPSword>>11)&0x1f); return 0; // DSUBU
				case 0x30: *puiSpecialregs_out |= MIPS_TRAP; return 0; // TGE
				case 0x31: *puiSpecialregs_out |= MIPS_TRAP; return 0; // TGEU
				case 0x32: *puiSpecialregs_out |= MIPS_TRAP; return 0; // TLT
				case 0x33: *puiSpecialregs_out |= MIPS_TRAP; return 0; // TLTU
				case 0x34: *puiSpecialregs_out |= MIPS_TRAP; return 0; // TEQ
				case 0x36: *puiSpecialregs_out |= MIPS_TRAP; return 0; // TNE

				case 0x38: *puiCPUregs_out |= MIPS_REG((uiMIPSword>>11)&0x1f); return 0; // DSLL
				case 0x3A: *puiCPUregs_out |= MIPS_REG((uiMIPSword>>11)&0x1f); return 0; // DSRL
				case 0x3B: *puiCPUregs_out |= MIPS_REG((uiMIPSword>>11)&0x1f); return 0; // DSRA
				case 0x3C: *puiCPUregs_out |= MIPS_REG((uiMIPSword>>11)&0x1f); return 0; // DSLL32
				case 0x3E: *puiCPUregs_out |= MIPS_REG((uiMIPSword>>11)&0x1f); return 0; // DSRL32
				case 0x3F: *puiCPUregs_out |= MIPS_REG((uiMIPSword>>11)&0x1f); return 0; // DSRA32
			}
			break; return 1;
		case 0x01:
			op2=(uiMIPSword>>16)&0x1f;
			switch(op2)
			{
			case 0x00: 	*puiSpecialregs_out |= MIPS_REG_PC; return 0; // BLTZ
			case 0x01: 	*puiSpecialregs_out |= MIPS_REG_PC; return 0; // BGEZ
			case 0x02: 	*puiSpecialregs_out |= MIPS_REG_PC; return 0; // BLTZL
			case 0x03: 	*puiSpecialregs_out |= MIPS_REG_PC; return 0; // BGEZL
			case 0x08: 	*puiSpecialregs_out |= MIPS_TRAP; return 0; // TGEI
			case 0x09: 	*puiSpecialregs_out |= MIPS_TRAP; return 0; // TGEIU
			case 0x0A: 	*puiSpecialregs_out |= MIPS_TRAP; return 0; // TLTI
			case 0x0B: 	*puiSpecialregs_out |= MIPS_TRAP; return 0; // TLTIU
			case 0x0C: 	*puiSpecialregs_out |= MIPS_TRAP; return 0; // TEQI
			case 0x0E: 	*puiSpecialregs_out |= MIPS_TRAP; return 0; // TNEI
			case 0x10: 	*puiCPUregs_out |= MIPS_REG_31;
						*puiSpecialregs_out |= MIPS_REG_PC; return 0; // BLTZAL
			case 0x11: 	*puiCPUregs_out |= MIPS_REG_31;
						*puiSpecialregs_out |= MIPS_REG_PC; return 0; // BGEZAL
			case 0x12: 	*puiCPUregs_out |= MIPS_REG_31;
						*puiSpecialregs_out |= MIPS_REG_PC; return 0; // BLTZALL
			case 0x13: 	*puiCPUregs_out |= MIPS_REG_31;
						*puiSpecialregs_out |= MIPS_REG_PC; return 0; // BGEZALL
			}

			break; return 1;

		case 0x02: 	*puiSpecialregs_out |= MIPS_REG_PC; return 0; //J; return 0;
		case 0x03: 	*puiCPUregs_out |= MIPS_REG_31;
					*puiSpecialregs_out |= MIPS_REG_PC; return 0; //JAL; return 0;
		case 0x04: 	*puiSpecialregs_out |= MIPS_REG_PC; return 0; // BEQ; return 0;	// I
		case 0x05: 	*puiSpecialregs_out |= MIPS_REG_PC; return 0; // BNE; return 0;	// I
		case 0x06: 	*puiSpecialregs_out |= MIPS_REG_PC; return 0; // BLEZ; return 0;
		case 0x07: 	*puiSpecialregs_out |= MIPS_REG_PC; return 0; // BGTZ; return 0;
		case 0x08: 	*puiCPUregs_out |= MIPS_REG((uiMIPSword>>16)&0x1f); return 0; // ADDI; return 0;	// I
		case 0x09: 	*puiCPUregs_out |= MIPS_REG((uiMIPSword>>16)&0x1f); return 0; // ADDIU; return 0;	// I
		case 0x0A: 	*puiCPUregs_out |= MIPS_REG((uiMIPSword>>16)&0x1f); return 0; // SLTI; return 0;	// I
		case 0x0B: 	*puiCPUregs_out |= MIPS_REG((uiMIPSword>>16)&0x1f); return 0; // SLTIU
		case 0x0C: 	*puiCPUregs_out |= MIPS_REG((uiMIPSword>>16)&0x1f); return 0; // ANDI
		case 0x0D: 	*puiCPUregs_out |= MIPS_REG((uiMIPSword>>16)&0x1f); return 0; // ORI
		case 0x0E: 	*puiCPUregs_out |= MIPS_REG((uiMIPSword>>16)&0x1f); return 0; // XORI
		case 0x0F: 	*puiCPUregs_out |= MIPS_REG((uiMIPSword>>16)&0x1f); return 0; // LUI
		case 0x10: 	// cop0
			op2=(uiMIPSword>>21)&0x1f;
			switch(op2)
			{
			case 0x00: *puiCPUregs_out |= MIPS_REG((uiMIPSword>>16)&0x1f); return 0; // MFC0
			case 0x04: *puiFPUregs_out |= MIPS_REG((uiMIPSword>>11)&0x1f); return 0; // MTC0
			case 0x10: // tlb;
				switch(uiMIPSword&0x3f)
				{
				case 0x01: *puiCPUregs_out |= MIPS_REG_ALL; return 2; // TLBR
				case 0x02: *puiCPUregs_out |= MIPS_REG_ALL; return 2; // TLBWI
				case 0x06: *puiCPUregs_out |= MIPS_REG_ALL; return 2; // TLBWR
				case 0x08: *puiCPUregs_out |= MIPS_REG_ALL; return 2; // TLBP
				case 0x18: *puiCPUregs_out |= MIPS_REG_ALL; return 2; // ERET
				}
			}
			break; return 1;

		case 0x11: // cop1
			op2=(uiMIPSword>>21)&0x1f;
			switch(op2)
			{
			case 0x00: *puiCPUregs_out |= MIPS_REG((uiMIPSword>>16)&0x1f); return 0; // MFC1
			case 0x01: *puiCPUregs_out |= MIPS_REG((uiMIPSword>>16)&0x1f); return 0; // DMFC1
			case 0x02: *puiCPUregs_out |= MIPS_REG((uiMIPSword>>16)&0x1f); return 0; // CFC1
			case 0x04: *puiFPUregs_out |= MIPS_REG((uiMIPSword>>11)&0x1f); return 0; // MTC1
			case 0x05: *puiFPUregs_out |= MIPS_REG((uiMIPSword>>11)&0x1f); return 0; // DMTC1
			case 0x06: *puiFPUregs_out |= MIPS_REG((uiMIPSword>>11)&0x1f); return 0; // CTC1
			case 0x08: // BC1;
				switch((uiMIPSword>>16)&0x3)
				{
				case 0x00: *puiSpecialregs_out |= MIPS_REG_CC; return 0; // BC1F
				case 0x01: *puiSpecialregs_out |= MIPS_REG_CC; return 0; // BC1T
				case 0x02: *puiSpecialregs_out |= MIPS_REG_CC; return 0; // BC1FL
				case 0x03: *puiSpecialregs_out |= MIPS_REG_CC; return 0; // BC1TL
				}break; return 1;

			case 0x10: // C1.S
			case 0x11:
				switch(uiMIPSword&0x3f)
				{
				case 0x00: *puiFPUregs_out |= MIPS_REG((uiMIPSword>>6)&0x1f); return 0; // ADD_S
				case 0x01: *puiFPUregs_out |= MIPS_REG((uiMIPSword>>6)&0x1f); return 0; // SUB_S; return 0;
				case 0x02: *puiFPUregs_out |= MIPS_REG((uiMIPSword>>6)&0x1f); return 0; // MUL_S; return 0;
				case 0x03: *puiFPUregs_out |= MIPS_REG((uiMIPSword>>6)&0x1f); return 0; // DIV_S; return 0;
				case 0x04: *puiFPUregs_out |= MIPS_REG((uiMIPSword>>6)&0x1f); return 0; // SQRT_S; return 0;
				case 0x05: *puiFPUregs_out |= MIPS_REG((uiMIPSword>>6)&0x1f); return 0; // ABS_S; return 0;
				case 0x06: *puiFPUregs_out |= MIPS_REG((uiMIPSword>>6)&0x1f); return 0; // MOV_S; return 0;
				case 0x07: *puiFPUregs_out |= MIPS_REG((uiMIPSword>>6)&0x1f); return 0; // NEG_S; return 0;
				case 0x08: *puiFPUregs_out |= MIPS_REG((uiMIPSword>>6)&0x1f); return 0; // ROUND_L_S; return 0;
				case 0x09: *puiFPUregs_out |= MIPS_REG((uiMIPSword>>6)&0x1f); return 0; // TRUNC_L_S; return 0;
				case 0x0A: *puiFPUregs_out |= MIPS_REG((uiMIPSword>>6)&0x1f); return 0; // CEIL_L_S; return 0;
				case 0x0B: *puiFPUregs_out |= MIPS_REG((uiMIPSword>>6)&0x1f); return 0; // FLOOR_L_S; return 0;
				case 0x0C: *puiFPUregs_out |= MIPS_REG((uiMIPSword>>6)&0x1f); return 0; // ROUND_W_S; return 0;
				case 0x0D: *puiFPUregs_out |= MIPS_REG((uiMIPSword>>6)&0x1f); return 0; // TRUNC_W_S; return 0;
				case 0x0E: *puiFPUregs_out |= MIPS_REG((uiMIPSword>>6)&0x1f); return 0; // CEIL_W_S; return 0;
				case 0x0F: *puiFPUregs_out |= MIPS_REG((uiMIPSword>>6)&0x1f); return 0; // FLOOR_W_S; return 0;

				case 0x21: *puiFPUregs_out |= MIPS_REG((uiMIPSword>>6)&0x1f); return 0; // CVT_D_S; return 0;
				case 0x24: *puiFPUregs_out |= MIPS_REG((uiMIPSword>>6)&0x1f); return 0; // CVT_W_S; return 0;
				case 0x25: *puiCPUregs_out |= MIPS_REG_ALL; return 2; // CVT_L_S; return 0;
				case 0x30: *puiCPUregs_out |= MIPS_REG_ALL; return 2; // C_F_S; return 0;
				case 0x31: *puiCPUregs_out |= MIPS_REG_ALL; return 2; // C_UN_S; return 0;
				case 0x32: *puiSpecialregs_out |= MIPS_REG_CC; return 0; // C_EQ_S; return 0;
				case 0x33: *puiCPUregs_out |= MIPS_REG_ALL; return 2; // C_UEQ_S; return 0;
				case 0x34: *puiCPUregs_out |= MIPS_REG_ALL; return 2; // C_OLT_S; return 0;
				case 0x35: *puiCPUregs_out |= MIPS_REG_ALL; return 2; // C_ULT_S; return 0;
				case 0x36: *puiCPUregs_out |= MIPS_REG_ALL; return 2; // C_OLE_S; return 0;
				case 0x37: *puiCPUregs_out |= MIPS_REG_ALL; return 2; // C_ULE_S; return 0;
				case 0x38: *puiCPUregs_out |= MIPS_REG_ALL; return 2; // C_SF_S; return 0;
				case 0x39: *puiCPUregs_out |= MIPS_REG_ALL; return 2; // C_NGLE_S; return 0;
				case 0x3A: *puiCPUregs_out |= MIPS_REG_ALL; return 2; // C_SEQ_S; return 0;
				case 0x3B: *puiCPUregs_out |= MIPS_REG_ALL; return 2; // C_NGL_S; return 0;
				case 0x3C: *puiSpecialregs_out |= MIPS_REG_CC; return 0; // C_LT_S; return 0;
				case 0x3D: *puiCPUregs_out |= MIPS_REG_ALL; return 2; // C_NGE_S; return 0;
				case 0x3E: *puiSpecialregs_out |= MIPS_REG_CC; return 0; // C_LE_S; return 0;
				case 0x3F: *puiCPUregs_out |= MIPS_REG_ALL; return 2; // C_NGT_S; return 0;
				}break; return 1;
		/*	case 0x11: // C1_D
				switch(uiMIPSword&0x3f)
				{
				case 0x00: *puiFPUregs_out |= MIPS_REG((uiMIPSword>>6)&0x1f); return 0; // ADD_D; return 0;
				case 0x01: *puiFPUregs_out |= MIPS_REG((uiMIPSword>>6)&0x1f); return 0; // SUB_D; return 0;
				case 0x02: *puiFPUregs_out |= MIPS_REG((uiMIPSword>>6)&0x1f); return 0; // MUL_D; return 0;
				case 0x03: *puiFPUregs_out |= MIPS_REG((uiMIPSword>>6)&0x1f); return 0; // DIV_D; return 0;
				case 0x04: *puiFPUregs_out |= MIPS_REG((uiMIPSword>>6)&0x1f); return 0; // SQRT_D; return 0;
				case 0x05: *puiFPUregs_out |= MIPS_REG((uiMIPSword>>6)&0x1f); return 0; // ABS_D; return 0;
				case 0x06: *puiFPUregs_out |= MIPS_REG((uiMIPSword>>6)&0x1f); return 0; // MOV_D; return 0;
				case 0x07: *puiFPUregs_out |= MIPS_REG((uiMIPSword>>6)&0x1f); return 0; // NEG_D; return 0;
				case 0x08: *puiFPUregs_out |= MIPS_REG((uiMIPSword>>6)&0x1f); return 0; // ROUND_L_D; return 0;
				case 0x09: *puiFPUregs_out |= MIPS_REG((uiMIPSword>>6)&0x1f); return 0; // TRUNC_L_D; return 0;
				case 0x0A: *puiFPUregs_out |= MIPS_REG((uiMIPSword>>6)&0x1f); return 0; // CEIL_L_D; return 0;
				case 0x0B: *puiFPUregs_out |= MIPS_REG((uiMIPSword>>6)&0x1f); return 0; // FLOOR_L_D; return 0;
				case 0x0C: *puiFPUregs_out |= MIPS_REG((uiMIPSword>>6)&0x1f); return 0; // ROUND_W_D; return 0;
				case 0x0D: *puiFPUregs_out |= MIPS_REG((uiMIPSword>>6)&0x1f); return 0; // TRUNC_W_D; return 0;
				case 0x0E: *puiFPUregs_out |= MIPS_REG((uiMIPSword>>6)&0x1f); return 0; // CEIL_W_D; return 0;
				case 0x0F: *puiFPUregs_out |= MIPS_REG((uiMIPSword>>6)&0x1f); return 0; // FLOOR_W_D; return 0;
				case 0x20: *puiCPUregs_out |= MIPS_REG_ALL; return 2; // CVT_S_D; return 0;
				case 0x24: *puiCPUregs_out |= MIPS_REG_ALL; return 2; // CVT_W_D; return 0;
				case 0x25: *puiCPUregs_out |= MIPS_REG_ALL; return 2; // CVT_L_D; return 0;
				case 0x30: *puiCPUregs_out |= MIPS_REG_ALL; return 2; // C_F_D; return 0;
				case 0x31: *puiCPUregs_out |= MIPS_REG_ALL; return 2; // C_UN_D; return 0;
				case 0x32: *puiCPUregs_out |= MIPS_REG_ALL; return 2; // C_EQ_D; return 0;
				case 0x33: *puiCPUregs_out |= MIPS_REG_ALL; return 2; // C_UEQ_D; return 0;
				case 0x34: *puiCPUregs_out |= MIPS_REG_ALL; return 2; // C_OLT_D; return 0;
				case 0x35: *puiCPUregs_out |= MIPS_REG_ALL; return 2; // C_ULT_D; return 0;
				case 0x36: *puiCPUregs_out |= MIPS_REG_ALL; return 2; // C_OLE_D; return 0;
				case 0x37: *puiCPUregs_out |= MIPS_REG_ALL; return 2; // C_ULE_D; return 0;
				case 0x38: *puiCPUregs_out |= MIPS_REG_ALL; return 2; // C_SF_D; return 0;
				case 0x39: *puiCPUregs_out |= MIPS_REG_ALL; return 2; // C_NGLE_D; return 0;
				case 0x3A: *puiCPUregs_out |= MIPS_REG_ALL; return 2; // C_SEQ_D; return 0;
				case 0x3B: *puiCPUregs_out |= MIPS_REG_ALL; return 2; // C_NGL_D; return 0;
				case 0x3C: *puiCPUregs_out |= MIPS_REG_ALL; return 2; // C_LT_D; return 0;
				case 0x3D: *puiCPUregs_out |= MIPS_REG_ALL; return 2; // C_NGE_D; return 0;
				case 0x3E: *puiCPUregs_out |= MIPS_REG_ALL; return 2; // C_LE_D; return 0;
				case 0x3F: *puiCPUregs_out |= MIPS_REG_ALL; return 2; // C_NGT_D; return 0;
				} break; return 1;*/
			case 0x14: // C1_W
				switch(uiMIPSword&0x3f)
				{
				case 0x20: *puiFPUregs_out |= MIPS_REG((uiMIPSword>>6)&0x1f); return 0; // CVT_S_W; return 0;
				case 0x21: *puiFPUregs_out |= MIPS_REG((uiMIPSword>>6)&0x1f); return 0; // CVT_D_W; return 0;
				}
				break; return 1;

			case 0x15: // C1_L
				switch(uiMIPSword&0x3f)
				{
				case 0x20: *puiCPUregs_out |= MIPS_REG_ALL; return 2; // CVT_S_L; return 0;
				case 0x21: *puiCPUregs_out |= MIPS_REG_ALL; return 2; // CVT_D_L; return 0;
				}
				break; return 1;
			}break; return 1;

		case 0x14: *puiSpecialregs_out |= MIPS_REG_PC; return 0; // BEQL; return 0;
		case 0x15: *puiSpecialregs_out |= MIPS_REG_PC; return 0; // BNEL; return 0;
		case 0x16: *puiSpecialregs_out |= MIPS_REG_PC; return 0; // BLEZL; return 0;
		case 0x17: *puiSpecialregs_out |= MIPS_REG_PC; return 0; // BGTZL; return 0;
		case 0x18: *puiCPUregs_out |= MIPS_REG((uiMIPSword>>16)&0x1f); return 0; // DADDI; return 0;
		case 0x19: *puiCPUregs_out |= MIPS_REG((uiMIPSword>>16)&0x1f); return 0; // DADDIU; return 0;
		case 0x1A: *puiCPUregs_out |= MIPS_REG((uiMIPSword>>16)&0x1f); return 0; // LDL; return 0;
		case 0x1B: *puiCPUregs_out |= MIPS_REG((uiMIPSword>>16)&0x1f); return 0; // LDR; return 0;
		case 0x20: *puiCPUregs_out |= MIPS_REG((uiMIPSword>>16)&0x1f); return 0; // LB; return 0;	// Load Byte
		case 0x21: *puiCPUregs_out |= MIPS_REG((uiMIPSword>>16)&0x1f); return 0; // LH; return 0;	// Load Halfword
		case 0x22: *puiCPUregs_out |= MIPS_REG((uiMIPSword>>16)&0x1f); return 0; // LWL; return 0;
		case 0x23: *puiCPUregs_out |= MIPS_REG((uiMIPSword>>16)&0x1f); return 0; // LW; return 0;	// Load Word
		case 0x24: *puiCPUregs_out |= MIPS_REG((uiMIPSword>>16)&0x1f); return 0; // LBU; return 0;	// Load Unsigned Byte
		case 0x25: *puiCPUregs_out |= MIPS_REG((uiMIPSword>>16)&0x1f); return 0; // LHU; return 0;	// Load Halfword unsigned
		case 0x26: *puiCPUregs_out |= MIPS_REG((uiMIPSword>>21)&0x1f); return 0; // LWR; return 0;
		case 0x27: *puiCPUregs_out |= MIPS_REG((uiMIPSword>>21)&0x1f); return 0; // LWU; return 0;	// Load Word unsigned
		case 0x28: return 0; // SB; return 0;	// I
		case 0x29: return 0; // SH; return 0;	// I
		case 0x2A: return 0; // SWL; return 0;
		case 0x2B: return 0; // SW; return 0;	// I
		case 0x2C: return 0; // SDL; return 0;
		case 0x2D: return 0; // SDR; return 0;
		case 0x2E: return 0; // SWR; return 0;
		case 0x2F: *puiCPUregs_out |= 0; return 0; // CACHE; return 0;
		case 0x30: *puiCPUregs_out |= MIPS_REG((uiMIPSword>>16)&0x1f);
					*puiSpecialregs_out |= MIPS_REG_LL; return 0; // LL; return 0;	// Load Linked Word atomic Read-Modify-Write ops
		case 0x31: *puiFPUregs_out |= MIPS_REG((uiMIPSword>>16)&0x1f); return 0; // LWC1; return 0;	// Load Word to co processor 1
		case 0x34: *puiCPUregs_out |= MIPS_REG((uiMIPSword>>16)&0x1f); return 0; // LLD; return 0;	// Load Linked Dbl Word atomic Read-Modify-Write ops
					*puiSpecialregs_out |= MIPS_REG_LL; return 0;
		case 0x35: *puiCPUregs_out |= MIPS_REG((uiMIPSword>>21)&0x1f) | MIPS_REG((uiMIPSword>>16)&0x1f); return 0; // LDC1; return 0;
		case 0x37: *puiCPUregs_out |= MIPS_REG((uiMIPSword>>21)&0x1f); return 0; // LD; return 0; 	// Load Double word
		case 0x38: *puiCPUregs_out |= MIPS_REG((uiMIPSword>>16)&0x1f); return 0; // SC; return 0;	// Store Linked Word atomic Read-Modify-Write ops
		case 0x39: return 0; // SWC1; return 0;	// Store Word from co processor 1 to memory
		case 0x3C: *puiCPUregs_out |= MIPS_REG((uiMIPSword>>16)&0x1f); return 0; // SCD; return 0;	// Store Conditional Double Word
		case 0x3D: return 0; // SDC1; return 0;
		case 0x3F: return 0; // SD; return 0; 	// Store Double word
	}

	return 1;
}

uint32_t ops_regs_input(const uint32_t uiMIPSword, uint32_t * const puiCPUregs_in, uint32_t * const puiFPUregs_in, uint32_t * const puiSpecialregs_in)
{
	uint32_t op=uiMIPSword>>26;
	uint32_t op2;

	switch(op)
	{
		case 0x00:
			op2=uiMIPSword&0x3f;
			switch(op2)
			{
				case 0x00: *puiCPUregs_in |= MIPS_REG((uiMIPSword>>16)&0x1f); return 0; // SLL;
				case 0x02: *puiCPUregs_in |= MIPS_REG((uiMIPSword>>16)&0x1f); return 0; // SRL; return 0;
				case 0x03: *puiCPUregs_in |= MIPS_REG((uiMIPSword>>16)&0x1f); return 0; // SRA; return 0;
				case 0x04: *puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f) | MIPS_REG((uiMIPSword>>16)&0x1f); return 0; // SLLV; return 0;
				case 0x07: *puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f) | MIPS_REG((uiMIPSword>>16)&0x1f); return 0; // SRAV; return 0;
				case 0x08: *puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f); return 0; // JR; return 0;
				case 0x09: *puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f); return 0; // JALR; return 0;
				case 0x0C: return 0; // SYSCALL; return 0;
				case 0x0D: return 0; // BREAK; return 0;
				case 0x0F: return 0; // SYNC; return 0;
				case 0x10: *puiCPUregs_in |= MIPS_REG_HI; return 0; // MFHI; return 0;
				case 0x11: *puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f); return 0; // MTHI; return 0;
				case 0x12: *puiCPUregs_in |= MIPS_REG_LO; return 0; // MFLO; return 0;
				case 0x13: *puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f); return 0; // MTLO; return 0;
				case 0x14: *puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f) | MIPS_REG((uiMIPSword>>16)&0x1f); return 0; // DSLLV; return 0;
				case 0x16: *puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f) | MIPS_REG((uiMIPSword>>16)&0x1f); return 0; // DSRLV; return 0;
				case 0x17: *puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f) | MIPS_REG((uiMIPSword>>16)&0x1f); return 0; // DSRAV; return 0;
				case 0x18: *puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f) | MIPS_REG((uiMIPSword>>16)&0x1f); return 0; // MULT; return 0;
				case 0x19: *puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f) | MIPS_REG((uiMIPSword>>16)&0x1f); return 0; // MULTU; return 0;
				case 0x1A: *puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f) | MIPS_REG((uiMIPSword>>16)&0x1f); return 0; // DIV; return 0;
				case 0x1B: *puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f) | MIPS_REG((uiMIPSword>>16)&0x1f); return 0; // DIVU; return 0;
				case 0x1C: *puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f) | MIPS_REG((uiMIPSword>>16)&0x1f); return 0; // DMULT; return 0;
				case 0x1D: *puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f) | MIPS_REG((uiMIPSword>>16)&0x1f); return 0; // DMULTU; return 0;
				case 0x1E: *puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f) | MIPS_REG((uiMIPSword>>16)&0x1f); return 0; // DDIV; return 0;
				case 0x1F: *puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f) | MIPS_REG((uiMIPSword>>16)&0x1f); return 0; // DDIVU; return 0;
				case 0x20: *puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f) | MIPS_REG((uiMIPSword>>16)&0x1f); return 0; // ADD; return 0;
				case 0x21: *puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f) | MIPS_REG((uiMIPSword>>16)&0x1f); return 0; // ADDU; return 0;
				case 0x22: *puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f) | MIPS_REG((uiMIPSword>>16)&0x1f); return 0; // SUB; return 0;
				case 0x23: *puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f) | MIPS_REG((uiMIPSword>>16)&0x1f); return 0; // SUBU; return 0;
				case 0x24: *puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f) | MIPS_REG((uiMIPSword>>16)&0x1f); return 0; // AND; return 0;
				case 0x25: *puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f) | MIPS_REG((uiMIPSword>>16)&0x1f); return 0; // OR; return 0;
				case 0x26: *puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f) | MIPS_REG((uiMIPSword>>16)&0x1f); return 0; // XOR; return 0;
				case 0x27: *puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f) | MIPS_REG((uiMIPSword>>16)&0x1f); return 0; // NOR; return 0;
				case 0x2A: *puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f) | MIPS_REG((uiMIPSword>>16)&0x1f); return 0; // SLT; return 0;
				case 0x2B: *puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f) | MIPS_REG((uiMIPSword>>16)&0x1f); return 0; // SLTU; return 0;
				case 0x2C: *puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f) | MIPS_REG((uiMIPSword>>16)&0x1f); return 0; // DADD; return 0;
				case 0x2D: *puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f) | MIPS_REG((uiMIPSword>>16)&0x1f); return 0; // DADDU; return 0;
				case 0x2E: *puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f) | MIPS_REG((uiMIPSword>>16)&0x1f); return 0; // DSUB; return 0;
				case 0x2F: *puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f) | MIPS_REG((uiMIPSword>>16)&0x1f); return 0; // DSUBU; return 0;
				case 0x30: *puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f) | MIPS_REG((uiMIPSword>>16)&0x1f); return 0; // TGE; return 0;
				case 0x31: *puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f) | MIPS_REG((uiMIPSword>>16)&0x1f); return 0; // TGEU; return 0;
				case 0x32: *puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f) | MIPS_REG((uiMIPSword>>16)&0x1f); return 0; // TLT; return 0;
				case 0x33: *puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f) | MIPS_REG((uiMIPSword>>16)&0x1f); return 0; // TLTU; return 0;
				case 0x34: *puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f) | MIPS_REG((uiMIPSword>>16)&0x1f); return 0; // TEQ; return 0;
				case 0x36: *puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f) | MIPS_REG((uiMIPSword>>16)&0x1f); return 0; // TNE; return 0;
				case 0x38: *puiCPUregs_in |= MIPS_REG((uiMIPSword>>16)&0x1f); return 0; // DSLL; return 0;
				case 0x3A: *puiCPUregs_in |= MIPS_REG((uiMIPSword>>16)&0x1f); return 0; // DSRL; return 0;
				case 0x3B: *puiCPUregs_in |= MIPS_REG((uiMIPSword>>16)&0x1f); return 0; // DSRA; return 0;
				case 0x3C: *puiCPUregs_in |= MIPS_REG((uiMIPSword>>16)&0x1f); return 0; // DSLL32; return 0;
				case 0x3E: *puiCPUregs_in |= MIPS_REG((uiMIPSword>>16)&0x1f); return 0; // DSRL32; return 0;
				case 0x3F: *puiCPUregs_in |= MIPS_REG((uiMIPSword>>16)&0x1f); return 0; // DSRA32; return 0;
			}
			break; return 1;
		case 0x01:
			op2=(uiMIPSword>>16)&0x1f;
			switch(op2)
			{
			case 0x00: 	*puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f); return 0; // BLTZ; return 0;	// I
			case 0x01: 	*puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f); return 0; // BGEZ; return 0;	// I
			case 0x02: 	*puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f); return 0; // BLTZL; return 0;
			case 0x03: 	*puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f); return 0; // BGEZL; return 0;
			case 0x08: 	*puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f); return 0; // TGEI; return 0;
			case 0x09: 	*puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f); return 0; // TGEIU; return 0;
			case 0x0A: 	*puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f); return 0; // TLTI; return 0;
			case 0x0B: 	*puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f); return 0; // TLTIU; return 0;
			case 0x0C: 	*puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f); return 0; // TEQI; return 0;
			case 0x0E: 	*puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f); return 0; // TNEI; return 0;
			case 0x10: 	*puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f); return 0; // BLTZAL; return 0;	// I and link
			case 0x11: 	*puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f); return 0; // BGEZAL; return 0;	// I and link
			case 0x12: 	*puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f); return 0; // BLTZALL; return 0;	// I and link likely
			case 0x13: 	*puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f); return 0; // BGEZALL; return 0;	// I and link likely
			}

			break; return 1;

		case 0x02: 	return 0; //J; return 0;
		case 0x03: 	return 0; //JAL; return 0;
		case 0x04: 	*puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f) | MIPS_REG((uiMIPSword>>16)&0x1f); return 0; // BEQ; return 0;	// I
		case 0x05: 	*puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f) | MIPS_REG((uiMIPSword>>16)&0x1f); return 0; // BNE; return 0;	// I
		case 0x06: 	*puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f); return 0; // BLEZ; return 0;
		case 0x07: 	*puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f); return 0; // BGTZ; return 0;
		case 0x08: 	*puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f); return 0; // ADDI; return 0;	// I
		case 0x09: 	*puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f); return 0; // ADDIU; return 0;	// I
		case 0x0A: 	*puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f); return 0; // SLTI; return 0;	// I
		case 0x0B: 	*puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f); return 0; // SLTIU; return 0;	// I
		case 0x0C: 	*puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f); return 0; // ANDI; return 0; 	// I
		case 0x0D: 	*puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f); return 0; // ORI; return 0;	// I
		case 0x0E: 	*puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f); return 0; // XORI; return 0;	// I
		case 0x0F: 	*puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f); return 0; // LUI; return 0;	// I
		case 0x10: 	// cop0
			op2=(uiMIPSword>>21)&0x1f;
			switch(op2)
			{
			case 0x00: *puiFPUregs_in |= MIPS_REG((uiMIPSword>>11)&0x1f); return 0; // MFC0; return 0;
			case 0x04: *puiCPUregs_in |= MIPS_REG((uiMIPSword>>16)&0x1f); return 0; // MTC0; return 0;
			case 0x10: // tlb
				switch(uiMIPSword&0x3f)
				{
				case 0x01: *puiCPUregs_in |= MIPS_REG_ALL; return 2; // TLBR; return 0;
				case 0x02: *puiCPUregs_in |= MIPS_REG_ALL; return 2; // TLBWI; return 0;
				case 0x06: *puiCPUregs_in |= MIPS_REG_ALL; return 2; // TLBWR; return 0;
				case 0x08: *puiCPUregs_in |= MIPS_REG_ALL; return 2; // TLBP; return 0;
				case 0x18: *puiCPUregs_in |= MIPS_REG_ALL; return 2; // ERET; return 0;
				}
			}
			break; return 1;

		case 0x11: // cop1
			op2=(uiMIPSword>>21)&0x1f;
			switch(op2)
			{
			case 0x00: *puiFPUregs_in |= MIPS_REG((uiMIPSword>>11)&0x1f); return 0; // MFC1
			case 0x01: *puiFPUregs_in |= MIPS_REG((uiMIPSword>>11)&0x1f); return 0; // DMFC1
			case 0x02: *puiFPUregs_in |= MIPS_REG((uiMIPSword>>11)&0x1f); return 0; // CFC1
			case 0x04: *puiCPUregs_in |= MIPS_REG((uiMIPSword>>16)&0x1f); return 0; // MTC1
			case 0x05: *puiCPUregs_in |= MIPS_REG((uiMIPSword>>16)&0x1f); return 0; // DMTC1
			case 0x06: *puiCPUregs_in |= MIPS_REG((uiMIPSword>>16)&0x1f); return 0; // CTC1
			case 0x08: //*uiCPUregs = BC1;
				switch((uiMIPSword>>16)&0x3)
				{
				case 0x00: return 0; // BC1F; return 0;
				case 0x01: return 0; // BC1T; return 0;
				case 0x02: return 0; // BC1FL; return 0;
				case 0x03: return 0; // BC1TL; return 0;
				}break; return 1;

			case 0x10: // C1.S
			case 0x11:
				switch(uiMIPSword&0x3f)
				{
				case 0x00: *puiFPUregs_in |= MIPS_REG((uiMIPSword>>16)&0x1f) | MIPS_REG((uiMIPSword>>11)&0x1f); return 0; // ADD_S
				case 0x01: *puiFPUregs_in |= MIPS_REG((uiMIPSword>>16)&0x1f) | MIPS_REG((uiMIPSword>>11)&0x1f); return 0; // SUB_S
				case 0x02: *puiFPUregs_in |= MIPS_REG((uiMIPSword>>16)&0x1f) | MIPS_REG((uiMIPSword>>11)&0x1f); return 0; // MUL_S
				case 0x03: *puiFPUregs_in |= MIPS_REG((uiMIPSword>>16)&0x1f) | MIPS_REG((uiMIPSword>>11)&0x1f); return 0; // DIV_S
				case 0x04: *puiFPUregs_in |= MIPS_REG((uiMIPSword>>11)&0x1f); return 0; // SQRT_S
				case 0x05: *puiFPUregs_in |= MIPS_REG((uiMIPSword>>11)&0x1f); return 0; // ABS_S
				case 0x06: *puiFPUregs_in |= MIPS_REG((uiMIPSword>>11)&0x1f); return 0; // MOV_S
				case 0x07: *puiFPUregs_in |= MIPS_REG((uiMIPSword>>11)&0x1f); return 0; // NEG_S
				case 0x08: *puiFPUregs_in |= MIPS_REG((uiMIPSword>>11)&0x1f); return 0; // ROUND_L_S
				case 0x09: *puiFPUregs_in |= MIPS_REG((uiMIPSword>>11)&0x1f); return 0; // TRUNC_L_S
				case 0x0A: *puiFPUregs_in |= MIPS_REG((uiMIPSword>>11)&0x1f); return 0; // CEIL_L_S
				case 0x0B: *puiFPUregs_in |= MIPS_REG((uiMIPSword>>11)&0x1f); return 0; // FLOOR_L_S
				case 0x0C: *puiFPUregs_in |= MIPS_REG((uiMIPSword>>11)&0x1f); return 0; // ROUND_W_S
				case 0x0D: *puiFPUregs_in |= MIPS_REG((uiMIPSword>>11)&0x1f); return 0; // TRUNC_W_S
				case 0x0E: *puiFPUregs_in |= MIPS_REG((uiMIPSword>>11)&0x1f); return 0; // CEIL_W_S
				case 0x0F: *puiFPUregs_in |= MIPS_REG((uiMIPSword>>11)&0x1f); return 0; // FLOOR_W_S
				// TODO some of these may be the wrong way round
				case 0x21: *puiFPUregs_in |= MIPS_REG((uiMIPSword>>11)&0x1f); return 0; // CVT_D_S
				case 0x24: *puiFPUregs_in |= MIPS_REG((uiMIPSword>>11)&0x1f); return 0; // CVT_W_S
				case 0x25: *puiCPUregs_in |= MIPS_REG_ALL; return 2; // CVT_L_S
				case 0x30: *puiCPUregs_in |= MIPS_REG_ALL; return 2; // C_F_S
				case 0x31: *puiCPUregs_in |= MIPS_REG_ALL; return 2; // C_UN_S
				case 0x32: *puiFPUregs_in |= MIPS_REG((uiMIPSword>>16)&0x1f) | MIPS_REG((uiMIPSword>>11)&0x1f); return 0; // C_EQ_S
				case 0x33: *puiCPUregs_in |= MIPS_REG_ALL; return 2; // C_UEQ_S
				case 0x34: *puiCPUregs_in |= MIPS_REG_ALL; return 2; // C_OLT_S
				case 0x35: *puiCPUregs_in |= MIPS_REG_ALL; return 2; // C_ULT_S
				case 0x36: *puiCPUregs_in |= MIPS_REG_ALL; return 2; // C_OLE_S
				case 0x37: *puiCPUregs_in |= MIPS_REG_ALL; return 2; // C_ULE_S
				case 0x38: *puiCPUregs_in |= MIPS_REG_ALL; return 2; // C_SF_S
				case 0x39: *puiCPUregs_in |= MIPS_REG_ALL; return 2; // C_NGLE_S
				case 0x3A: *puiCPUregs_in |= MIPS_REG_ALL; return 2; // C_SEQ_S
				case 0x3B: *puiCPUregs_in |= MIPS_REG_ALL; return 2; // C_NGL_S
				case 0x3C: *puiFPUregs_in |= MIPS_REG((uiMIPSword>>16)&0x1f) | MIPS_REG((uiMIPSword>>11)&0x1f); return 0; // C_LT_S
				case 0x3D: *puiCPUregs_in |= MIPS_REG_ALL; return 2; // C_NGE_S
				case 0x3E: *puiFPUregs_in |= MIPS_REG((uiMIPSword>>16)&0x1f) | MIPS_REG((uiMIPSword>>11)&0x1f); return 0; // C_LE_S
				case 0x3F: *puiCPUregs_in |= MIPS_REG_ALL; return 2; // C_NGT_S
				}break; return 1;
			/*case 0x11: // C1_D
				switch(uiMIPSword&0x3f)
				{
				case 0x00: *puiFPUregs_in |= MIPS_REG((uiMIPSword>>16)&0x1f) | MIPS_REG((uiMIPSword>>11)&0x1f); return 0; // ADD_D; return 0;
				case 0x01: *puiCPUregs_in |= MIPS_REG((uiMIPSword>>16)&0x1f) | MIPS_REG((uiMIPSword>>11)&0x1f); return 0; // SUB_D; return 0;
				case 0x02: *puiCPUregs_in |= MIPS_REG((uiMIPSword>>16)&0x1f) | MIPS_REG((uiMIPSword>>11)&0x1f); return 0; // MUL_D; return 0;
				case 0x03: *puiCPUregs_in |= MIPS_REG((uiMIPSword>>16)&0x1f) | MIPS_REG((uiMIPSword>>11)&0x1f); return 0; // DIV_D; return 0;
				case 0x04: *puiFPUregs_in |= MIPS_REG((uiMIPSword>>11)&0x1f); return 0; // SQRT_D; return 0;
				case 0x05: *puiFPUregs_in |= MIPS_REG((uiMIPSword>>11)&0x1f); return 0; // ABS_D; return 0;
				case 0x06: *puiFPUregs_in |= MIPS_REG((uiMIPSword>>11)&0x1f); return 0; // MOV_D; return 0;
				case 0x07: *puiFPUregs_in |= MIPS_REG((uiMIPSword>>11)&0x1f); return 0; // NEG_D; return 0;
				case 0x08: *puiFPUregs_in |= MIPS_REG((uiMIPSword>>11)&0x1f); return 0; // ROUND_L_D; return 0;
				case 0x09: *puiFPUregs_in |= MIPS_REG((uiMIPSword>>11)&0x1f); return 0; // TRUNC_L_D; return 0;
				case 0x0A: *puiFPUregs_in |= MIPS_REG((uiMIPSword>>11)&0x1f); return 0; // CEIL_L_D; return 0;
				case 0x0B: *puiFPUregs_in |= MIPS_REG((uiMIPSword>>11)&0x1f); return 0; // FLOOR_L_D; return 0;
				case 0x0C: *puiFPUregs_in |= MIPS_REG((uiMIPSword>>11)&0x1f); return 0; // ROUND_W_D; return 0;
				case 0x0D: *puiFPUregs_in |= MIPS_REG((uiMIPSword>>11)&0x1f); return 0; // TRUNC_W_D; return 0;
				case 0x0E: *puiFPUregs_in |= MIPS_REG((uiMIPSword>>11)&0x1f); return 0; // CEIL_W_D; return 0;
				case 0x0F: *puiFPUregs_in |= MIPS_REG((uiMIPSword>>11)&0x1f); return 0; // FLOOR_W_D; return 0;
				case 0x20: *puiCPUregs_in |= MIPS_REG_ALL; return 2; // CVT_S_D; return 0;
				case 0x24: *puiCPUregs_in |= MIPS_REG_ALL; return 2; // CVT_W_D; return 0;
				case 0x25: *puiCPUregs_in |= MIPS_REG_ALL; return 2; // CVT_L_D; return 0;
				case 0x30: *puiCPUregs_in |= MIPS_REG_ALL; return 2; // C_F_D; return 0;
				case 0x31: *puiCPUregs_in |= MIPS_REG_ALL; return 2; // C_UN_D; return 0;
				case 0x32: *puiCPUregs_in |= MIPS_REG_ALL; return 2; // C_EQ_D; return 0;
				case 0x33: *puiCPUregs_in |= MIPS_REG_ALL; return 2; // C_UEQ_D; return 0;
				case 0x34: *puiCPUregs_in |= MIPS_REG_ALL; return 2; // C_OLT_D; return 0;
				case 0x35: *puiCPUregs_in |= MIPS_REG_ALL; return 2; // C_ULT_D; return 0;
				case 0x36: *puiCPUregs_in |= MIPS_REG_ALL; return 2; // C_OLE_D; return 0;
				case 0x37: *puiCPUregs_in |= MIPS_REG_ALL; return 2; // C_ULE_D; return 0;
				case 0x38: *puiCPUregs_in |= MIPS_REG_ALL; return 2; // C_SF_D; return 0;
				case 0x39: *puiCPUregs_in |= MIPS_REG_ALL; return 2; // C_NGLE_D; return 0;
				case 0x3A: *puiCPUregs_in |= MIPS_REG_ALL; return 2; // C_SEQ_D; return 0;
				case 0x3B: *puiCPUregs_in |= MIPS_REG_ALL; return 2; // C_NGL_D; return 0;
				case 0x3C: *puiCPUregs_in |= MIPS_REG_ALL; return 2; // C_LT_D; return 0;
				case 0x3D: *puiCPUregs_in |= MIPS_REG_ALL; return 2; // C_NGE_D; return 0;
				case 0x3E: *puiCPUregs_in |= MIPS_REG_ALL; return 2; // C_LE_D; return 0;
				case 0x3F: *puiCPUregs_in |= MIPS_REG_ALL; return 2; // C_NGT_D; return 0;
				} break; return 1;*/
			case 0x14: //*uiCPUregs = C1_W\n",x);
				switch(uiMIPSword&0x3f)
				{
				case 0x20: *puiFPUregs_in |= MIPS_REG((uiMIPSword>>11)&0x1f); return 0; // CVT_S_W; return 0;
				case 0x21: *puiFPUregs_in |= MIPS_REG((uiMIPSword>>11)&0x1f); return 0; // CVT_D_W; return 0;
				}
				break; return 1;

			case 0x15: //*uiCPUregs = C1_L\n",x);
				switch(uiMIPSword&0x3f)
				{
				case 0x20: *puiCPUregs_in |= MIPS_REG_ALL; return 2; // CVT_S_L; return 0;
				case 0x21: *puiCPUregs_in |= MIPS_REG_ALL; return 2; // CVT_D_L; return 0;
				}
				break; return 1;
			}break; return 1;

		case 0x14: *puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f) | MIPS_REG((uiMIPSword>>16)&0x1f); return 0; // BEQL; return 0;
		case 0x15: *puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f) | MIPS_REG((uiMIPSword>>16)&0x1f); return 0; // BNEL; return 0;
		case 0x16: *puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f); return 0; // BLEZL; return 0;
		case 0x17: *puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f); return 0; // BGTZL; return 0;
		case 0x18: *puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f) | MIPS_REG((uiMIPSword>>16)&0x1f); return 0; // DADDI; return 0;
		case 0x19: *puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f) | MIPS_REG((uiMIPSword>>16)&0x1f); return 0; // DADDIU; return 0;
		case 0x1A: *puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f); return 0; // LDL; return 0;
		case 0x1B: *puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f); return 0; // LDR; return 0;
		case 0x20: *puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f); return 0; // LB; return 0;	// Load Byte
		case 0x21: *puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f); return 0; // LH; return 0;	// Load Halfword
		case 0x22: *puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f); return 0; // LWL; return 0;
		case 0x23: *puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f); return 0; // LW; return 0;	// Load Word
		case 0x24: *puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f); return 0; // LBU; return 0;	// Load Unsigned Byte
		case 0x25: *puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f); return 0; // LHU; return 0;	// Load Halfword unsigned
		case 0x26: *puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f); return 0; // LWR; return 0;
		case 0x27: *puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f); return 0; // LWU; return 0;	// Load Word unsigned
		case 0x28: *puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f) | MIPS_REG((uiMIPSword>>16)&0x1f); return 0; // SB; return 0;	// I
		case 0x29: *puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f) | MIPS_REG((uiMIPSword>>16)&0x1f); return 0; // SH; return 0;	// I
		case 0x2A: *puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f) | MIPS_REG((uiMIPSword>>16)&0x1f); return 0; // SWL; return 0;
		case 0x2B: *puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f) | MIPS_REG((uiMIPSword>>16)&0x1f); return 0; // SW; return 0;	// I
		case 0x2C: *puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f) | MIPS_REG((uiMIPSword>>16)&0x1f); return 0; // SDL; return 0;
		case 0x2D: *puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f) | MIPS_REG((uiMIPSword>>16)&0x1f); return 0; // SDR; return 0;
		case 0x2E: *puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f) | MIPS_REG((uiMIPSword>>16)&0x1f); return 0; // SWR; return 0;
		case 0x2F: return 0; // CACHE; return 0;
		case 0x30: *puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f); return 0; // LL; return 0;	// Load Linked Word atomic Read-Modify-Write ops
		case 0x31: *puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f); return 0; // LWC1; return 0;	// Load Word to co processor 1
		case 0x34: *puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f); return 0; // LLD; return 0;	// Load Linked Dbl Word atomic Read-Modify-Write ops
		case 0x35: *puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f); return 0; // LDC1; return 0;
		case 0x37: *puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f); return 0; // LD; return 0; 	// Load Double word
		case 0x38: *puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f) | MIPS_REG((uiMIPSword>>16)&0x1f) | MIPS_REG_LL; return 0; // SC; return 0;	// Store Linked Word atomic Read-Modify-Write ops
		case 0x39: *puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f); return 0; // SWC1; return 0;	// Store Word from co processor 1 to memory
		case 0x3C: *puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f) | MIPS_REG((uiMIPSword>>16)&0x1f) | MIPS_REG_LL; return 0; // SCD; return 0;	// Store Conditional Double Word
		case 0x3D: *puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f);
					*puiFPUregs_in |= MIPS_REG((uiMIPSword>>16)&0x1f); return 0; // SDC1; return 0;
		case 0x3F: *puiCPUregs_in |= MIPS_REG((uiMIPSword>>21)&0x1f) | MIPS_REG((uiMIPSword>>16)&0x1f); return 0; // SD; return 0; 	// Store Double word
	}

	return 1;
}

Instruction_e ops_type(const uint32_t uiMIPSword)
{
	uint32_t op=uiMIPSword>>26;
	uint32_t op2;

	if (0 == uiMIPSword) return NO_OP;

	switch(op)
	{
		case 0x00:
	        op2=uiMIPSword&0x3f;
	        switch(op2)
	        {
	          case 0x00: return SLL;
	          case 0x02: return SRL;
	          case 0x03: return SRA;
	          case 0x04: return SLLV;
	          case 0x07: return SRAV;
	          case 0x08: return JR;
	          case 0x09: return JALR;
	          //case 0x0A: printf("detected MOVZ\n"); return UNKNOWN;	// MOVZ in MIPS IV
	          //case 0x0B: printf("detected MOVN\n"); return UNKNOWN;	// MOVN in MIPS IV
	          case 0x0C: return SYSCALL;
	          case 0x0D: return BREAK;
	          case 0x0F: return SYNC;
	          case 0x10: return MFHI;
	          case 0x11: return MTHI;
	          case 0x12: return MFLO;
	          case 0x13: return MTLO;
	          case 0x14: return DSLLV;
	          case 0x16: return DSRLV;
	          case 0x17: return DSRAV;
	          case 0x18: return MULT;
	          case 0x19: return MULTU;
	          case 0x1A: return DIV;
	          case 0x1B: return DIVU;
	          case 0x1C: return DMULT;
	          case 0x1D: return DMULTU;
	          case 0x1E: return DDIV;
	          case 0x1F: return DDIVU;
	          case 0x20: return ADD;
	          case 0x21: return ADDU;
	          case 0x22: return SUB;
	          case 0x23: return SUBU;
	          case 0x24: return AND;
	          case 0x25: return OR;
	          case 0x26: return XOR;
	          case 0x27: return NOR;
	          case 0x2A: return SLT;
	          case 0x2B: return SLTU;
	          case 0x2C: return DADD;
	          case 0x2D: return DADDU;
	          case 0x2E: return DSUB;
	          case 0x2F: return DSUBU;
	          case 0x30: return TGE;
	          case 0x31: return TGEU;
	          case 0x32: return TLT;
	          case 0x33: return TLTU;
	          case 0x34: return TEQ;
	          case 0x36: return TNE;
	          case 0x38: return DSLL;
	          case 0x3A: return DSRL;
	          case 0x3B: return DSRA;
	          case 0x3C: return DSLL32;
	          case 0x3E: return DSRL32;
	          case 0x3F: return DSRA32;
	        }
	        break;

	case 0x01:
		op2=(uiMIPSword>>16)&0x1f;
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

#define M_Rs(x) ((x>>21)&0x1f)
#define M_Rt(x) ((x>>16)&0x1f)
#define M_Rd(x) ((x>>11)&0x1f)
#define IMM(x,y) ( ( ( (int32_t)(x) & ( (1 << (y)) - 1) ) << (32 -(y)) ) / (1 << (32 - (y)) ) )
#define IMMU(x,y) ( (uint32_t)(x) & ( (1 << (y)) - 1) )

uint32_t mips_decode(const uint32_t uiMIPSword, Instruction_t* const ins)
{
	uint32_t op=uiMIPSword>>26;
	uint32_t op2;

	switch(op)
	{
	case 0x00:
		op2=uiMIPSword&0x3f;
		switch(op2)
		{
		case 0x00:

			if (0 == uiMIPSword)
			{
				ins->instruction = NO_OP;
			}
			else
			{
				ins->instruction = SLL;
				ins->shiftType = LOGICAL_LEFT;
				ins->I = 1;
				ins->shift = (uiMIPSword>>6)&0x1f;
				ins->Rd1.regID = M_Rd(uiMIPSword);
				ins->R1.regID = M_Rt(uiMIPSword);
			}


			return 0;
		case 0x02:
			ins->instruction = SRL;
			ins->shiftType = LOGICAL_RIGHT;
			ins->I = 1;
			ins->shift = (uiMIPSword>>6)&0x1f;
			ins->Rd1.regID = M_Rd(uiMIPSword);
			ins->R1.regID = M_Rt(uiMIPSword);
			return 0;
		case 0x03:
			ins->instruction = SRA;
			ins->shiftType = ARITHMETIC_RIGHT;
			ins->I = 1;
			ins->shift = (uiMIPSword>>6)&0x1f;
			ins->Rd1.regID = M_Rd(uiMIPSword);
			ins->R1.regID = M_Rt(uiMIPSword);
			return 0;
		case 0x04:
			ins->instruction = SLLV;
			ins->shiftType = LOGICAL_LEFT;
			ins->I = 0;
			ins->Rd1.regID = M_Rd(uiMIPSword);
			ins->R1.regID = M_Rt(uiMIPSword);
			ins->R2.regID = M_Rs(uiMIPSword);
			return 0;
		case 0x07:
			ins->instruction = SRAV;
			ins->shiftType = ARITHMETIC_RIGHT;
			ins->I = 0;
			ins->Rd1.regID = M_Rd(uiMIPSword);
			ins->R1.regID = M_Rt(uiMIPSword);
			ins->R2.regID = M_Rs(uiMIPSword);
			return 0;
		case 0x08:
			ins->instruction = JR;
			ins->I = 0;
			ins->Ln = 0;
			ins->R1.regID = M_Rs(uiMIPSword);
			return 0;

		case 0x09:
			ins->instruction = JALR;
			ins->I = 0;
			ins->Ln = 1;
			ins->R1.regID = M_Rs(uiMIPSword);
			return 0;

		case 0x0C:
			ins->instruction =  SYSCALL;
			return 0;
		case 0x0D:
			ins->instruction =  BREAK;
			return 0;
		case 0x0F:
			ins->instruction =  SYNC;
			return 0;
		case 0x10:
			ins->instruction =  MFHI;
			ins->Rd1.regID = M_Rd(uiMIPSword);
			ins->R1.regID = REG_MULTHI;
			return 0;
		case 0x11:
			ins->instruction =  MTHI;
			ins->R1.regID = M_Rs(uiMIPSword);
			ins->Rd1.regID = REG_MULTHI;
			return 0;
		case 0x12:
			ins->instruction =  MFLO;
			ins->Rd1.regID = M_Rd(uiMIPSword);
			ins->R1.regID = REG_MULTLO;
			return 0;
		case 0x13:
			ins->instruction =  MTLO;
			ins->R1.regID = M_Rs(uiMIPSword);
			ins->Rd1.regID = REG_MULTLO;
			return 0;
		case 0x14:
			ins->instruction =  DSLLV;
			ins->R1.regID = M_Rt(uiMIPSword);
			ins->R2.regID = M_Rs(uiMIPSword);
			ins->Rd1.regID = M_Rd(uiMIPSword);
			return 0;
		case 0x16:
			ins->instruction =  DSRLV;
			ins->R1.regID = M_Rt(uiMIPSword);
			ins->R2.regID = M_Rs(uiMIPSword);
			ins->Rd1.regID = M_Rd(uiMIPSword);
			return 0;
		case 0x17:
			ins->instruction =  DSRAV;
			ins->R1.regID = M_Rt(uiMIPSword);
			ins->R2.regID = M_Rs(uiMIPSword);
			ins->Rd1.regID = M_Rd(uiMIPSword);
			return 0;
		case 0x18:
			ins->instruction =  MULT;
			ins->R1.regID = M_Rs(uiMIPSword);
			ins->R2.regID = M_Rt(uiMIPSword);
			return 0;
		case 0x19:
			ins->instruction =  MULTU;
			ins->R1.regID = M_Rs(uiMIPSword);
			ins->R2.regID = M_Rt(uiMIPSword);
			return 0;
		case 0x1A:
			ins->instruction =  DIV;
			ins->R1.regID = M_Rs(uiMIPSword);
			ins->R2.regID = M_Rt(uiMIPSword);
			return 0;
		case 0x1B:
			ins->instruction =  DIVU;
			ins->R1.regID = M_Rs(uiMIPSword);
			ins->R2.regID = M_Rt(uiMIPSword);
			return 0;
		case 0x1C:
			ins->instruction =  DMULT;
			ins->R1.regID = M_Rs(uiMIPSword);
			ins->R2.regID = M_Rt(uiMIPSword);
			return 0;
		case 0x1D:
			ins->instruction =  DMULTU;
			ins->R1.regID = M_Rs(uiMIPSword);
			ins->R2.regID = M_Rt(uiMIPSword);
			return 0;
		case 0x1E:
			ins->instruction =  DDIV;
			ins->R1.regID = M_Rs(uiMIPSword);
			ins->R2.regID = M_Rt(uiMIPSword);
			return 0;
		case 0x1F:
			ins->instruction =  DDIVU;
			ins->R1.regID = M_Rs(uiMIPSword);
			ins->R2.regID = M_Rt(uiMIPSword);
			return 0;
		case 0x20:
			ins->instruction =  ADD;
			ins->I = 0;
			ins->R1.regID = M_Rs(uiMIPSword);
			ins->R2.regID = M_Rt(uiMIPSword);
			ins->Rd1.regID = M_Rd(uiMIPSword);
			return 0;
		case 0x21:
			ins->instruction =  ADDU;
			ins->I = 0;
			ins->R1.regID = M_Rs(uiMIPSword);
			ins->R2.regID = M_Rt(uiMIPSword);
			ins->Rd1.regID = M_Rd(uiMIPSword);
			return 0;
		case 0x22:
			ins->instruction =  SUB;
			ins->R1.regID = M_Rs(uiMIPSword);
			ins->R2.regID = M_Rt(uiMIPSword);
			ins->Rd1.regID = M_Rd(uiMIPSword);
			return 0;
		case 0x23:
			ins->instruction =  SUBU;
			ins->R1.regID = M_Rs(uiMIPSword);
			ins->R2.regID = M_Rt(uiMIPSword);
			ins->Rd1.regID = M_Rd(uiMIPSword);
			return 0;
		case 0x24:
			ins->instruction =  AND;
			ins->R1.regID = M_Rs(uiMIPSword);
			ins->R2.regID = M_Rt(uiMIPSword);
			ins->Rd1.regID = M_Rd(uiMIPSword);
			return 0;
		case 0x25:
			ins->instruction =  OR;
			ins->R1.regID = M_Rs(uiMIPSword);
			ins->R2.regID = M_Rt(uiMIPSword);
			ins->Rd1.regID = M_Rd(uiMIPSword);
			return 0;
		case 0x26:
			ins->instruction =  XOR;
			ins->R1.regID = M_Rs(uiMIPSword);
			ins->R2.regID = M_Rt(uiMIPSword);
			ins->Rd1.regID = M_Rd(uiMIPSword);
			return 0;
		case 0x27:
			ins->instruction =  NOR;
			ins->R1.regID = M_Rs(uiMIPSword);
			ins->R2.regID = M_Rt(uiMIPSword);
			ins->Rd1.regID = M_Rd(uiMIPSword);
			return 0;
		case 0x2A:
			ins->instruction =  SLT;
			ins->Rd1.regID = M_Rd(uiMIPSword);
			ins->R1.regID = M_Rs(uiMIPSword);
			ins->R2.regID = M_Rt(uiMIPSword);
			return 0;
		case 0x2B:
			ins->instruction =  SLTU;
			ins->Rd1.regID = M_Rd(uiMIPSword);
			ins->R1.regID = M_Rs(uiMIPSword);
			ins->R2.regID = M_Rt(uiMIPSword);
			return 0;
		case 0x2C:
			ins->instruction =  DADD;
			ins->R1.regID = M_Rs(uiMIPSword);
			ins->R2.regID= M_Rd(uiMIPSword);
			return 0;
		case 0x2D:
			ins->instruction =  DADDU;
			ins->R1.regID = M_Rs(uiMIPSword);
			ins->R2.regID = M_Rt(uiMIPSword);
			ins->Rd1.regID = M_Rd(uiMIPSword);
			return 0;
		case 0x2E:
			ins->instruction =  DSUB;
			ins->R1.regID = M_Rs(uiMIPSword);
			ins->R2.regID = M_Rt(uiMIPSword);
			ins->Rd1.regID = M_Rd(uiMIPSword);
			return 0;
		case 0x2F:
			ins->instruction =  DSUBU;
			ins->R1.regID = M_Rs(uiMIPSword);
			ins->R2.regID = M_Rt(uiMIPSword);
			ins->Rd1.regID = M_Rd(uiMIPSword);
			return 0;
		case 0x30:
			ins->instruction =  TGE;
			ins->R1.regID = M_Rs(uiMIPSword);
			ins->R2.regID = M_Rt(uiMIPSword);
			return 0;
		case 0x31:
			ins->instruction =  TGEU;
			ins->R1.regID = M_Rs(uiMIPSword);
			ins->R2.regID = M_Rt(uiMIPSword);
			return 0;
		case 0x32:
			ins->instruction =  TLT;
			ins->R1.regID = M_Rs(uiMIPSword);
			ins->R2.regID = M_Rt(uiMIPSword);
			return 0;
		case 0x33:
			ins->instruction =  TLTU;
			ins->R1.regID = M_Rs(uiMIPSword);
			ins->R2.regID = M_Rt(uiMIPSword);
			return 0;
		case 0x34:
			ins->instruction =  TEQ;
			ins->R1.regID = M_Rs(uiMIPSword);
			ins->R2.regID = M_Rt(uiMIPSword);
			return 0;
		case 0x36:
			ins->instruction =  TNE;
			ins->R1.regID = M_Rs(uiMIPSword);
			ins->R2.regID = M_Rt(uiMIPSword);
			return 0;
		case 0x38:
			ins->instruction =  DSLL;
			ins->R1.regID = M_Rs(uiMIPSword);
			ins->R2.regID = M_Rt(uiMIPSword);
			ins->Rd1.regID = M_Rd(uiMIPSword);
			return 0;
		case 0x3A:
			ins->instruction =  DSRL;
			ins->R1.regID = M_Rs(uiMIPSword);
			ins->R2.regID = M_Rt(uiMIPSword);
			ins->Rd1.regID = M_Rd(uiMIPSword);
			return 0;
		case 0x3B:
			ins->instruction =  DSRA;
			ins->R1.regID = M_Rs(uiMIPSword);
			ins->R2.regID = M_Rt(uiMIPSword);
			ins->Rd1.regID = M_Rd(uiMIPSword);
			return 0;
		case 0x3C:
			ins->instruction =  DSLL32;
			ins->R1.regID = M_Rs(uiMIPSword);
			ins->R2.regID = M_Rt(uiMIPSword);
			ins->Rd1.regID = M_Rd(uiMIPSword);
			return 0;
		case 0x3E:
			ins->instruction =  DSRL32;
			ins->R1.regID = M_Rs(uiMIPSword);
			ins->R2.regID = M_Rt(uiMIPSword);
			ins->Rd1.regID = M_Rd(uiMIPSword);
			return 0;
		case 0x3F:
			ins->instruction =  DSRA32;
			ins->R1.regID = M_Rs(uiMIPSword);
			ins->R2.regID = M_Rt(uiMIPSword);
			ins->Rd1.regID = M_Rd(uiMIPSword);
			return 0;
		}
		break;

	case 0x01:
		op2=(uiMIPSword>>16)&0x1f;
		switch(op2)
		{
		case 0x00:
			ins->instruction =  BLTZ;
			ins->R1.regID = M_Rs(uiMIPSword);
			ins->offset = IMM(uiMIPSword,16);
			ins->cond = LTZ;
			ins->I = 1;
			return 0; 	// I
		case 0x01:
			ins->instruction =  BGEZ;
			ins->R1.regID = M_Rs(uiMIPSword);
			ins->offset = IMM(uiMIPSword,16);
			ins->cond = GEZ;
			ins->I = 1;
			return 0; 	// I
		case 0x02:
			ins->instruction =  BLTZL;
			ins->R1.regID = M_Rs(uiMIPSword);
			ins->offset = IMM(uiMIPSword,16);
			ins->cond = LTZ;
			ins->I = 1;
			return 0;
		case 0x03:
			ins->instruction =  BGEZL;
			ins->R1.regID = M_Rs(uiMIPSword);
			ins->offset = IMM(uiMIPSword,16);
			ins->cond = GEZ;
			ins->I = 1;
			return 0;
		case 0x08:
			ins->instruction =  TGEI;
			ins->R1.regID = M_Rs(uiMIPSword);
			ins->immediate = IMM(uiMIPSword,16);
			ins->I = 1;
			return 0;
		case 0x09:
			ins->instruction =  TGEIU;
			ins->R1.regID = M_Rs(uiMIPSword);
			ins->immediate = IMMU(uiMIPSword,16);
			ins->I = 1;
			return 0;
		case 0x0A:
			ins->instruction =  TLTI;
			ins->R1.regID = M_Rs(uiMIPSword);
			ins->immediate = IMM(uiMIPSword,16);
			ins->I = 1;
			return 0;
		case 0x0B:
			ins->instruction =  TLTIU;
			ins->R1.regID = M_Rs(uiMIPSword);
			ins->immediate = IMMU(uiMIPSword,16);
			ins->I = 1;
			return 0;
		case 0x0C:
			ins->instruction =  TEQI;
			ins->R1.regID = M_Rs(uiMIPSword);
			ins->immediate = IMM(uiMIPSword,16);
			ins->I = 1;
			return 0;
		case 0x0E:
			ins->instruction =  TNEI;
			ins->R1.regID = M_Rs(uiMIPSword);
			ins->immediate = IMM(uiMIPSword,16);
			ins->I = 1;
			return 0;
		case 0x10:
			ins->instruction =  BLTZAL;
			ins->R1.regID = M_Rs(uiMIPSword);
			ins->offset = IMM(uiMIPSword,16);
			ins->cond = LTZ;
			ins->Ln = 1;
			ins->I = 1;
			return 0; 	// I and link
		case 0x11:
			ins->instruction =  BGEZAL;
			ins->R1.regID = M_Rs(uiMIPSword);
			ins->offset = IMM(uiMIPSword,16);
			ins->cond = GEZ;
			ins->Ln = 1;
			ins->I = 1;
			return 0; 	// I and link
		case 0x12:
			ins->instruction =  BLTZALL;
			ins->R1.regID = M_Rs(uiMIPSword);
			ins->offset = IMM(uiMIPSword,16);
			ins->cond = LTZ;
			ins->Ln = 1;
			ins->I = 1;
			return 0; 	// I and link likely
		case 0x13:
			ins->instruction =  BGEZALL;
			ins->R1.regID = M_Rs(uiMIPSword);
			ins->offset = IMM(uiMIPSword,16);
			ins->cond = GEZ;
			ins->Ln = 1;
			ins->I = 1;
			return 0; 	// I and link likely
		} break;

	case 0x02:
		ins->instruction = J;
		ins->offset = IMMU(uiMIPSword, 26);
		return 0;
	case 0x03:
		ins->instruction = JAL;
		ins->Ln = 1;
		ins->offset = IMMU(uiMIPSword, 26);
		return 0;
	case 0x04:
		ins->instruction = BEQ;
		ins->R1.regID = M_Rs(uiMIPSword);
		ins->R2.regID = M_Rt(uiMIPSword);
		ins->offset = IMM(uiMIPSword, 16);
		ins->cond = EQ;
		return 0; 	// I
	case 0x05:
		ins->instruction = BNE;
		ins->R1.regID = M_Rs(uiMIPSword);
		ins->R2.regID = M_Rt(uiMIPSword);
		ins->offset = IMM(uiMIPSword, 16);
		ins->cond = NE;
		return 0; 	// I
	case 0x06:
		ins->instruction = BLEZ;
		ins->R1.regID = M_Rs(uiMIPSword);
		ins->R2.regID = M_Rt(uiMIPSword);
		ins->offset = IMM(uiMIPSword, 16);
		ins->cond = LEZ;
		return 0;
	case 0x07:
		ins->instruction = BGTZ;
		ins->R1.regID = M_Rs(uiMIPSword);
		ins->R2.regID = M_Rt(uiMIPSword);
		ins->offset = IMM(uiMIPSword, 16);
		ins->cond = GTZ;
		return 0;
	case 0x08:
		ins->instruction = ADDI;
		ins->R1.regID = M_Rs(uiMIPSword);
		ins->Rd1.regID = M_Rt(uiMIPSword);
		ins->immediate = IMM(uiMIPSword, 16);
		ins->I = 1;
		return 0; 	// I
	case 0x09:
		ins->instruction = ADDIU;
		ins->Rd1.regID = M_Rt(uiMIPSword);
		ins->R1.regID = M_Rs(uiMIPSword);
		ins->immediate = IMM(uiMIPSword, 16);
		ins->I = 1;
		return 0; 	// I

	case 0x0A:
		ins->instruction =  SLTI;
		ins->R1.regID = M_Rs(uiMIPSword);
		ins->Rd1.regID = M_Rt(uiMIPSword);
		ins->immediate = IMM(uiMIPSword, 16);
		ins->I = 1;
		return 0; 	// I
	case 0x0B:
		ins->instruction =  SLTIU;
		ins->R1.regID = M_Rs(uiMIPSword);
		ins->Rd1.regID = M_Rt(uiMIPSword);
		ins->immediate = IMM(uiMIPSword, 16);
		ins->I = 1;
		return 0; 	// I
	case 0x0C:
		ins->instruction =  ANDI;
		ins->Rd1.regID = M_Rt(uiMIPSword);
		ins->R1.regID = M_Rs(uiMIPSword);
		ins->immediate = IMMU(uiMIPSword, 16);
		ins->I = 1;
		return 0;  	// I
	case 0x0D:
		ins->instruction =  ORI;
		ins->Rd1.regID = M_Rt(uiMIPSword);
		ins->R1.regID = M_Rs(uiMIPSword);
		ins->immediate = IMMU(uiMIPSword, 16);
		ins->I = 1;
		return 0; 	// I
	case 0x0E:
		ins->instruction =  XORI;
		ins->Rd1.regID = M_Rt(uiMIPSword);
		ins->R1.regID = M_Rs(uiMIPSword);
		ins->immediate = IMMU(uiMIPSword, 16);
		ins->I = 1;
		return 0; 	// I
	case 0x0F:
		ins->instruction =  LUI;
		ins->Rd1.regID = M_Rt(uiMIPSword);
		ins->immediate = IMMU(uiMIPSword, 16);
		ins->I = 1;
		return 0; 	// I
	case 0x10:
		op2=(uiMIPSword>>21)&0x1f;
		switch(op2)
		{
		case 0x00:
			ins->instruction =  MFC0;
			ins->R1.regID = REG_CO + M_Rd(uiMIPSword);
			ins->Rd1.regID = M_Rt(uiMIPSword);
			return 0;
		case 0x04:
			ins->instruction =  MTC0;
			ins->Rd1.regID = REG_CO + M_Rd(uiMIPSword);
			ins->R1.regID = M_Rt(uiMIPSword);
			return 0;
		case 0x10: //
			ins->C0 = (uiMIPSword >> 25)&1;
			switch(uiMIPSword&0x3f)
			{
			case 0x01:
			ins->instruction =  TLBR;
			return 0;
			case 0x02:
			ins->instruction =  TLBWI;
			return 0;
			case 0x06:
			ins->instruction =  TLBWR;
			return 0;
			case 0x08:
			ins->instruction =  TLBP;
			return 0;
			case 0x18:
			ins->instruction =  ERET;
			return 0;
			}
		}
		break;
			return 0;

	case 0x11:
		op2=(uiMIPSword>>21)&0x1f;
		switch(op2)
		{
		case 0x00:
			ins->instruction =  MFC1;
			ins->R1.regID = REG_FP + M_Rd(uiMIPSword);
			ins->Rd1.regID = M_Rt(uiMIPSword);
			return 0;
		case 0x01:
			ins->instruction =  DMFC1;
			ins->R1.regID = REG_FP + M_Rd(uiMIPSword);
			ins->Rd1.regID = M_Rt(uiMIPSword);
			return 0;
		case 0x02:
			ins->instruction =  CFC1;
			ins->Rd1.regID = M_Rt(uiMIPSword);
			ins->R1.regID = REG_FP + M_Rd(uiMIPSword);
			return 0;
		case 0x04:
			ins->instruction =  MTC1;
			ins->Rd1.regID = REG_FP + M_Rd(uiMIPSword);
			ins->R1.regID = M_Rt(uiMIPSword);
			return 0;
		case 0x05:
			ins->instruction =  DMTC1;
			ins->Rd1.regID = REG_FP + M_Rd(uiMIPSword);
			ins->R1.regID = M_Rt(uiMIPSword);
			return 0;
		case 0x06:
			ins->instruction =  CTC1;
			ins->Rd1.regID = REG_FP + M_Rd(uiMIPSword);
			ins->R1.regID = M_Rt(uiMIPSword);
			return 0;
		case 0x08: //
			ins->offset = IMM(uiMIPSword, 16);
			switch((uiMIPSword>>16)&0x3)
			{
			case 0x00:
			ins->instruction =  BC1F;
			return 0;
			case 0x01:
			ins->instruction =  BC1T;
			return 0;
			case 0x02:
			ins->instruction =  BC1FL;
			return 0;
			case 0x03:
			ins->instruction =  BC1TL;
			return 0;
			}break;
			return 0;

		case 0x10:
			switch(uiMIPSword&0x3f)
			{
			case 0x00:
			ins->instruction =  ADD_S;
			ins->Rd1.regID = REG_FP + ((uiMIPSword>>6)&0x1f);
			ins->R1.regID = REG_FP + ((uiMIPSword>>11)&0x1f);
			ins->R2.regID = REG_FP + ((uiMIPSword>>16)&0x1f);
			return 0;
			case 0x01:
			ins->instruction =  SUB_S;
			ins->Rd1.regID = REG_FP + ((uiMIPSword>>6)&0x1f);
			ins->R1.regID = REG_FP + ((uiMIPSword>>11)&0x1f);
			ins->R2.regID = REG_FP + ((uiMIPSword>>16)&0x1f);
			return 0;
			case 0x02:
			ins->instruction =  MUL_S;

			return 0;
			case 0x03:
			ins->instruction =  DIV_S;
			return 0;
			case 0x04:
			ins->instruction =  SQRT_S;
			return 0;
			case 0x05:
			ins->instruction =  ABS_S;
			return 0;
			case 0x06:
			ins->instruction =  MOV_S;
			return 0;
			case 0x07:
			ins->instruction =  NEG_S;
			return 0;
			case 0x08:
			ins->instruction =  ROUND_L_S;
			return 0;
			case 0x09:
			ins->instruction =  TRUNC_L_S;
			return 0;
			case 0x0A:
			ins->instruction =  CEIL_L_S;
			return 0;
			case 0x0B:
			ins->instruction =  FLOOR_L_S;
			return 0;
			case 0x0C:
			ins->instruction =  ROUND_W_S;
			return 0;
			case 0x0D:
			ins->instruction =  TRUNC_W_S;
			return 0;
			case 0x0E:
			ins->instruction =  CEIL_W_S;
			return 0;
			case 0x0F:
			ins->instruction =  FLOOR_W_S;
			return 0;
			case 0x21:
			ins->instruction =  CVT_D_S;
			return 0;
			case 0x24:
			ins->instruction =  CVT_W_S;
			return 0;
			case 0x25:
			ins->instruction =  CVT_L_S;
			return 0;
			case 0x30:
			ins->instruction =  C_F_S;
			return 0;
			case 0x31:
			ins->instruction =  C_UN_S;
			return 0;
			case 0x32:
			ins->instruction =  C_EQ_S;
			return 0;
			case 0x33:
			ins->instruction =  C_UEQ_S;
			return 0;
			case 0x34:
			ins->instruction =  C_OLT_S;
			return 0;
			case 0x35:
			ins->instruction =  C_ULT_S;
			return 0;
			case 0x36:
			ins->instruction =  C_OLE_S;
			return 0;
			case 0x37:
			ins->instruction =  C_ULE_S;
			return 0;
			case 0x38:
			ins->instruction =  C_SF_S;
			return 0;
			case 0x39:
			ins->instruction =  C_NGLE_S;
			return 0;
			case 0x3A:
			ins->instruction =  C_SEQ_S;
			return 0;
			case 0x3B:
			ins->instruction =  C_NGL_S;
			return 0;
			case 0x3C:
			ins->instruction =  C_LT_S;
			return 0;
			case 0x3D:
			ins->instruction =  C_NGE_S;
			return 0;
			case 0x3E:
			ins->instruction =  C_LE_S;
			return 0;
			case 0x3F:
			ins->instruction =  C_NGT_S;
			return 0;
			}break;
			return 0;
		case 0x11:
			switch(uiMIPSword&0x3f)
			{
			case 0x00:
			ins->instruction =  ADD_D;
			ins->Rd1.regID = REG_FP + ((uiMIPSword>>6)&0x1f);
			ins->R1.regID = REG_FP + ((uiMIPSword>>11)&0x1f);
			ins->R2.regID = REG_FP + ((uiMIPSword>>16)&0x1f);
			return 0;
			case 0x01:
			ins->instruction =  SUB_D;
			ins->Rd1.regID = REG_FP + ((uiMIPSword>>6)&0x1f);
			ins->R1.regID = REG_FP + ((uiMIPSword>>11)&0x1f);
			ins->R2.regID = REG_FP + ((uiMIPSword>>16)&0x1f);
			return 0;
			case 0x02:
			ins->instruction =  MUL_D;
			return 0;
			case 0x03:
			ins->instruction =  DIV_D;
			return 0;
			case 0x04:
			ins->instruction =  SQRT_D;
			return 0;
			case 0x05:
			ins->instruction =  ABS_D;
			return 0;
			case 0x06:
			ins->instruction =  MOV_D;
			return 0;
			case 0x07:
			ins->instruction =  NEG_D;
			return 0;
			case 0x08:
			ins->instruction =  ROUND_L_D;
			return 0;
			case 0x09:
			ins->instruction =  TRUNC_L_D;
			return 0;
			case 0x0A:
			ins->instruction =  CEIL_L_D;
			return 0;
			case 0x0B:
			ins->instruction =  FLOOR_L_D;
			return 0;
			case 0x0C:
			ins->instruction =  ROUND_W_D;
			return 0;
			case 0x0D:
			ins->instruction =  TRUNC_W_D;
			return 0;
			case 0x0E:
			ins->instruction =  CEIL_W_D;
			return 0;
			case 0x0F:
			ins->instruction =  FLOOR_W_D;
			return 0;
			case 0x20:
			ins->instruction =  CVT_S_D;
			return 0;
			case 0x24:
			ins->instruction =  CVT_W_D;
			return 0;
			case 0x25:
			ins->instruction =  CVT_L_D;
			return 0;
			case 0x30:
			ins->instruction =  C_F_D;
			return 0;
			case 0x31:
			ins->instruction =  C_UN_D;
			return 0;
			case 0x32:
			ins->instruction =  C_EQ_D;
			return 0;
			case 0x33:
			ins->instruction =  C_UEQ_D;
			return 0;
			case 0x34:
			ins->instruction =  C_OLT_D;
			return 0;
			case 0x35:
			ins->instruction =  C_ULT_D;
			return 0;
			case 0x36:
			ins->instruction =  C_OLE_D;
			return 0;
			case 0x37:
			ins->instruction =  C_ULE_D;
			return 0;
			case 0x38:
			ins->instruction =  C_SF_D;
			return 0;
			case 0x39:
			ins->instruction =  C_NGLE_D;
			return 0;
			case 0x3A:
			ins->instruction =  C_SEQ_D;
			return 0;
			case 0x3B:
			ins->instruction =  C_NGL_D;
			return 0;
			case 0x3C:
			ins->instruction =  C_LT_D;
			return 0;
			case 0x3D:
			ins->instruction =  C_NGE_D;
			return 0;
			case 0x3E:
			ins->instruction =  C_LE_D;
			return 0;
			case 0x3F:
			ins->instruction =  C_NGT_D;
			return 0;
			} break;
			return 0;
		case 0x14:
			switch(uiMIPSword&0x3f)
			{
			case 0x20:
			ins->instruction =  CVT_S_W;
			return 0;
			case 0x21:
			ins->instruction =  CVT_D_W;
			return 0;
			}
			break;
			return 0;

		case 0x15: switch(uiMIPSword&0x3f)
			{
			case 0x20:
			ins->instruction =  CVT_S_L;
			return 0;
			case 0x21:
			ins->instruction =  CVT_D_L;
			return 0;
			}
			break;
			return 0;
		}break;
			return 0;

	case 0x14:
			ins->instruction =  BEQL;
			ins->cond = EQ;
			return 0;
	case 0x15:
			ins->instruction =  BNEL;
			ins->cond = NE;
			return 0;
	case 0x16:
			ins->instruction =  BLEZL;
			ins->cond = LEZ;
			return 0;
	case 0x17:
			ins->instruction =  BGTZL;
			ins->cond = GTZ;
			return 0;
	case 0x18:
			ins->instruction =  DADDI;
			ins->Rd1.regID = M_Rt(uiMIPSword);
			ins->R1.regID = M_Rs(uiMIPSword);
			ins->immediate = IMM(uiMIPSword, 16);
			ins->I = 1;
			return 0;
	case 0x19:
			ins->instruction =  DADDIU;
			ins->Rd1.regID = M_Rt(uiMIPSword);
			ins->R1.regID = M_Rs(uiMIPSword);
			ins->immediate = IMM(uiMIPSword, 16);
			ins->I = 1;
			return 0;
	case 0x1A:
			ins->instruction =  LDL;
			return 0;
	case 0x1B:
			ins->instruction =  LDR;
			return 0;
	case 0x20:
			ins->instruction =  LB;
			ins->Rd1.regID = M_Rt(uiMIPSword);
			ins->R1.regID = M_Rs(uiMIPSword);
			ins->immediate = IMM(uiMIPSword, 16);
			ins->I = 1;
			return 0; 	// Load Byte
	case 0x21:
			ins->instruction =  LH;
			ins->Rd1.regID = M_Rt(uiMIPSword);
			ins->R1.regID = M_Rs(uiMIPSword);
			ins->immediate = IMM(uiMIPSword, 16);
			ins->I = 1;
			return 0; 	// Load Halfword
	case 0x22:
			ins->instruction =  LWL;

			return 0;
	case 0x23:
			ins->instruction =  LW;
			ins->Rd1.regID = M_Rt(uiMIPSword);
			ins->R1.regID = M_Rs(uiMIPSword);
			ins->immediate = IMM(uiMIPSword, 16);
			ins->I = 1;
			return 0; 	// Load Word
	case 0x24:
			ins->instruction =  LBU;
			ins->Rd1.regID = M_Rt(uiMIPSword);
			ins->R1.regID = M_Rs(uiMIPSword);
			ins->immediate = IMM(uiMIPSword, 16);
			return 0; 	// Load Unsigned Byte
	case 0x25:
			ins->instruction =  LHU;
			ins->Rd1.regID = M_Rt(uiMIPSword);
			ins->R1.regID = M_Rs(uiMIPSword);
			ins->immediate = IMM(uiMIPSword, 16);
			return 0; 	// Load Halfword unsigned
	case 0x26:
			ins->instruction =  LWR;
			return 0;
	case 0x27:
			ins->instruction =  LWU;
			ins->Rd1.regID = M_Rt(uiMIPSword);
			ins->R1.regID = M_Rs(uiMIPSword);
			ins->immediate = IMM(uiMIPSword, 16);
			ins->I = 1;
			return 0; 	// Load Word unsigned
	case 0x28:
			ins->instruction =  SB;
			ins->R2.regID = M_Rt(uiMIPSword);
			ins->R1.regID = M_Rs(uiMIPSword);
			ins->immediate = IMM(uiMIPSword, 16);
			return 0; 	// I
	case 0x29:
			ins->instruction =  SH;
			ins->R2.regID = M_Rt(uiMIPSword);
			ins->R1.regID = M_Rs(uiMIPSword);
			ins->immediate = IMM(uiMIPSword, 16);
			return 0; 	// I
	case 0x2A:
			ins->instruction =  SWL;
			return 0;
	case 0x2B:
			ins->instruction =  SW;
			ins->R2.regID = M_Rt(uiMIPSword);
			ins->R1.regID = M_Rs(uiMIPSword);
			ins->immediate = IMM(uiMIPSword, 16);
			return 0; 	// I
	case 0x2C:
			ins->instruction =  SDL;
			return 0;
	case 0x2D:
			ins->instruction =  SDR;
			return 0;
	case 0x2E:
			ins->instruction =  SWR;
			return 0;
	case 0x2F:
			ins->instruction =  CACHE;
			return 0;
	case 0x30:
			ins->instruction =  LL;
			return 0; 	// Load Linked Word atomic Read-Modify-Write ops
	case 0x31:
			ins->instruction =  LWC1;
			return 0; 	// Load Word to co processor 1
	case 0x34:
			ins->instruction =  LLD;
			return 0; 	// Load Linked Dbl Word atomic Read-Modify-Write ops
	case 0x35:
			ins->instruction =  LDC1;
			return 0;
	case 0x37:
			ins->instruction =  LD;
			return 0;  	// Load Double word
	case 0x38:
			ins->instruction =  SC;
			return 0; 	// Store Linked Word atomic Read-Modify-Write ops
	case 0x39:
			ins->instruction =  SWC1;
			return 0; 	// Store Word from co processor 1 to memory
	case 0x3C:
			ins->instruction =  SCD;
			return 0; 	// Store Conditional Double Word
	case 0x3D:
			ins->instruction =  SDC1;
			return 0;
	case 0x3F:
			ins->instruction =  SD;
			ins->R2.regID = M_Rt(uiMIPSword);
			ins->R1.regID = M_Rs(uiMIPSword);
			ins->immediate = IMM(uiMIPSword, 16);
			return 0;  	// Store Double word
	}

	return 1;
}

void fprintf_mips(FILE* stream, const uint32_t x, const uint32_t uiMIPSword)
{
	uint32_t op=uiMIPSword>>26;
	uint32_t op2;

#if defined(SHOW_PRINT_MIPS_VALUE)
	fprintf(stream, "0x%08x 0x%08x", x, uiMIPSword);
#else
	fprintf(stream, "0x%08x ", x);
#endif

	switch(op)
	{
	case 0x00:
		op2=uiMIPSword&0x3f;
		switch(op2)
		{
			case 0x00: fprintf(stream, "\tSLL   \t" I_OP "\n", OP_I(uiMIPSword)); return;	// I
			case 0x02: fprintf(stream, "\tSRL   \t" I_OP "\n", OP_I(uiMIPSword)); return;	// I
			case 0x03: fprintf(stream, "\tSRA   \t" I_OP "\n", OP_I(uiMIPSword)); return;	// I
			case 0x04: fprintf(stream, "\tSLLV  \t" R_OP "\n", OP_R(uiMIPSword)); return;	// I
			case 0x07: fprintf(stream, "\tSRAV  \t" R_OP "\n", OP_R(uiMIPSword)); return;	// I
			case 0x08: fprintf(stream, "\tJR    \tr%d\n", (uiMIPSword>>21)&0x1f); return;	// J
			case 0x09: fprintf(stream, "\tJALR  \tr%d // PC=r%d, r%d=%d \n",
					(uiMIPSword>>21)&0x1f,
					(uiMIPSword>>21)&0x1f,
					(uiMIPSword>>11)&0x1f,
					x+8); return;	// J
			case 0x0C: fprintf(stream, "\tSYSCALL\t" I_OP "\n", OP_I(uiMIPSword)); return;	// I
			case 0x0D: fprintf(stream, "\tBREAK \t" I_OP "\n", OP_I(uiMIPSword)); return;	// I
			case 0x0F: fprintf(stream, "\tSYNC  \t" I_OP "\n", OP_I(uiMIPSword)); return;	// I
			case 0x10: fprintf(stream, "\tMFHI  \t" I_OP "\n", OP_I(uiMIPSword)); return;	// I
			case 0x11: fprintf(stream, "\tMTHI  \t" I_OP "\n", OP_I(uiMIPSword)); return;	// I
			case 0x12: fprintf(stream, "\tMFLO  \t" I_OP "\n", OP_I(uiMIPSword)); return;	// I
			case 0x13: fprintf(stream, "\tMTLO  \t" I_OP "\n", OP_I(uiMIPSword)); return;	// I
			case 0x14: fprintf(stream, "\tDSLLV \t" R_OP "\n", OP_R(uiMIPSword)); return;	// I
			case 0x16: fprintf(stream, "\tDSRLV \t" R_OP "\n", OP_R(uiMIPSword)); return;	// I
			case 0x17: fprintf(stream, "\tDSRAV \t" R_OP "\n", OP_R(uiMIPSword)); return;	// I
			case 0x18: fprintf(stream, "\tMULT  \t" R2_OP "\n", OP_R2(uiMIPSword)); return;	// I
			case 0x19: fprintf(stream, "\tMULTU \t" R2_OP "\n", OP_R2(uiMIPSword)); return;	// I
			case 0x1A: fprintf(stream, "\tDIV   \t" R2_OP "\n", OP_R2(uiMIPSword)); return;	// I
			case 0x1B: fprintf(stream, "\tDIVU  \t" R2_OP "\n", OP_R2(uiMIPSword)); return;	// I
			case 0x1C: fprintf(stream, "\tDMULT \t" R2_OP "\n", OP_R2(uiMIPSword)); return;	// I
			case 0x1D: fprintf(stream, "\tDMULTU\t" R2_OP "\n", OP_R2(uiMIPSword)); return;	// I
			case 0x1E: fprintf(stream, "\tDDIV  \t" R2_OP "\n", OP_R2(uiMIPSword)); return;	// I
			case 0x1F: fprintf(stream, "\tDDIVU \t" R2_OP "\n", OP_R2(uiMIPSword)); return;	// I
			case 0x20: fprintf(stream, "\tADD   \t" R_OP "\n", OP_R(uiMIPSword)); return;	// I
			case 0x21: fprintf(stream, "\tADDU  \t" R_OP "\n", OP_R(uiMIPSword)); return;	// I
			case 0x22: fprintf(stream, "\tSUB   \t" R_OP "\n", OP_R(uiMIPSword)); return;	// I
			case 0x23: fprintf(stream, "\tSUBU  \t" R_OP "\n", OP_R(uiMIPSword)); return;	// I
			case 0x24: fprintf(stream, "\tAND   \t" R_OP "\n", OP_R(uiMIPSword)); return;	// I
			case 0x25: fprintf(stream, "\tOR    \t" R_OP "\n", OP_R(uiMIPSword)); return;	// I
			case 0x26: fprintf(stream, "\tXOR   \t" R_OP "\n", OP_R(uiMIPSword)); return;	// I
			case 0x27: fprintf(stream, "\tNOR   \t" R_OP "\n", OP_R(uiMIPSword)); return;	// I
			case 0x2A: fprintf(stream, "\tSLT   \t" R2_OP "\n", OP_R2(uiMIPSword)); return;	// I
			case 0x2B: fprintf(stream, "\tSLTU  \t" R2_OP "\n", OP_R2(uiMIPSword)); return;	// I
			case 0x2C: fprintf(stream, "\tDADD  \t" R_OP "\n", OP_R(uiMIPSword)); return;	// I
			case 0x2D: fprintf(stream, "\tDADDU \t" R_OP "\n", OP_R(uiMIPSword)); return;	// I
			case 0x2E: fprintf(stream, "\tDSUB  \t" R_OP "\n", OP_R(uiMIPSword)); return;	// I
			case 0x2F: fprintf(stream, "\tDSUBU \t" R_OP "\n", OP_R(uiMIPSword)); return;	// I
			case 0x30: fprintf(stream, "\tTGE   \t" R2_OP "\n", OP_R2(uiMIPSword)); return;	// I
			case 0x31: fprintf(stream, "\tTGEU  \t" R2_OP "\n", OP_R2(uiMIPSword)); return;	// I
			case 0x32: fprintf(stream, "\tTLT   \t" R2_OP "\n", OP_R2(uiMIPSword)); return;	// I
			case 0x33: fprintf(stream, "\tTLTU  \t" R2_OP "\n", OP_R2(uiMIPSword)); return;	// I
			case 0x34: fprintf(stream, "\tTEQ   \t" R2_OP "\n", OP_R2(uiMIPSword)); return;	// I
			case 0x36: fprintf(stream, "\tTNE   \t" R2_OP "\n", OP_R2(uiMIPSword)); return;	// I
			case 0x38: fprintf(stream, "\tDSLL  \t" I_OP "\n", OP_I(uiMIPSword)); return;	// I
			case 0x3A: fprintf(stream, "\tDSRL  \t" I_OP "\n", OP_I(uiMIPSword)); return;	// I
			case 0x3B: fprintf(stream, "\tDSRA  \t" I_OP "\n", OP_I(uiMIPSword)); return;	// I
			case 0x3C: fprintf(stream, "\tDSLL32\t" I_OP "\n", OP_I(uiMIPSword)); return;	// I
			case 0x3E: fprintf(stream, "\tDSRL32\t" I_OP "\n", OP_I(uiMIPSword)); return;	// I
			case 0x3F: fprintf(stream, "\tDSRA32\t" I_OP "\n", OP_I(uiMIPSword)); return;	// I
		}
		break;

	case 0x01:
		op2=(uiMIPSword>>16)&0x1f;

		switch(op2)
		{
		case 0x00: 	fprintf(stream, "\tBLTZ   \t" BV_OP "\n", OP_BV(uiMIPSword)); return;	// I
		case 0x01: 	fprintf(stream, "\tBGEZ   \t" BV_OP "\n", OP_BV(uiMIPSword)); return;	// I
		case 0x02: 	fprintf(stream, "\tBLTZL  \t" BV_OP "\n", OP_BV(uiMIPSword)); return;	// I
		case 0x03: 	fprintf(stream, "\tBGEZL  \t" BV_OP "\n", OP_BV(uiMIPSword)); return;	// I
		case 0x08: 	fprintf(stream, "\tTGEI\n"); 	return;
		case 0x09: 	fprintf(stream, "\tTGEIU\n"); 	return;
		case 0x0A: 	fprintf(stream, "\tTLTI\n"); 	return;
		case 0x0B: 	fprintf(stream, "\tTLTIU\n"); 	return;
		case 0x0C: 	fprintf(stream, "\tTEQI\n"); 	return;
		case 0x0E: 	fprintf(stream, "\tTNEI\n"); 	return;
		case 0x10: 	fprintf(stream, "\tBLTZAL \t" BV_OP "\n", OP_BV(uiMIPSword)); return;	// I and link
		case 0x11: 	fprintf(stream, "\tBGEZAL \t" BV_OP "\n", OP_BV(uiMIPSword)); return;	// I and link
		case 0x12: 	fprintf(stream, "\tBLTZALL\t" BV_OP "\n", OP_BV(uiMIPSword)); return;	// I and link likely
		case 0x13: 	fprintf(stream, "\tBGEZALL\t" BV_OP "\n", OP_BV(uiMIPSword)); return;	// I and link likely
		}

		break;

	case 0x02: 	fprintf(stream, "\tJ      \t" J_OP "\n", OP_J(x, uiMIPSword)); return;	// J
	case 0x03: 	fprintf(stream, "\tJAL    \t" J_OP "\n", OP_J(x, uiMIPSword)); return;	// J
	case 0x04: 	fprintf(stream, "\tBEQ    \t" B_OP "\n", OP_B(uiMIPSword)); return;	// I
	case 0x05: 	fprintf(stream, "\tBNE    \t" B_OP "\n", OP_B(uiMIPSword)); return;	// I
	case 0x06: 	fprintf(stream, "\tBLEZ   \t" BV_OP "\n", OP_BV(uiMIPSword)); return;	// I
	case 0x07: 	fprintf(stream, "\tBGTZ   \t" BV_OP "\n", OP_BV(uiMIPSword)); return;	// I
	case 0x08: 	fprintf(stream, "\tADDI   \t" I_OP "\n", OP_I(uiMIPSword)); return;	// I
	case 0x09: 	fprintf(stream, "\tADDIU  \t" I_OP "\n", OP_I(uiMIPSword)); return;	// I
	case 0x0A: 	fprintf(stream, "\tSLTI   \t" I_OP "\n", OP_I(uiMIPSword)); return;	// I
	case 0x0B: 	fprintf(stream, "\tSLTIU  \t" I_OP "\n", OP_I(uiMIPSword)); return;	// I
	case 0x0C: 	fprintf(stream, "\tANDI   \t" I_OP "\n", OP_UI(uiMIPSword)); return;	// I
	case 0x0D: 	fprintf(stream, "\tORI    \t" I_OP "\n", OP_UI(uiMIPSword)); return;	// I
	case 0x0E: 	fprintf(stream, "\tXORI   \t" I_OP "\n", OP_UI(uiMIPSword)); return;	// I
	case 0x0F: 	fprintf(stream, "\tLUI    \t s%02u, #%02d\t\t// Load upper half of word with 0x%x\n", ((uiMIPSword & 0x001F0000) >> 16), uiMIPSword&0xFFFF, uiMIPSword&0xFFFF); return;	// I
	case 0x10: 	// cop0
		op2=(uiMIPSword>>21)&0x1f;
		switch(op2)
		{
		case 0x00: fprintf(stream, "\tMFC0 \tr%02d, f%02d \t\t// move f%d to r%d\n", ((uiMIPSword>>16)&0x1f), ((uiMIPSword>>11)&0x1f), ((uiMIPSword>>16)&0x1f), ((uiMIPSword>>11)&0x1f)); return;
		case 0x04: fprintf(stream, "\tMTC0 \tr%02d, f%02d \t\t// move r%d to f%d\n", ((uiMIPSword>>16)&0x1f), ((uiMIPSword>>11)&0x1f), ((uiMIPSword>>16)&0x1f), ((uiMIPSword>>11)&0x1f)); return;
		case 0x10: // tlb
			switch(uiMIPSword&0x3f)
			{
			case 0x01: fprintf(stream, "\tTLBR\n"); return;
			case 0x02: fprintf(stream, "\tTLBWI\n"); return;
			case 0x06: fprintf(stream, "\tTLBWR\n"); return;
			case 0x08: fprintf(stream, "\tTLBP\n"); return;
			case 0x18: fprintf(stream, "\tERET\n"); return;
			}
		}
		break;

	case 0x11: // cop1
		op2=(uiMIPSword>>21)&0x1f;
		switch(op2)
		{
		case 0x00: fprintf(stream, "\tMFC1 \t r%2d, f%2d \t// move f%d to r%d\n", ((uiMIPSword>>16)&0x1f), ((uiMIPSword>>11)&0x1f), ((uiMIPSword>>16)&0x1f), ((uiMIPSword>>11)&0x1f)); return;
		case 0x01: fprintf(stream, "\tDMFC1\t r%2d, f%2d \t// move r%d to f%d\n", ((uiMIPSword>>16)&0x1f), ((uiMIPSword>>11)&0x1f), ((uiMIPSword>>16)&0x1f), ((uiMIPSword>>11)&0x1f)); return;
		case 0x02: fprintf(stream, "\tCFC1\n"); return;
		case 0x04: fprintf(stream, "\tMTC1\n"); return;
		case 0x05: fprintf(stream, "\tDMTC1\n"); return;
		case 0x06: fprintf(stream, "\tCTC1\n"); return;
		case 0x08: fprintf(stream, "\tBC1\n");
			switch((uiMIPSword>>16)&0x3)
			{
			case 0x00: fprintf(stream, "\tBC1F\n"); return;
			case 0x01: fprintf(stream, "\tBC1T\n"); return;
			case 0x02: fprintf(stream, "\tBC1FL\n"); return;
			case 0x03: fprintf(stream, "\tBC1TL\n"); return;
			}break;

		case 0x10: // C1.S");
			switch(uiMIPSword&0x3f)
			{
			case 0x00: fprintf(stream, "\tADD.S\n"); return;
			case 0x01: fprintf(stream, "\tSUB.S\n"); return;
			case 0x02: fprintf(stream, "\tMUL.S\n"); return;
			case 0x03: fprintf(stream, "\tDIV.S\n"); return;
			case 0x04: fprintf(stream, "\tSQRT.S\n"); return;
			case 0x05: fprintf(stream, "\tABS.S\n"); return;
			case 0x06: fprintf(stream, "\tMOV.S\n"); return;
			case 0x07: fprintf(stream, "\tNEG.S\n"); return;
			case 0x08: fprintf(stream, "\tROUND.L.S\n"); return;
			case 0x09: fprintf(stream, "\tTRUNC.L.S\n"); return;
			case 0x0A: fprintf(stream, "\tCEIL.L.S\n"); return;
			case 0x0B: fprintf(stream, "\tFLOOR.L.S\n"); return;
			case 0x0C: fprintf(stream, "\tROUND.W.S\n"); return;
			case 0x0D: fprintf(stream, "\tTRUNC.W.S\n"); return;
			case 0x0E: fprintf(stream, "\tCEIL.W.S\n"); return;
			case 0x0F: fprintf(stream, "\tFLOOR.W.S\n"); return;
			case 0x21: fprintf(stream, "\tCVT.D.S\n"); return;
			case 0x24: fprintf(stream, "\tCVT.W.S\n"); return;
			case 0x25: fprintf(stream, "\tCVT.L.S\n"); return;
			case 0x30: fprintf(stream, "\tC.F.S\n"); return;
			case 0x31: fprintf(stream, "\tC.UN.S\n"); return;
			case 0x32: fprintf(stream, "\tC.EQ.S\n"); return;
			case 0x33: fprintf(stream, "\tC.UEQ.S\n"); return;
			case 0x34: fprintf(stream, "\tC.OLT.S\n"); return;
			case 0x35: fprintf(stream, "\tC.ULT.S\n"); return;
			case 0x36: fprintf(stream, "\tC.OLE.S\n"); return;
			case 0x37: fprintf(stream, "\tC.ULE.S\n"); return;
			case 0x38: fprintf(stream, "\tC.SF.S\n"); return;
			case 0x39: fprintf(stream, "\tC.NGLE.S\n"); return;
			case 0x3A: fprintf(stream, "\tC.SEQ.S\n"); return;
			case 0x3B: fprintf(stream, "\tC.NGL.S\n"); return;
			case 0x3C: fprintf(stream, "\tC.LT.S\n"); return;
			case 0x3D: fprintf(stream, "\tC.NGE.S\n"); return;
			case 0x3E: fprintf(stream, "\tC.LE.S\n"); return;
			case 0x3F: fprintf(stream, "\tC.NGT.S\n"); return;
			}
			break;
		case 0x11: //fprintf(stream, "\tC1.D\n");
			switch(uiMIPSword&0x3f)
			{
			case 0x00: fprintf(stream, "\tADD.D\n"); return;
			case 0x01: fprintf(stream, "\tSUB.D\n"); return;
			case 0x02: fprintf(stream, "\tMUL.D\n"); return;
			case 0x03: fprintf(stream, "\tDIV.D\n"); return;
			case 0x04: fprintf(stream, "\tSQRT.D\n"); return;
			case 0x05: fprintf(stream, "\tABS.D\n"); return;
			case 0x06: fprintf(stream, "\tMOV.D\n"); return;
			case 0x07: fprintf(stream, "\tNEG.D\n"); return;
			case 0x08: fprintf(stream, "\tROUND.L.D\n"); return;
			case 0x09: fprintf(stream, "\tTRUNC.L.D\n"); return;
			case 0x0A: fprintf(stream, "\tCEIL.L.D\n"); return;
			case 0x0B: fprintf(stream, "\tFLOOR.L.D\n"); return;
			case 0x0C: fprintf(stream, "\tROUND.W.D\n"); return;
			case 0x0D: fprintf(stream, "\tTRUNC.W.D\n"); return;
			case 0x0E: fprintf(stream, "\tCEIL.W.D\n"); return;
			case 0x0F: fprintf(stream, "\tFLOOR.W.D\n"); return;
			case 0x20: fprintf(stream, "\tCVT.S.D\n"); return;
			case 0x24: fprintf(stream, "\tCVT.W.D\n"); return;
			case 0x25: fprintf(stream, "\tCVT.L.D\n"); return;
			case 0x30: fprintf(stream, "\tC.F.D\n"); return;
			case 0x31: fprintf(stream, "\tC.UN.D\n"); return;
			case 0x32: fprintf(stream, "\tC.EQ.D\n"); return;
			case 0x33: fprintf(stream, "\tC.UEQ.D\n"); return;
			case 0x34: fprintf(stream, "\tC.OLT.D\n"); return;
			case 0x35: fprintf(stream, "\tC.ULT.D\n"); return;
			case 0x36: fprintf(stream, "\tC.OLE.D\n"); return;
			case 0x37: fprintf(stream, "\tC.ULE.D\n"); return;
			case 0x38: fprintf(stream, "\tC.SF.D\n"); return;
			case 0x39: fprintf(stream, "\tC.NGLE.D\n"); return;
			case 0x3A: fprintf(stream, "\tC.SEQ.D\n"); return;
			case 0x3B: fprintf(stream, "\tC.NGL.D\n"); return;
			case 0x3C: fprintf(stream, "\tC.LT.D\n"); return;
			case 0x3D: fprintf(stream, "\tC.NGE.D\n"); return;
			case 0x3E: fprintf(stream, "\tC.LE.D\n"); return;
			case 0x3F: fprintf(stream, "\tC.NGT.D\n"); return;
			}
			break;
		case 0x14: //fprintf(stream, "\tC1.W\n");
			switch(uiMIPSword&0x3f)
			{
			case 0x20: fprintf(stream, "\tCVT.S.W\n"); return;
			case 0x21: fprintf(stream, "\tCVT.D.W\n"); return;
			}
			break;

		case 0x15: //fprintf(stream, "\tC1.L\n");
			switch(uiMIPSword&0x3f)
			{
			case 0x20: fprintf(stream, "\tCVT.S.L\n"); return;
			case 0x21: fprintf(stream, "\tCVT.D.L\n"); return;
			}
			break;
		}
		break;

	case 0x14: fprintf(stream, "\tBEQL  \n"); return;
	case 0x15: fprintf(stream, "\tBNEL  \n"); return;
	case 0x16: fprintf(stream, "\tBLEZL \n"); return;
	case 0x17: fprintf(stream, "\tBGTZL \n"); return;
	case 0x18: fprintf(stream, "\tDADDI \n"); return;
	case 0x19: fprintf(stream, "\tDADDIU\n"); return;
	case 0x1A: fprintf(stream, "\tLDL   \n"); return;
	case 0x1B: fprintf(stream, "\tLDR   \n"); return;
	case 0x20: fprintf(stream, "\tLB    \t" L_OP "\n", OP_L(uiMIPSword)); return;	// Load Byte
	case 0x21: fprintf(stream, "\tLH    \t" L_OP "\n", OP_L(uiMIPSword)); return;	// Load Halfword
	case 0x22: fprintf(stream, "\tLWL   \n"); return;
	case 0x23: fprintf(stream, "\tLW    \t" L_OP "\n", OP_L(uiMIPSword)); return;	// Load Word
	case 0x24: fprintf(stream, "\tLBU   \t" L_OP "\n", OP_L(uiMIPSword)); return;	// Load Unsigned Byte
	case 0x25: fprintf(stream, "\tLHU   \t" L_OP "\n", OP_L(uiMIPSword)); return;	// Load Halfword unsigned
	case 0x26: fprintf(stream, "\tLWR   \n"); return;
	case 0x27: fprintf(stream, "\tLWU   \t" L_OP "\n", OP_L(uiMIPSword)); return;	// Load Word unsigned
	case 0x28: fprintf(stream, "\tSB    \t" L_OP "\n", OP_L(uiMIPSword)); return;	// I
	case 0x29: fprintf(stream, "\tSH    \t" L_OP "\n", OP_L(uiMIPSword)); return;	// I
	case 0x2A: fprintf(stream, "\tSWL   \n"); return;
	case 0x2B: fprintf(stream, "\tSW    \t" L_OP "\n", OP_L(uiMIPSword)); return;	// I
	case 0x2C: fprintf(stream, "\tSDL   \n"); return;
	case 0x2D: fprintf(stream, "\tSDR   \n"); return;
	case 0x2E: fprintf(stream, "\tSWR   \n"); return;
	case 0x2F: fprintf(stream, "\tCACHE \n"); return;
	case 0x30: fprintf(stream, "\tLL    \t" L_OP "\n", OP_L(uiMIPSword)); return;	// Load Linked Word atomic Read-Modify-Write ops
	case 0x31: fprintf(stream, "\tLWC1  \t" L_OP "\n", OP_L(uiMIPSword)); return;	// Load Word to co processor 1
	case 0x34: fprintf(stream, "\tLLD   \t" L_OP "\n", OP_L(uiMIPSword)); return;	// Load Linked Dbl Word atomic Read-Modify-Write ops
	case 0x35: fprintf(stream, "\tLDC1  \n"); return;
	case 0x37: fprintf(stream, "\tLD    \t" L_OP "\n", OP_L(uiMIPSword)); return; 	// Load Double word
	case 0x38: fprintf(stream, "\tSC    \t" L_OP "\n", OP_L(uiMIPSword)); return;	// Store Linked Word atomic Read-Modify-Write ops
	case 0x39: fprintf(stream, "\tSWC1  \t" L_OP "\n", OP_L(uiMIPSword)); return;	// Store Word from co processor 1 to memory
	case 0x3C: fprintf(stream, "\tSCD   \t" L_OP "\n", OP_L(uiMIPSword)); return;	// Store Conditional Double Word
	case 0x3D: fprintf(stream, "\tSDC1  \n"); return;
	case 0x3F: fprintf(stream, "\tSD    \t" L_OP "\n", OP_L(uiMIPSword)); return; 	// Store Double word
	}

	//fprintf(stream, "\t...\t0x%08X\n",x, uiMIPSword); return;
}

void sprintf_mips(char* stream, const uint32_t x, const uint32_t uiMIPSword)
{
	uint32_t op=uiMIPSword>>26;
	uint32_t op2;

#if defined(SHOW_PRINT_MIPS_VALUE)
	stream += sprintf(stream, "0x%08x 0x%08x", x, uiMIPSword);
#else
	stream += sprintf(stream, "0x%08x ", x);
#endif

	switch(op)
	{
	case 0x00:
		op2=uiMIPSword&0x3f;
		switch(op2)
		{
			case 0x00: sprintf(stream, "\tSLL   \t" I_OP "\n", OP_I(uiMIPSword)); return;	// I
			case 0x02: sprintf(stream, "\tSRL   \t" I_OP "\n", OP_I(uiMIPSword)); return;	// I
			case 0x03: sprintf(stream, "\tSRA   \t" I_OP "\n", OP_I(uiMIPSword)); return;	// I
			case 0x04: sprintf(stream, "\tSLLV  \t" R_OP "\n", OP_R(uiMIPSword)); return;	// I
			case 0x07: sprintf(stream, "\tSRAV  \t" R_OP "\n", OP_R(uiMIPSword)); return;	// I
			case 0x08: sprintf(stream, "\tJR    \tr%d\n", (uiMIPSword>>21)&0x1f); return;	// J
			case 0x09: sprintf(stream, "\tJALR  \tr%d // PC=r%d, r%d=%d \n",
					(uiMIPSword>>21)&0x1f,
					(uiMIPSword>>21)&0x1f,
					(uiMIPSword>>11)&0x1f,
					x+8); return;	// J
			case 0x0C: sprintf(stream, "\tSYSCALL\t" I_OP "\n", OP_I(uiMIPSword)); return;	// I
			case 0x0D: sprintf(stream, "\tBREAK \t" I_OP "\n", OP_I(uiMIPSword)); return;	// I
			case 0x0F: sprintf(stream, "\tSYNC  \t" I_OP "\n", OP_I(uiMIPSword)); return;	// I
			case 0x10: sprintf(stream, "\tMFHI  \t" I_OP "\n", OP_I(uiMIPSword)); return;	// I
			case 0x11: sprintf(stream, "\tMTHI  \t" I_OP "\n", OP_I(uiMIPSword)); return;	// I
			case 0x12: sprintf(stream, "\tMFLO  \t" I_OP "\n", OP_I(uiMIPSword)); return;	// I
			case 0x13: sprintf(stream, "\tMTLO  \t" I_OP "\n", OP_I(uiMIPSword)); return;	// I
			case 0x14: sprintf(stream, "\tDSLLV \t" R_OP "\n", OP_R(uiMIPSword)); return;	// I
			case 0x16: sprintf(stream, "\tDSRLV \t" R_OP "\n", OP_R(uiMIPSword)); return;	// I
			case 0x17: sprintf(stream, "\tDSRAV \t" R_OP "\n", OP_R(uiMIPSword)); return;	// I
			case 0x18: sprintf(stream, "\tMULT  \t" R2_OP "\n", OP_R2(uiMIPSword)); return;	// I
			case 0x19: sprintf(stream, "\tMULTU \t" R2_OP "\n", OP_R2(uiMIPSword)); return;	// I
			case 0x1A: sprintf(stream, "\tDIV   \t" R2_OP "\n", OP_R2(uiMIPSword)); return;	// I
			case 0x1B: sprintf(stream, "\tDIVU  \t" R2_OP "\n", OP_R2(uiMIPSword)); return;	// I
			case 0x1C: sprintf(stream, "\tDMULT \t" R2_OP "\n", OP_R2(uiMIPSword)); return;	// I
			case 0x1D: sprintf(stream, "\tDMULTU\t" R2_OP "\n", OP_R2(uiMIPSword)); return;	// I
			case 0x1E: sprintf(stream, "\tDDIV  \t" R2_OP "\n", OP_R2(uiMIPSword)); return;	// I
			case 0x1F: sprintf(stream, "\tDDIVU \t" R2_OP "\n", OP_R2(uiMIPSword)); return;	// I
			case 0x20: sprintf(stream, "\tADD   \t" R_OP "\n", OP_R(uiMIPSword)); return;	// I
			case 0x21: sprintf(stream, "\tADDU  \t" R_OP "\n", OP_R(uiMIPSword)); return;	// I
			case 0x22: sprintf(stream, "\tSUB   \t" R_OP "\n", OP_R(uiMIPSword)); return;	// I
			case 0x23: sprintf(stream, "\tSUBU  \t" R_OP "\n", OP_R(uiMIPSword)); return;	// I
			case 0x24: sprintf(stream, "\tAND   \t" R_OP "\n", OP_R(uiMIPSword)); return;	// I
			case 0x25: sprintf(stream, "\tOR    \t" R_OP "\n", OP_R(uiMIPSword)); return;	// I
			case 0x26: sprintf(stream, "\tXOR   \t" R_OP "\n", OP_R(uiMIPSword)); return;	// I
			case 0x27: sprintf(stream, "\tNOR   \t" R_OP "\n", OP_R(uiMIPSword)); return;	// I
			case 0x2A: sprintf(stream, "\tSLT   \t" I_OP "\n", OP_I(uiMIPSword)); return;	// I
			case 0x2B: sprintf(stream, "\tSLTU  \t" I_OP "\n", OP_I(uiMIPSword)); return;	// I
			case 0x2C: sprintf(stream, "\tDADD  \t" R_OP "\n", OP_R(uiMIPSword)); return;	// I
			case 0x2D: sprintf(stream, "\tDADDU \t" R_OP "\n", OP_R(uiMIPSword)); return;	// I
			case 0x2E: sprintf(stream, "\tDSUB  \t" R_OP "\n", OP_R(uiMIPSword)); return;	// I
			case 0x2F: sprintf(stream, "\tDSUBU \t" R_OP "\n", OP_R(uiMIPSword)); return;	// I
			case 0x30: sprintf(stream, "\tTGE   \t" R2_OP "\n", OP_R2(uiMIPSword)); return;	// I
			case 0x31: sprintf(stream, "\tTGEU  \t" R2_OP "\n", OP_R2(uiMIPSword)); return;	// I
			case 0x32: sprintf(stream, "\tTLT   \t" R2_OP "\n", OP_R2(uiMIPSword)); return;	// I
			case 0x33: sprintf(stream, "\tTLTU  \t" R2_OP "\n", OP_R2(uiMIPSword)); return;	// I
			case 0x34: sprintf(stream, "\tTEQ   \t" R2_OP "\n", OP_R2(uiMIPSword)); return;	// I
			case 0x36: sprintf(stream, "\tTNE   \t" R2_OP "\n", OP_R2(uiMIPSword)); return;	// I
			case 0x38: sprintf(stream, "\tDSLL  \t" I_OP "\n", OP_I(uiMIPSword)); return;	// I
			case 0x3A: sprintf(stream, "\tDSRL  \t" I_OP "\n", OP_I(uiMIPSword)); return;	// I
			case 0x3B: sprintf(stream, "\tDSRA  \t" I_OP "\n", OP_I(uiMIPSword)); return;	// I
			case 0x3C: sprintf(stream, "\tDSLL32\t" I_OP "\n", OP_I(uiMIPSword)); return;	// I
			case 0x3E: sprintf(stream, "\tDSRL32\t" I_OP "\n", OP_I(uiMIPSword)); return;	// I
			case 0x3F: sprintf(stream, "\tDSRA32\t" I_OP "\n", OP_I(uiMIPSword)); return;	// I
		}
		break;

	case 0x01:
		op2=(uiMIPSword>>16)&0x1f;

		switch(op2)
		{
		case 0x00: 	sprintf(stream, "\tBLTZ   \t" BV_OP "\n", OP_BV(uiMIPSword)); return;	// I
		case 0x01: 	sprintf(stream, "\tBGEZ   \t" BV_OP "\n", OP_BV(uiMIPSword)); return;	// I
		case 0x02: 	sprintf(stream, "\tBLTZL  \t" BV_OP "\n", OP_BV(uiMIPSword)); return;	// I
		case 0x03: 	sprintf(stream, "\tBGEZL  \t" BV_OP "\n", OP_BV(uiMIPSword)); return;	// I
		case 0x08: 	sprintf(stream, "\tTGEI\n"); 	return;
		case 0x09: 	sprintf(stream, "\tTGEIU\n"); 	return;
		case 0x0A: 	sprintf(stream, "\tTLTI\n"); 	return;
		case 0x0B: 	sprintf(stream, "\tTLTIU\n"); 	return;
		case 0x0C: 	sprintf(stream, "\tTEQI\n"); 	return;
		case 0x0E: 	sprintf(stream, "\tTNEI\n"); 	return;
		case 0x10: 	sprintf(stream, "\tBLTZAL \t" BV_OP "\n", OP_BV(uiMIPSword)); return;	// I and link
		case 0x11: 	sprintf(stream, "\tBGEZAL \t" BV_OP "\n", OP_BV(uiMIPSword)); return;	// I and link
		case 0x12: 	sprintf(stream, "\tBLTZALL\t" BV_OP "\n", OP_BV(uiMIPSword)); return;	// I and link likely
		case 0x13: 	sprintf(stream, "\tBGEZALL\t" BV_OP "\n", OP_BV(uiMIPSword)); return;	// I and link likely
		}

		break;

	case 0x02: 	sprintf(stream, "\tJ      \t" J_OP "\n", OP_J(x, uiMIPSword)); return;	// J
	case 0x03: 	sprintf(stream, "\tJAL    \t" J_OP "\n", OP_J(x, uiMIPSword)); return;	// J
	case 0x04: 	sprintf(stream, "\tBEQ    \t" B_OP "\n", OP_B(uiMIPSword)); return;	// I
	case 0x05: 	sprintf(stream, "\tBNE    \t" B_OP "\n", OP_B(uiMIPSword)); return;	// I
	case 0x06: 	sprintf(stream, "\tBLEZ   \t" BV_OP "\n", OP_BV(uiMIPSword)); return;	// I
	case 0x07: 	sprintf(stream, "\tBGTZ   \t" BV_OP "\n", OP_BV(uiMIPSword)); return;	// I
	case 0x08: 	sprintf(stream, "\tADDI   \t" I_OP "\n", OP_I(uiMIPSword)); return;	// I
	case 0x09: 	sprintf(stream, "\tADDIU  \t" I_OP "\n", OP_I(uiMIPSword)); return;	// I
	case 0x0A: 	sprintf(stream, "\tSLTI   \t" I_OP "\n", OP_I(uiMIPSword)); return;	// I
	case 0x0B: 	sprintf(stream, "\tSLTIU  \t" I_OP "\n", OP_I(uiMIPSword)); return;	// I
	case 0x0C: 	sprintf(stream, "\tANDI   \t" I_OP "\n", OP_UI(uiMIPSword)); return;	// I
	case 0x0D: 	sprintf(stream, "\tORI    \t" I_OP "\n", OP_UI(uiMIPSword)); return;	// I
	case 0x0E: 	sprintf(stream, "\tXORI   \t" I_OP "\n", OP_UI(uiMIPSword)); return;	// I
	case 0x0F: 	sprintf(stream, "\tLUI    \t s%02u, #%02d\t\t// Load upper half of word with 0x%x\n", ((uiMIPSword & 0x001F0000) >> 16), uiMIPSword&0xFFFF, uiMIPSword&0xFFFF); return;	// I
	case 0x10: 	// cop0
		op2=(uiMIPSword>>21)&0x1f;
		switch(op2)
		{
		case 0x00: sprintf(stream, "\tMFC0 \tr%02d, f%02d \t\t// move f%d to r%d\n", ((uiMIPSword>>16)&0x1f), ((uiMIPSword>>11)&0x1f), ((uiMIPSword>>16)&0x1f), ((uiMIPSword>>11)&0x1f)); return;
		case 0x04: sprintf(stream, "\tMTC0 \tr%02d, f%02d \t\t// move r%d to f%d\n", ((uiMIPSword>>16)&0x1f), ((uiMIPSword>>11)&0x1f), ((uiMIPSword>>16)&0x1f), ((uiMIPSword>>11)&0x1f)); return;
		case 0x10: // tlb
			switch(uiMIPSword&0x3f)
			{
			case 0x01: sprintf(stream, "\tTLBR\n"); return;
			case 0x02: sprintf(stream, "\tTLBWI\n"); return;
			case 0x06: sprintf(stream, "\tTLBWR\n"); return;
			case 0x08: sprintf(stream, "\tTLBP\n"); return;
			case 0x18: sprintf(stream, "\tERET\n"); return;
			}
		}
		break;

	case 0x11: // cop1
		op2=(uiMIPSword>>21)&0x1f;
		switch(op2)
		{
		case 0x00: sprintf(stream, "\tMFC1 \t r%2d, f%2d \t// move f%d to r%d\n", ((uiMIPSword>>16)&0x1f), ((uiMIPSword>>11)&0x1f), ((uiMIPSword>>16)&0x1f), ((uiMIPSword>>11)&0x1f)); return;
		case 0x01: sprintf(stream, "\tDMFC1\t r%2d, f%2d \t// move r%d to f%d\n", ((uiMIPSword>>16)&0x1f), ((uiMIPSword>>11)&0x1f), ((uiMIPSword>>16)&0x1f), ((uiMIPSword>>11)&0x1f)); return;
		case 0x02: sprintf(stream, "\tCFC1\n"); return;
		case 0x04: sprintf(stream, "\tMTC1\n"); return;
		case 0x05: sprintf(stream, "\tDMTC1\n"); return;
		case 0x06: sprintf(stream, "\tCTC1\n"); return;
		case 0x08: sprintf(stream, "\tBC1\n");
			switch((uiMIPSword>>16)&0x3)
			{
			case 0x00: sprintf(stream, "\tBC1F\n"); return;
			case 0x01: sprintf(stream, "\tBC1T\n"); return;
			case 0x02: sprintf(stream, "\tBC1FL\n"); return;
			case 0x03: sprintf(stream, "\tBC1TL\n"); return;
			}break;

		case 0x10: // C1.S");
			switch(uiMIPSword&0x3f)
			{
			case 0x00: sprintf(stream, "\tADD.S\n"); return;
			case 0x01: sprintf(stream, "\tSUB.S\n"); return;
			case 0x02: sprintf(stream, "\tMUL.S\n"); return;
			case 0x03: sprintf(stream, "\tDIV.S\n"); return;
			case 0x04: sprintf(stream, "\tSQRT.S\n"); return;
			case 0x05: sprintf(stream, "\tABS.S\n"); return;
			case 0x06: sprintf(stream, "\tMOV.S\n"); return;
			case 0x07: sprintf(stream, "\tNEG.S\n"); return;
			case 0x08: sprintf(stream, "\tROUND.L.S\n"); return;
			case 0x09: sprintf(stream, "\tTRUNC.L.S\n"); return;
			case 0x0A: sprintf(stream, "\tCEIL.L.S\n"); return;
			case 0x0B: sprintf(stream, "\tFLOOR.L.S\n"); return;
			case 0x0C: sprintf(stream, "\tROUND.W.S\n"); return;
			case 0x0D: sprintf(stream, "\tTRUNC.W.S\n"); return;
			case 0x0E: sprintf(stream, "\tCEIL.W.S\n"); return;
			case 0x0F: sprintf(stream, "\tFLOOR.W.S\n"); return;
			case 0x21: sprintf(stream, "\tCVT.D.S\n"); return;
			case 0x24: sprintf(stream, "\tCVT.W.S\n"); return;
			case 0x25: sprintf(stream, "\tCVT.L.S\n"); return;
			case 0x30: sprintf(stream, "\tC.F.S\n"); return;
			case 0x31: sprintf(stream, "\tC.UN.S\n"); return;
			case 0x32: sprintf(stream, "\tC.EQ.S\n"); return;
			case 0x33: sprintf(stream, "\tC.UEQ.S\n"); return;
			case 0x34: sprintf(stream, "\tC.OLT.S\n"); return;
			case 0x35: sprintf(stream, "\tC.ULT.S\n"); return;
			case 0x36: sprintf(stream, "\tC.OLE.S\n"); return;
			case 0x37: sprintf(stream, "\tC.ULE.S\n"); return;
			case 0x38: sprintf(stream, "\tC.SF.S\n"); return;
			case 0x39: sprintf(stream, "\tC.NGLE.S\n"); return;
			case 0x3A: sprintf(stream, "\tC.SEQ.S\n"); return;
			case 0x3B: sprintf(stream, "\tC.NGL.S\n"); return;
			case 0x3C: sprintf(stream, "\tC.LT.S\n"); return;
			case 0x3D: sprintf(stream, "\tC.NGE.S\n"); return;
			case 0x3E: sprintf(stream, "\tC.LE.S\n"); return;
			case 0x3F: sprintf(stream, "\tC.NGT.S\n"); return;
			}
			break;
		case 0x11: //sprintf(stream, "\tC1.D\n");
			switch(uiMIPSword&0x3f)
			{
			case 0x00: sprintf(stream, "\tADD.D\n"); return;
			case 0x01: sprintf(stream, "\tSUB.D\n"); return;
			case 0x02: sprintf(stream, "\tMUL.D\n"); return;
			case 0x03: sprintf(stream, "\tDIV.D\n"); return;
			case 0x04: sprintf(stream, "\tSQRT.D\n"); return;
			case 0x05: sprintf(stream, "\tABS.D\n"); return;
			case 0x06: sprintf(stream, "\tMOV.D\n"); return;
			case 0x07: sprintf(stream, "\tNEG.D\n"); return;
			case 0x08: sprintf(stream, "\tROUND.L.D\n"); return;
			case 0x09: sprintf(stream, "\tTRUNC.L.D\n"); return;
			case 0x0A: sprintf(stream, "\tCEIL.L.D\n"); return;
			case 0x0B: sprintf(stream, "\tFLOOR.L.D\n"); return;
			case 0x0C: sprintf(stream, "\tROUND.W.D\n"); return;
			case 0x0D: sprintf(stream, "\tTRUNC.W.D\n"); return;
			case 0x0E: sprintf(stream, "\tCEIL.W.D\n"); return;
			case 0x0F: sprintf(stream, "\tFLOOR.W.D\n"); return;
			case 0x20: sprintf(stream, "\tCVT.S.D\n"); return;
			case 0x24: sprintf(stream, "\tCVT.W.D\n"); return;
			case 0x25: sprintf(stream, "\tCVT.L.D\n"); return;
			case 0x30: sprintf(stream, "\tC.F.D\n"); return;
			case 0x31: sprintf(stream, "\tC.UN.D\n"); return;
			case 0x32: sprintf(stream, "\tC.EQ.D\n"); return;
			case 0x33: sprintf(stream, "\tC.UEQ.D\n"); return;
			case 0x34: sprintf(stream, "\tC.OLT.D\n"); return;
			case 0x35: sprintf(stream, "\tC.ULT.D\n"); return;
			case 0x36: sprintf(stream, "\tC.OLE.D\n"); return;
			case 0x37: sprintf(stream, "\tC.ULE.D\n"); return;
			case 0x38: sprintf(stream, "\tC.SF.D\n"); return;
			case 0x39: sprintf(stream, "\tC.NGLE.D\n"); return;
			case 0x3A: sprintf(stream, "\tC.SEQ.D\n"); return;
			case 0x3B: sprintf(stream, "\tC.NGL.D\n"); return;
			case 0x3C: sprintf(stream, "\tC.LT.D\n"); return;
			case 0x3D: sprintf(stream, "\tC.NGE.D\n"); return;
			case 0x3E: sprintf(stream, "\tC.LE.D\n"); return;
			case 0x3F: sprintf(stream, "\tC.NGT.D\n"); return;
			}
			break;
		case 0x14: //sprintf(stream, "\tC1.W\n");
			switch(uiMIPSword&0x3f)
			{
			case 0x20: sprintf(stream, "\tCVT.S.W\n"); return;
			case 0x21: sprintf(stream, "\tCVT.D.W\n"); return;
			}
			break;

		case 0x15: //sprintf(stream, "\tC1.L\n");
			switch(uiMIPSword&0x3f)
			{
			case 0x20: sprintf(stream, "\tCVT.S.L\n"); return;
			case 0x21: sprintf(stream, "\tCVT.D.L\n"); return;
			}
			break;
		}
		break;

	case 0x14: sprintf(stream, "\tBEQL  \n"); return;
	case 0x15: sprintf(stream, "\tBNEL  \n"); return;
	case 0x16: sprintf(stream, "\tBLEZL \n"); return;
	case 0x17: sprintf(stream, "\tBGTZL \n"); return;
	case 0x18: sprintf(stream, "\tDADDI \n"); return;
	case 0x19: sprintf(stream, "\tDADDIU\n"); return;
	case 0x1A: sprintf(stream, "\tLDL   \n"); return;
	case 0x1B: sprintf(stream, "\tLDR   \n"); return;
	case 0x20: sprintf(stream, "\tLB    \t" L_OP "\n", OP_L(uiMIPSword)); return;	// Load Byte
	case 0x21: sprintf(stream, "\tLH    \t" L_OP "\n", OP_L(uiMIPSword)); return;	// Load Halfword
	case 0x22: sprintf(stream, "\tLWL   \n"); return;
	case 0x23: sprintf(stream, "\tLW    \t" L_OP "\n", OP_L(uiMIPSword)); return;	// Load Word
	case 0x24: sprintf(stream, "\tLBU   \t" L_OP "\n", OP_L(uiMIPSword)); return;	// Load Unsigned Byte
	case 0x25: sprintf(stream, "\tLHU   \t" L_OP "\n", OP_L(uiMIPSword)); return;	// Load Halfword unsigned
	case 0x26: sprintf(stream, "\tLWR   \n"); return;
	case 0x27: sprintf(stream, "\tLWU   \t" L_OP "\n", OP_L(uiMIPSword)); return;	// Load Word unsigned
	case 0x28: sprintf(stream, "\tSB    \t" L_OP "\n", OP_L(uiMIPSword)); return;	// I
	case 0x29: sprintf(stream, "\tSH    \t" L_OP "\n", OP_L(uiMIPSword)); return;	// I
	case 0x2A: sprintf(stream, "\tSWL   \n"); return;
	case 0x2B: sprintf(stream, "\tSW    \t" L_OP "\n", OP_L(uiMIPSword)); return;	// I
	case 0x2C: sprintf(stream, "\tSDL   \n"); return;
	case 0x2D: sprintf(stream, "\tSDR   \n"); return;
	case 0x2E: sprintf(stream, "\tSWR   \n"); return;
	case 0x2F: sprintf(stream, "\tCACHE \n"); return;
	case 0x30: sprintf(stream, "\tLL    \t" L_OP "\n", OP_L(uiMIPSword)); return;	// Load Linked Word atomic Read-Modify-Write ops
	case 0x31: sprintf(stream, "\tLWC1  \t" L_OP "\n", OP_L(uiMIPSword)); return;	// Load Word to co processor 1
	case 0x34: sprintf(stream, "\tLLD   \t" L_OP "\n", OP_L(uiMIPSword)); return;	// Load Linked Dbl Word atomic Read-Modify-Write ops
	case 0x35: sprintf(stream, "\tLDC1  \n"); return;
	case 0x37: sprintf(stream, "\tLD    \t" L_OP "\n", OP_L(uiMIPSword)); return; 	// Load Double word
	case 0x38: sprintf(stream, "\tSC    \t" L_OP "\n", OP_L(uiMIPSword)); return;	// Store Linked Word atomic Read-Modify-Write ops
	case 0x39: sprintf(stream, "\tSWC1  \t" L_OP "\n", OP_L(uiMIPSword)); return;	// Store Word from co processor 1 to memory
	case 0x3C: sprintf(stream, "\tSCD   \t" L_OP "\n", OP_L(uiMIPSword)); return;	// Store Conditional Double Word
	case 0x3D: sprintf(stream, "\tSDC1  \n"); return;
	case 0x3F: sprintf(stream, "\tSD    \t" L_OP "\n", OP_L(uiMIPSword)); return; 	// Store Double word
	}
}
