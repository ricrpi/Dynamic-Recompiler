
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "CodeSegments.h"
#include "Debugger.h"
#include "InstructionSetMIPS4.h"
#include "InstructionSetARM6hf.h"
#include "Translate.h"

#define LINE_LEN 400
//#define HISTORY_LEN 5

#define MIN(x, y)	(x < y? x : y)




#define CMD_CMP(X, TXT) (strncasecmp(userInput[X], TXT, strlen(userInput[X])))


static char userInput[20][20];
//static char cmdHistory[HISTORY_LEN][LINE_LEN];

static code_seg_t* CurrentCodeSeg = NULL;

void getCmd() {

	unsigned char line[LINE_LEN];
	int c=0;
	int d;
	int argN = 0;

	memset(line, '\0', sizeof(line));

	while (c < sizeof(userInput[argN])-1) {
		line[c] = fgetc(stdin);

		// TODO strip arrow key input for terminals, eclipse strips this already
		if (line[c] == '\n' || line[c] == '\r')
		{
			line[c] = '\0';
			break;
		}
		else if (line[c] == 128)
		{
			if (c > 0)c--;
		}
		else
		{
			c++;
		}
	}

	c=d=0;

	//pushback history
//	memmove(&cmdHistory[0], &cmdHistory[1], (HISTORY_LEN - 1) * LINE_LEN);
//	memcpy(cmdHistory, line, LINE_LEN);

	//clear user input so it doesn't corrupt
	memset(userInput, '\0', sizeof(userInput));

	while (line[c] != '\0')
	{
		if (line[c] == ' ')
		{
			userInput[argN][d] = '\0';
			argN++;
			d = 0;
		}
		else
		{
			userInput[argN][d++] = line[c];
		}

		c++;
	}

	return;
}

static void printLine()
{
	printf("_____________________________________\n\n");
}

static void Debugger_seg_returnAddr(const code_segment_data_t* const segmentData, const uint32_t val, const code_seg_t* const CurrentCodeSeg)
{
	int x;

	if (!CurrentCodeSeg->MIPSReturnRegister)
	{
		printf("Seg  0x%08x  \t0x%08x\ttype: %s\n"
				"next segments:\n"
				, (uint32_t)CurrentCodeSeg, (uint32_t)CurrentCodeSeg->MIPScode
				, seg_type_s[CurrentCodeSeg->Type]);

		if (CurrentCodeSeg->pContinueNext != NULL)
		{
			printf(" (1) 0x%08x  \t0x%08x\t (Continue)\n"
					, (uint32_t)CurrentCodeSeg->pContinueNext, (uint32_t)CurrentCodeSeg->pContinueNext->MIPScode);
		}

		if (CurrentCodeSeg->pBranchNext != NULL)
		{
			if (CurrentCodeSeg->MIPScode == CurrentCodeSeg->pBranchNext->MIPScode)	//Loops on itself
			{
				printf(" (2) 0x%08x  \t0x%08x\t (Loop)\n"
									, (uint32_t)CurrentCodeSeg->pBranchNext, (uint32_t)CurrentCodeSeg->pBranchNext->MIPScode);
			}else
			{
				printf(" (2) 0x%08x  \t0x%08x\t (Branch)\n"
									, (uint32_t)CurrentCodeSeg->pBranchNext, (uint32_t)CurrentCodeSeg->pBranchNext->MIPScode);
			}
		}

		if (!CurrentCodeSeg->pBranchNext && !CurrentCodeSeg->pContinueNext)
		{
			printf("No linkage for code in its current location!\n");
		}
	}
	else
	{
		printf("Seg  0x%08x  \t0x%08x\ttype: %s\nnext segments:\n"
				, (uint32_t)CurrentCodeSeg, (uint32_t)CurrentCodeSeg->MIPScode, seg_type_s[CurrentCodeSeg->Type]);

		code_seg_t*  tempCodeSeg=segmentData->StaticSegments;
		x=1;
		while (tempCodeSeg !=NULL)
		{
			if (tempCodeSeg->pBranchNext == CurrentCodeSeg)
			{
				printf(" (%d) 0x%08x  \t0x%08x\n", x, (uint32_t)tempCodeSeg->pBranchNext, (uint32_t)tempCodeSeg->pBranchNext->MIPScode);
				x++;
			}

			tempCodeSeg = tempCodeSeg->next;
		}
	}
}

