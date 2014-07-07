
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "CodeSegments.h"
#include "Debugger.h"
#include "InstructionSetMIPS4.h"
#include "Translate.h"

#define LINE_LEN 400
//#define HISTORY_LEN 5


static char userInput[20][20];
//static char cmdHistory[HISTORY_LEN][LINE_LEN];

static code_seg_t* CurrentCodeSeg = NULL;

void getCmd() {

	unsigned char line[LINE_LEN];
	int c,d;
	int argN = 0;


	for(c=0;c<sizeof(userInput[argN])-1;c++) {
		line[c] = fgetc(stdin);

		// TODO strip arrow key input for terminals, eclipse strips this already

		if (line[c] == '\n' || line[c] == '\0')
		{
			line[c+1] = '\0';
			break;
		}
		else if (line[c] == 128 && c > 0)
		{
			c--;
		}
	}
	c=d=0;

	//pushback history
//	memmove(&cmdHistory[0], &cmdHistory[1], (HISTORY_LEN - 1) * LINE_LEN);
//	memcpy(cmdHistory, line, LINE_LEN);

	//clear user input so it doesn't corrupt
	memset(userInput,0,sizeof(userInput));

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


static void Debugger_seg_returnAddr(const code_segment_data_t* segmentData, uint32_t val, code_seg_t* CurrentCodeSeg)
{
	int x;

	if (!CurrentCodeSeg->MIPSReturnRegister)
	{
		if (CurrentCodeSeg->pCodeSegmentTargets[1] != NULL)
		{
			printf("Seg  0x%08x  \t0x%08x\tblock type: %d\n"
					"next segments:\n"
					" (1) 0x%08x  \t0x%08x\n"
					" (2) 0x%08x  \t0x%08x\n"
					, (uint32_t)CurrentCodeSeg, (uint32_t)CurrentCodeSeg->MIPScode, CurrentCodeSeg->blockType
					, (uint32_t)CurrentCodeSeg->pCodeSegmentTargets[0], (uint32_t)CurrentCodeSeg->pCodeSegmentTargets[0]->MIPScode
					, (uint32_t)CurrentCodeSeg->pCodeSegmentTargets[1], (uint32_t)CurrentCodeSeg->pCodeSegmentTargets[1]->MIPScode);
		}
		else
		{
			printf("Seg  0x%08x  \t0x%08x\tblock type: %d\n"
					"next segments:\n"
					" (1) 0x%08x  \t0x%08x\n"

					, (uint32_t)CurrentCodeSeg, (uint32_t)CurrentCodeSeg->MIPScode, CurrentCodeSeg->blockType
					, (uint32_t)CurrentCodeSeg->pCodeSegmentTargets[0], (uint32_t)CurrentCodeSeg->pCodeSegmentTargets[0]->MIPScode);
		}
	}
	else
	{
		printf("Seg  0x%08x  \t0x%08x\tblock type: %d\nnext segments:\n"
				, (uint32_t)CurrentCodeSeg, (uint32_t)CurrentCodeSeg->MIPScode, CurrentCodeSeg->blockType);

		code_seg_t*  tempCodeSeg=segmentData->StaticSegments;
		x=1;
		while (tempCodeSeg !=NULL)
		{
			if (tempCodeSeg->pCodeSegmentTargets[0] == CurrentCodeSeg)
			{
				printf(" (%d) 0x%08x  \t0x%08x\n", x, (uint32_t)tempCodeSeg->pCodeSegmentTargets[0], (uint32_t)tempCodeSeg->pCodeSegmentTargets[0]->MIPScode);
				x++;
			}
			else if (tempCodeSeg->pCodeSegmentTargets[1] == CurrentCodeSeg)
			{
				printf(" (%d) 0x%08x  \t0x%08x\n", x, (uint32_t)tempCodeSeg->pCodeSegmentTargets[1], (uint32_t)tempCodeSeg->pCodeSegmentTargets[1]->MIPScode);
				x++;
			}

			tempCodeSeg = tempCodeSeg->nextCodeSegmentLinkedList;
		}
	}
}

static int Debugger_print(const code_segment_data_t* segmentData)
{
	//int val;
	int x=0;
	//char *tailPointer;
	//code_seg_t* tempCodeSeg;

	//val = strtoul(userInput[1], &tailPointer, 0);

	if (!strncmp(userInput[1], "arm", 1))
	{
		printf("\naddr 0x%x, len %d\n"
				, (uint32_t)CurrentCodeSeg->ARMcode, CurrentCodeSeg->ARMcodeLen );
		printLine();
	}
	else if (!strncmp(userInput[1], "mips", 1))
	{

		for (x=0; x< CurrentCodeSeg->MIPScodeLen; x++)
		{
			mips_print((x)*4, *(CurrentCodeSeg->MIPScode + x));
		}
		printLine();

		for (x=CurrentCodeSeg->MIPScodeLen; x< CurrentCodeSeg->MIPScodeLen+3; x++)
		{
			mips_print((x)*4, *(CurrentCodeSeg->MIPScode + x));
		}

		printf("\naddr 0x%x, len %d, return reg %d\n"
				"0x%08X %08X %02X (%d)\n"
				, (uint32_t)CurrentCodeSeg->MIPScode, CurrentCodeSeg->MIPScodeLen, CurrentCodeSeg->MIPSReturnRegister
				, CurrentCodeSeg->MIPSRegistersUsed[0], CurrentCodeSeg->MIPSRegistersUsed[1], CurrentCodeSeg->MIPSRegistersUsed[2]
				                                                                                                                , CurrentCodeSeg->MIPSRegistersUsedCount);
	}
	else if (!strncmp(userInput[1], "intermediate", 1))
	{
		Intermediate_print(CurrentCodeSeg);
	}
	else if (!strncmp(userInput[1], "reg", 1))
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
	else
	{
		printf(HELP_PRINT);
	}
	return 1;
}

static int Debugger_seg(const code_segment_data_t* segmentData)
{
	int val;
	int x=0;
	char *tailPointer;
	code_seg_t* tempCodeSeg;

	val = strtoul(userInput[1], &tailPointer, 0);

	if (!strlen(userInput[1]))
	{
		printf("First Segment at 0x%x, number of segments %d\n", (uint32_t)segmentData->StaticSegments, segmentData->count);

		printf("Current Segment 0x%x\n"
				"\tMIPS            ARM\n"
				"\t0x%08X %u\t0x%08X %u\n\n",
				(uint32_t)CurrentCodeSeg,
				(uint32_t)CurrentCodeSeg->MIPScode, CurrentCodeSeg->MIPScodeLen,
				(uint32_t)CurrentCodeSeg->ARMcode, CurrentCodeSeg->ARMcodeLen);

		Debugger_seg_returnAddr(segmentData, val, CurrentCodeSeg);

		return 0;
	}

	if (!CurrentCodeSeg->MIPSReturnRegister)
	{
		int ok = 0;
		if (0 == val)
		{
			CurrentCodeSeg = segmentData->StaticSegments;
		}
		else if (1 == val)
		{
			CurrentCodeSeg = CurrentCodeSeg->pCodeSegmentTargets[0];
			ok = 1;
		}
		else if (2 == val && CurrentCodeSeg->pCodeSegmentTargets[1] != 0)
		{
			CurrentCodeSeg = CurrentCodeSeg->pCodeSegmentTargets[1];
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

				tempCodeSeg = tempCodeSeg->nextCodeSegmentLinkedList;
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
			else if (tempCodeSeg->pCodeSegmentTargets[0] == CurrentCodeSeg)
			{
				if (x == val)
				{
					CurrentCodeSeg = tempCodeSeg->pCodeSegmentTargets[0];
					break;
				}
				x++;
			}
			else if (tempCodeSeg->pCodeSegmentTargets[1] == CurrentCodeSeg)
			{
				if (x == val)
				{
					CurrentCodeSeg = tempCodeSeg->pCodeSegmentTargets[1];
					break;
				}
				x++;
			}

			tempCodeSeg = tempCodeSeg->nextCodeSegmentLinkedList;
		}
	}

	Debugger_seg_returnAddr(segmentData, val, CurrentCodeSeg);

	return 0;

}

static int Debugger_opt(const code_segment_data_t* segmentData)
{
	if (!strlen(userInput[1]))
	{
		Translate(CurrentCodeSeg);

	}
	else if (!strncasecmp(userInput[1], "DelaySlot", 1))
	{
		Translate_DelaySlot(CurrentCodeSeg);
	}
	else if (!strncasecmp(userInput[1], "CountRegister", 1))
	{
		Translate_CountRegister(CurrentCodeSeg);
	}
	else if (!strncasecmp(userInput[1], "32BitRegisters", 1))
	{
		Translate_32BitRegisters(CurrentCodeSeg);
	}
	else if (!strncasecmp(userInput[1], "ReduceRegistersUsed", 1))
	{
		Translate_ReduceRegistersUsed(CurrentCodeSeg);
	}
	else if (!strncasecmp(userInput[1], "loadStoreWriteBack", 1))
	{
		Translate_LoadStoreWriteBack(CurrentCodeSeg);
	}
	else if (!strncasecmp(userInput[1], "init", 1))
	{
		Translate_init(CurrentCodeSeg);
	}
	else if (!strncasecmp(userInput[1], "full", 1))
	{
		Translate(CurrentCodeSeg);
	}
	else
	{
		printf(HELP_OPT);
	}

	return 0;
}

void Debugger_start(code_segment_data_t* segmentData)
{

	//find segment
	if (!CurrentCodeSeg) CurrentCodeSeg = segmentData->StaticSegments;

	printf("> "); fflush(stdin);
	getCmd();

	if (!strncmp(userInput[0], "quit", 1))
	{
		exit(0);
	}
	else if (!strncmp(userInput[0], "print", 1))
	{
		Debugger_print(segmentData);
	}
	else if (!strncmp(userInput[0], "segment", 1))
	{
		Debugger_seg(segmentData);
	}
	else if (!strncmp(userInput[0], "opt", 1))
	{
		Debugger_opt(segmentData);
	}
	else if (!strncmp(userInput[0], "help", 1))
	{
		printf(HELP_GEN);
	}
	else
	{
		printf("unknown command: %s", userInput[0]);
	}
}