static int Debugger_print(const code_segment_data_t* const segmentData)
{
	//int val;
	int x=0;
	char *tailPointer;

	if (!CMD_CMP(1, "arm"))
	{
		uint32_t count = CurrentCodeSeg->ARMcodeLen;
		uint32_t* addr = CurrentCodeSeg->ARMcode;

		if (strlen(userInput[3]))
		{
			count = strtoul(userInput[3], &tailPointer, 0);
			addr = (uint32_t*)((strtoul(userInput[2], &tailPointer, 0))&~0x3);
		}
		else if (strlen(userInput[2]))
		{
			count = strtoul(userInput[2], &tailPointer, 0);
		}

		if (NULL == addr)
		{
			printf("Need to Generate ARM code\n");
			return 0;
		}

		for (x=0; x< count; x++)
		{
			if (addr + x == (uint32_t*)CurrentCodeSeg->ARMEntryPoint)
			{
				printf(".EntryPoint:\n");
				arm_print((uint32_t)((uint32_t*)addr + x), *((uint32_t*)addr + x));
			}
			else if (addr + x < (uint32_t*)CurrentCodeSeg->ARMEntryPoint)
				printf("\t.word\t%12d (0x%08x)\n", *((uint32_t*)addr + x), *((uint32_t*)addr + x));
			else
				arm_print((uint32_t)((uint32_t*)addr + x), *((uint32_t*)addr + x));
		}

		printLine();
	}
	else if (!CMD_CMP(1, "mips"))
	{
		uint32_t count = CurrentCodeSeg->MIPScodeLen;
		uint32_t* addr = CurrentCodeSeg->MIPScode;

		if (strlen(userInput[3]))
		{
			count = strtoul(userInput[3], &tailPointer, 0);
			addr = (uint32_t*)((strtoul(userInput[2], &tailPointer, 0))&~0x3);
		}
		else if (strlen(userInput[2]))
		{
				if (!strncasecmp(userInput[2], "0x", 2))
				{
					addr = (uint32_t*)((strtoul(userInput[2], &tailPointer, 0))&~0x3);
				}
				else
				{
					count = strtoul(userInput[2], &tailPointer, 0);
				}

		}

		if (NULL == addr)
		{
			printf("Invalid Address\n");
			return 0;
		}

		for (x=0; x< count; x++)
		{
			mips_print((uint32_t)((uint32_t*)addr + x), *((uint32_t*)addr + x));
		}

		// if printing segment then print next few instructions for Delay Slot analysis
		if (!strlen(userInput[2]))
		{
			printLine();

			for (x=CurrentCodeSeg->MIPScodeLen; x< CurrentCodeSeg->MIPScodeLen+3; x++)
			{
				mips_print((uint32_t)(CurrentCodeSeg->MIPScode + x), *(CurrentCodeSeg->MIPScode + x));
			}
			printf("\naddr 0x%x, len %d, return reg %d\n"
				"0x%08X %08X %02X (%d)\n"
				, (uint32_t)CurrentCodeSeg->MIPScode, CurrentCodeSeg->MIPScodeLen, CurrentCodeSeg->MIPSReturnRegister
				, CurrentCodeSeg->MIPSRegistersUsed[0], CurrentCodeSeg->MIPSRegistersUsed[1], CurrentCodeSeg->MIPSRegistersUsed[2]
				, CurrentCodeSeg->MIPSRegistersUsedCount);
		}
	}
	else if (!CMD_CMP(1, "intermediate"))
	{
		CodeSeg_print(CurrentCodeSeg);
	}
	else if (!CMD_CMP(1, "literals"))
	{
		Intermediate_Literals_print(CurrentCodeSeg);
	}
	//TODO I do not like this!
	else if (!CMD_CMP(1, "reg"))
	{
		uint32_t cpureg_i, fpureg_i,specialreg_i, cpureg_o, fpureg_o,specialreg_o;
		int y;

		printf("00 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31\n");

		for (x=0; x< CurrentCodeSeg->MIPScodeLen; x++)
		{
			cpureg_i = fpureg_i = specialreg_i = cpureg_o = fpureg_o = specialreg_o = 0;

			switch (ops_type(*(CurrentCodeSeg->MIPScode + x)))
			{
			case DSRLV:
			case DSRAV:
				//	    MULT:
				//	    MULTU:
				//	    DIV:
				//	    DIVU:
			case	    DMULT:
			case	    DMULTU:
			case	    DDIV:
			case	    DDIVU:
				//	    ADD:
				//	    ADDU:
				//	    SUB:
				//	    SUBU:
				//	    AND:
				//	    OR:
				//	    XOR:
				//	    NOR:
				//	    SLT:
				//	    SLTU:
			case	    DADD:
			case	    DADDU:
			case	    DSUB:
			case	    DSUBU:
				//	    TGE:
				//	    TGEU:
				//	    TLT:
				//	    TLTU:
				//	    TEQ:
				//	    TNE:
			case	    DSLL:
			case	    DSRL:
			case	    DSRA:
			case	    DSLL32:
			case	    DSRL32:
			case	    DSRA32:
				//		INVALID:
				//		TGEI:
				//		TGEIU:
				//		TLTI:
				//		TLTIU:
				//		TEQI:
				//		TNEI:
				//		ADDI:
				//		ADDIU:
				//		SLTI:
				//		SLTIU:
				//		ANDI:
				//		ORI:
				//		XORI:
				//		LUI:
				//		cop0:
				//		MFC0:
				//		MTC0:
				//		tlb:
				//		TLBR:
				//		TLBWI:
				//		TLBWR:
				//		TLBP:
				//		ERET:
				//		MFC1:
			case		DMFC1:
				//		CFC1:
				//		MTC1:
			case		DMTC1:
				//		CTC1:
				//		BC1:
				//		BC1F:
				//		BC1T:
				//		BC1FL:
				//		BC1TL:
				//		ADD_S:
				//		SUB_S:
				//		MUL_S:
				//		DIV_S:
				//		SQRT_S:
				//		ABS_S:
				//		MOV_S:
				//		NEG_S:
				//		ROUND_L_S:
				//		TRUNC_L_S:
				//		CEIL_L_S:
				//		FLOOR_L_S:
				//		ROUND_W_S:
				//		TRUNC_W_S:
				//		CEIL_W_S:
				//		FLOOR_W_S:

				//		CVT_W_S:
				//		CVT_L_S:
				//		C_F_S:
				//		C_UN_S:
				//		C_EQ_S:
				//		C_UEQ_S:
				//		C_OLT_S:
				//		C_ULT_S:
				//		C_OLE_S:
				//		C_ULE_S:
				//		C_SF_S:
				//		C_NGLE_S:
				//		C_SEQ_S:
				//		C_NGL_S:
				//		C_LT_S:
				//		C_NGE_S:
				//		C_LE_S:
				//		C_NGT_S:
			case		ADD_D:
			case		SUB_D:
			case		MUL_D:
			case		DIV_D:
			case		SQRT_D:
			case		ABS_D:
			case		MOV_D:
			case		NEG_D:
			case		ROUND_L_D:
			case		TRUNC_L_D:
			case		CEIL_L_D:
			case		FLOOR_L_D:

			case		CVT_L_D:
			case		C_F_D:	//TODO not sure
			case		C_UN_D:
			case		C_EQ_D:
			case		C_UEQ_D:
			case		C_OLT_D:
			case		C_ULT_D:
			case		C_OLE_D:
			case		C_ULE_D:
			case		C_SF_D:	//TODO not sure
			case		C_NGLE_D:
			case		C_SEQ_D:
			case		C_NGL_D:
			case		C_LT_D:
			case		C_NGE_D:
			case		C_LE_D:
			case		C_NGT_D:
				//		CVT_S_W:
			case		CVT_D_L:
			case		DADDI:
			case		DADDIU:
				//		CACHE:
				//		LL:
				//		LWC1:
			case		LLD:
			case		LDC1:
			case		LD:
				//		SC:
				//		SWC1:
			case		SCD:
			case		SDC1:
			case		SD:

				//		SWL:
				//		SW:
			case		SDL:
			case		SDR:
				//		SWR:

			case		LDL:	//only 32 bits are loaded but implies register is 64 bit
			case		LDR:

				ops_regs_input(*(CurrentCodeSeg->MIPScode + x), &cpureg_i, &fpureg_i, &specialreg_i);
				ops_regs_output(*(CurrentCodeSeg->MIPScode + x), &cpureg_o, &fpureg_o, &specialreg_o);

				for (y=0; y < 32; y++)
				{

					if (((cpureg_i >> y)&1) && ((cpureg_o >> y)&1)) printf("I0 ");
					else if ((cpureg_i >> y)&1) printf("I  ");
					else if ((cpureg_o >> y)&1) printf(" 0 ");
					else printf("   ");
				}
				printf("\n");
				break;

			case	ROUND_W_D: //double in, single out
			case	CVT_S_L:
			case	TRUNC_W_D:
			case	CEIL_W_D:
			case	FLOOR_W_D:
			case	CVT_S_D:
			case	CVT_W_D:

				ops_regs_input(*(CurrentCodeSeg->MIPScode + x), &cpureg_i, &fpureg_i, &specialreg_i);
				ops_regs_output(*(CurrentCodeSeg->MIPScode + x), &cpureg_o, &fpureg_o, &specialreg_o);

				for (y=0; y < 32; y++)
				{
					if (((cpureg_i >> y)&1) && ((cpureg_o >> y)&1)) printf ("Io ");
					else if ((cpureg_i >> y)&1) printf("I  ");
					else if ((cpureg_o >> y)&1) printf(" o ");
					else printf("   ");
				}
				printf("\n");
				break;

			case	CVT_D_S: //single in, double out
			case	CVT_D_W:

				ops_regs_input(*(CurrentCodeSeg->MIPScode + x), &cpureg_i, &fpureg_i, &specialreg_i);
				ops_regs_output(*(CurrentCodeSeg->MIPScode + x), &cpureg_o, &fpureg_o, &specialreg_o);

				for (y=0; y < 32; y++)
				{
					if (((cpureg_i >> y)&1) && ((cpureg_o >> y)&1)) printf ("i0 ");
					else if ((cpureg_i >> y)&1) printf(" i ");
					else if ((cpureg_o >> y)&1) printf(" 0 ");
					else printf("   ");
				}
				printf("\n");
				break;

			default:
				ops_regs_input(*(CurrentCodeSeg->MIPScode + x), &cpureg_i, &fpureg_i, &specialreg_i);
				ops_regs_output(*(CurrentCodeSeg->MIPScode + x), &cpureg_o, &fpureg_o, &specialreg_o);
				for (y=0; y < 32; y++)
				{
					if (((cpureg_i >> y)&1) && ((cpureg_o >> y)&1)) printf ("io ");
					else if ((cpureg_i >> y)&1) printf(" i ");
					else if ((cpureg_o >> y)&1) printf(" o ");
					else printf("   ");
				}
				printf("\n");
			}
		}
		printLine();
	}
	else if (!CMD_CMP(1, "lookup"))
	{
		uint32_t val = strtoul(userInput[2], &tailPointer, 0);
		uint32_t len = strtoul(userInput[3], &tailPointer, 0);
		int x;

		if (val < 0x88000000)
		{
			for (x=0; x < len; x++)
			{
				printf("0x%08x => 0x%08x\n", (val + x*4),(uint32_t)segmentData->DynamicBounds[(val)/4 + x]);
			}
		}
		else
		{
			for (x=0; x < len; x++)
			{
				printf("0x%08x => 0x%08x\n", (val + x*4),(uint32_t)segmentData->StaticBounds[(val-0x88000000)/4 + x]);
			}
		}
	}
	else
	{
		printf(HELP_PRINT);
	}
	return 1;
}

static int Debugger_seg(const code_segment_data_t* const segmentData)
{
	int val;
	int x=0;
	char *tailPointer;
	code_seg_t* tempCodeSeg;


	if (!strlen(userInput[1]))
	{
		printf("First Segment   0x%x, number of segments %d\n", (uint32_t)segmentData->StaticSegments, segmentData->count);

		printf("Current Segment 0x%x\n"
				"\tMIPS            ARM\n"
				"\t0x%08X %u\t0x%08X %u\n\n",
				(uint32_t)CurrentCodeSeg,
				(uint32_t)CurrentCodeSeg->MIPScode, CurrentCodeSeg->MIPScodeLen,
				(uint32_t)CurrentCodeSeg->ARMcode, CurrentCodeSeg->ARMcodeLen);
	}
	else if (!CMD_CMP(1, "start"))
	{
		CurrentCodeSeg = segmentData->segStart;

	}
	else if (!CMD_CMP(1, "stop"))
	{
		CurrentCodeSeg = segmentData->segStop;
	}
	else if (!CMD_CMP(1, "memory"))
	{
		CurrentCodeSeg = segmentData->segMem;
	}
	else if (!CMD_CMP(1, "interrupt"))
	{
		CurrentCodeSeg = segmentData->segInterrupt;
	}
	else
	{
		val = strtoul(userInput[1], &tailPointer, 0);

		if (!CurrentCodeSeg->MIPSReturnRegister)
		{
			int ok = 0;
			if (0 == val)
			{
				CurrentCodeSeg = segmentData->StaticSegments;
				ok = 1;
			}
			else if (1 == val && CurrentCodeSeg->pContinueNext != NULL)
			{
				CurrentCodeSeg = CurrentCodeSeg->pContinueNext;
				ok = 1;
			}
			else if (2 == val && CurrentCodeSeg->pBranchNext != NULL)
			{
				CurrentCodeSeg = CurrentCodeSeg->pBranchNext;
				ok = 1;
			}
			else
			{
				tempCodeSeg=segmentData->StaticSegments;

				while (tempCodeSeg != NULL)
				{
					if ((uint32_t)tempCodeSeg == val || (uint32_t)tempCodeSeg->MIPScode == val)
					{
						CurrentCodeSeg = tempCodeSeg;
						ok = 1;
						break;
					}

					tempCodeSeg = tempCodeSeg->next;
				}
			}

			if (!ok) printf("Invalid entry\n");
		}
		else
		{
			tempCodeSeg = segmentData->StaticSegments;
			x=1;
			while (tempCodeSeg != NULL)
			{
				if (tempCodeSeg == (code_seg_t*)val)
				{
					CurrentCodeSeg = (code_seg_t*)val;
					break;
				}
				else if (tempCodeSeg->pBranchNext == CurrentCodeSeg)
				{
					if (x == val)
					{
						CurrentCodeSeg = tempCodeSeg->pBranchNext;
						break;
					}
					x++;
				}

				tempCodeSeg = tempCodeSeg->next;
			}
		}
	}
	Debugger_seg_returnAddr(segmentData, val, CurrentCodeSeg);

	return 0;

}

static int Debugger_translate(const code_segment_data_t* const segmentData)
{

	uint32_t bounds[2];
	uint32_t x,y;
	uint32_t val;

	bounds[0] = 0;
	bounds[1] = COUNTOF(Translations)-1;
	char *tailPointer;

	for (x=0; x < COUNTOF(bounds); x++)
	{
			if (!strlen(userInput[1+x])) break;

			val = strtoul(userInput[1+x], &tailPointer, 0);

			//test for numbers
			if (tailPointer != userInput[1+x])
			{
				bounds[x] = val;
			}
			else
			{
				// scan for names
				for (y = 0; y < COUNTOF(Translations); y++)
				{
					if (!CMD_CMP(1+x, Translations[y].name)){
						bounds[x] = y;
						break;
					}

				}
			}
	}

	if (!strlen(userInput[1]))
	{
		Translate(CurrentCodeSeg);
	}
	else
	{
		if (!strlen(userInput[2]))
		{
			bounds[1] = bounds[0];
		}

		for (x = bounds[0]; x <= bounds[1]; x++)
		{
			printf("Translate_%s:\n", Translations[x].name);
			Translations[x].function(CurrentCodeSeg);
		}

		if (x < COUNTOF(Translations)-1)
		{
			CodeSeg_print(CurrentCodeSeg);
		}
		else
		{
			uint32_t count = CurrentCodeSeg->ARMcodeLen;
			uint32_t* addr = CurrentCodeSeg->ARMcode;
			int x;

			for (x=0; x< count; x++)
			{
				if (addr + x == (uint32_t*)CurrentCodeSeg->ARMEntryPoint)
				{
					printf(".EntryPoint:\n");
					arm_print((uint32_t)((uint32_t*)addr + x), *((uint32_t*)addr + x));
				}
				else if (addr + x < (uint32_t*)CurrentCodeSeg->ARMEntryPoint)
					printf("\t.word\t%12d (0x%08x)\n", *((uint32_t*)addr + x), *((uint32_t*)addr + x));
				else
					arm_print((uint32_t)((uint32_t*)addr + x), *((uint32_t*)addr + x));
			}
		}
	}

	return 0;
}

int Debugger_start(const code_segment_data_t* const segmentData)
{
	//find segment
	if (!CurrentCodeSeg) CurrentCodeSeg = segmentData->dbgCurrentSegment;

	printf("> "); fflush(stdin);
	getCmd();

	if (userInput[0][0] == '\0') return 1;

	if (!CMD_CMP(0, "quit"))
	{
		return 0;
	}
	else if (!CMD_CMP(0, "print"))
	{
		Debugger_print(segmentData);
	}
	else if (!CMD_CMP(0, "segment"))
	{
		Debugger_seg(segmentData);
	}
	else if (!CMD_CMP(0, "translate"))
	{
		Debugger_translate(segmentData);
	}
	else if (!CMD_CMP(0, "start"))
	{
		pfvru1 run = (pfvru1)segmentData->segStart->ARMEntryPoint;

		printf("Starting ...\n");

		uint32_t ret = run();

		printf("End run: %u (0x%x)\n", ret, ret);
	}
	else if (!CMD_CMP(0, "help"))
	{
		if 		(!CMD_CMP(1, "print"))		printf(HELP_PRINT);
		else if (!CMD_CMP(1, "segment"))	printf(HELP_SEG);
		else if (!CMD_CMP(1, "translate")) 	printf(HELP_TRANS);
		else printf(HELP_GEN);
	}
	else
	{
		printf("unknown command: %s\n", userInput[0]);
	}
	return 1;
}
