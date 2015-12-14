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

#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "CodeSegments.h"
#include "Debugger.h"
#include "InstructionSetMIPS4.h"
#include "InstructionSetARM6hf.h"
#include "Translate.h"
#include "memory.h"
#include "mem_state.h"

#define LINE_LEN 500
//#define HISTORY_LEN 5

#define MIN(x, y)	(x < y? x : y)

#define CMD_CMP(X, TXT) (strncasecmp(userInput[X], TXT, strlen(userInput[X])))


static const char* seg_type_s[sizeof_SEG_TYPE_E] = {
		"SEG_SANDWICH",		// segment has code before and after - literals need to be global
		"SEG_START",			// segment has no code before - literals go before
		"SEG_END",			// segment has no code after - literals go after
		"SEG_ALONE"
};

static char userInput[20][40];
//static char cmdHistory[HISTORY_LEN][LINE_LEN];

static code_seg_t* CurrentCodeSeg = NULL;

static unsigned long int Mstrtoul(char* addr, char** tailPointer, int base)
{
	//TODO  do better error detection here
	int x;

	if (!strncmp("fp",addr,2))
	{
		for (x = 0; x < strnlen(addr, LINE_LEN); x++)
			{
				switch (addr[x])
				{
				case '+':
					addr[x] = '\0';
					return MMAP_FP_BASE + strtoul(&addr[x+1], tailPointer, base);
				case '-':
					addr[x] = '\0';
					return MMAP_FP_BASE - strtoul(&addr[x+1], tailPointer, base);
				case '*':
					addr[x] = '\0';
					return MMAP_FP_BASE * strtoul(&addr[x+1], tailPointer, base);
				case '/':
					addr[x] = '\0';
					return MMAP_FP_BASE / strtoul(&addr[x+1], tailPointer, base);
				default:
					break;
				}
			}
			return MMAP_FP_BASE;
	}
	else
	{
		for (x = 0; x < strnlen(addr, LINE_LEN); x++)
		{
			switch (addr[x])
			{
			case '+':
				addr[x] = '\0';
				return strtoul(addr, tailPointer, base) + strtoul(&addr[x+1], tailPointer, base);
			case '-':
				addr[x] = '\0';
				return strtoul(addr, tailPointer, base) - strtoul(&addr[x+1], tailPointer, base);
			case '*':
				addr[x] = '\0';
				return strtoul(addr, tailPointer, base) * strtoul(&addr[x+1], tailPointer, base);
			case '/':
				addr[x] = '\0';
				return strtoul(addr, tailPointer, base) / strtoul(&addr[x+1], tailPointer, base);
			default:
				break;
			}
		}
		return strtoul(addr, tailPointer, base);
	}
}

void getCmd() {

	unsigned char line[LINE_LEN];
	int c=0;
	int d;
	int argN = 0;

	memset(line, '\0', sizeof(line));

	while (c < LINE_LEN-1) {
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

static void printregs(mcontext_t* context, size_t* regs)
{
	if (context)
	{
	#ifndef __i386
			printf("\n\tr0 0x%08lx\t r8  0x%08lx\n", context->arm_r0, context->arm_r8);
			printf("\tr1 0x%08lx\t r9  0x%08lx\n", context->arm_r1, context->arm_r9);
			printf("\tr2 0x%08lx\t r10 0x%08lx\n", context->arm_r2, context->arm_r10);
			printf("\tr3 0x%08lx\t fp  0x%08lx\n", context->arm_r3, context->arm_fp);
			printf("\tr4 0x%08lx\t ip  0x%08lx\n", context->arm_r4, context->arm_ip);
			printf("\tr5 0x%08lx\t sp  0x%08lx\n", context->arm_r5, context->arm_sp);
			printf("\tr6 0x%08lx\t lr  0x%08lx\n", context->arm_r6, context->arm_lr);
			printf("\tr7 0x%08lx\t pc  0x%08lx\n", context->arm_r7, context->arm_pc);
			printf("\tcpsr 0x%08lx\n", context->arm_cpsr);
	#endif
	}
	else if (regs)
	{
		printf("\n\tr0 0x%08x\t r8  0x%08x\n", regs[0], regs[8]);
		printf("\tr1 0x%08x\t r9  0x%08x\n", regs[1], regs[9]);
		printf("\tr2 0x%08x\t r10 0x%08x\n", regs[2], regs[10]);
		printf("\tr3 0x%08x\t fp  0x%08x\n", regs[3], regs[11]);
		printf("\tr4 0x%08x\t ip  0x%08x\n", regs[4], regs[12]);
		printf("\tr5 0x%08x\t sp  0x%08x\n", regs[5], regs[13]);
		printf("\tr6 0x%08x\t lr  0x%08x\n", regs[6], regs[14]);
		printf("\tr7 0x%08x\n", regs[7]);
	}
	else
	{
		printf("Cannot print registers\n");
	}
}

static void Debugger_seg_returnAddr(const code_segment_data_t* const segmentData, const uint32_t val, const code_seg_t* const CurrentCodeSeg)
{
	if (!CurrentCodeSeg->MIPSReturnRegister)
	{
		printf("Seg  0x%08x  \t0x%08x\t0x%08x\ttype: %s\n"
				"next segments:\n"
				, (uint32_t)CurrentCodeSeg, (uint32_t)CurrentCodeSeg->MIPScode
				, (uint32_t)CurrentCodeSeg->ARMcode
				, seg_type_s[CurrentCodeSeg->Type]);

		if (CurrentCodeSeg->pContinueNext != NULL)
		{
			printf(" (1) 0x%08x  \t0x%08x\t0x%08x\t (Continue)\n"
					, (uint32_t)CurrentCodeSeg->pContinueNext
					, (uint32_t)CurrentCodeSeg->pContinueNext->MIPScode
					, (uint32_t)CurrentCodeSeg->pContinueNext->ARMcode);
		}

		Instruction_t ins;
		code_seg_t* branch = NULL;
		if (CurrentCodeSeg->MIPScodeLen > 0U)
		{
			mips_decode(CurrentCodeSeg->MIPScode[CurrentCodeSeg->MIPScodeLen -1], &ins);

			branch = getSegmentAt((size_t)&CurrentCodeSeg->MIPScode[CurrentCodeSeg->MIPScodeLen -1U + ins.offset]);

			if (branch != NULL)
			{
				if (CurrentCodeSeg->MIPScode == branch->MIPScode)	//Loops on itself
				{
					printf(" (2) 0x%08x  \t0x%08x\t0x%08x\t (Loop)\n"
						, (uint32_t)branch
						, (uint32_t)branch->MIPScode
						, (uint32_t)branch->ARMcode);
				}
				else
				{
					printf(" (2) 0x%08x  \t0x%08x\t0x%08x\t (Branch)\n"
						, (uint32_t)branch
						, (uint32_t)branch->MIPScode
						, (uint32_t)branch->ARMcode);
				}
			}
		}
		if (!branch && !CurrentCodeSeg->pContinueNext)
		{
			printf("No linkage for code in its current location!\n");
		}
	}
	else
	{
		printf("Seg  0x%08x  \t0x%08x\t0x%08x\ttype: %s\nnext segments:\n"
				, (uint32_t)CurrentCodeSeg
				, (uint32_t)CurrentCodeSeg->MIPScode
				, (uint32_t)CurrentCodeSeg->ARMcode
				, seg_type_s[CurrentCodeSeg->Type]);

		/*code_seg_t*  tempCodeSeg=segmentData->StaticSegments;
		x=1;
		while (tempCodeSeg !=NULL)
		{
			if (tempCodeSeg->pBranchNext == CurrentCodeSeg)
			{
				printf(" (%d) 0x%08x  \t0x%08x\t0x%08x\n", x
						, (uint32_t)tempCodeSeg->pBranchNext
						, (uint32_t)tempCodeSeg->pBranchNext->MIPScode
						, (uint32_t)tempCodeSeg->pBranchNext->ARMcode);
				x++;
			}

			tempCodeSeg = tempCodeSeg->next;
		}*/
	}
}

static int Debugger_print(const code_segment_data_t* const segmentData, mcontext_t* context, size_t* regs)
{
	int x=0;
	char *tailPointer;

	if (!CMD_CMP(1, "arm"))
	{
		uint32_t count= CurrentCodeSeg->ARMcodeLen;
		uint32_t* addr;

		if (CurrentCodeSeg)
		{
			count = CurrentCodeSeg->ARMcodeLen;
			addr = CurrentCodeSeg->ARMcode;
		}
		else
		{
			count = 10;
			addr = (uint32_t*)0x81000000;
		}

		if (strlen(userInput[3]))
		{
			count = Mstrtoul(userInput[3], &tailPointer, 0);
			addr = (uint32_t*)((Mstrtoul(userInput[2], &tailPointer, 0))&~0x3);

			for (x=0; x< count; x++)
			{
					printf_arm((uint32_t)((uint32_t*)addr + x), *((uint32_t*)addr + x));
			}
			printLine();
			return 1;

		}
		else if (strlen(userInput[2]))
		{
			count = Mstrtoul(userInput[2], &tailPointer, 0);
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
				printf_arm((uint32_t)((uint32_t*)addr + x), *((uint32_t*)addr + x));
			}
			else if (addr + x < (uint32_t*)CurrentCodeSeg->ARMEntryPoint)
			{
				printf("0x%08x", (uint32_t)(addr + x));
#if SHOW_PRINT_ARM_VALUE
				printf("          ");
#endif
				printf("\t.word\t%12d (0x%08x)\n", *((uint32_t*)addr + x), *((uint32_t*)addr + x));
			}
			else
				printf_arm((uint32_t)((uint32_t*)addr + x), *((uint32_t*)addr + x));
		}

		printLine();
	}
	else if (!CMD_CMP(1, "mips"))
	{
		uint32_t count;
		uint32_t* addr;

		if (CurrentCodeSeg)
		{
			count = CurrentCodeSeg->MIPScodeLen;
			addr = CurrentCodeSeg->MIPScode;
		}
		else
		{
			count = 10;
			addr = (uint32_t*)0x88000040;
		}

		if (strlen(userInput[3]))
		{
			count = Mstrtoul(userInput[3], &tailPointer, 0);
			addr = (uint32_t*)((Mstrtoul(userInput[2], &tailPointer, 0))&~0x3);
		}
		else if (strlen(userInput[2]))
		{
			if (!strncasecmp(userInput[2], "0x", 2))
			{
				addr = (uint32_t*)((Mstrtoul(userInput[2], &tailPointer, 0))&~0x3);
			}
			else
			{
				count = Mstrtoul(userInput[2], &tailPointer, 0);
			}
		}

		if (NULL == addr)
		{
			printf("Invalid Address\n");
			return 0;
		}

		for (x=0; x< count; x++)
		{
			fprintf_mips(stdout, (uint32_t)((uint32_t*)addr + x), *((uint32_t*)addr + x));
		}

		// if printing segment then print next few instructions for Delay Slot analysis
		if (!strlen(userInput[2]))
		{
			printLine();

			for (x=CurrentCodeSeg->MIPScodeLen; x< CurrentCodeSeg->MIPScodeLen+3; x++)
			{
				fprintf_mips(stdout, (uint32_t)(CurrentCodeSeg->MIPScode + x), *(CurrentCodeSeg->MIPScode + x));
			}
			printf("\naddr 0x%x, len %d, return reg %d\n"
				"0x%08X %08X %02X (%d)\n"
				, (uint32_t)CurrentCodeSeg->MIPScode, CurrentCodeSeg->MIPScodeLen, CurrentCodeSeg->MIPSReturnRegister
				, CurrentCodeSeg->MIPSRegistersUsed[0], CurrentCodeSeg->MIPSRegistersUsed[1], CurrentCodeSeg->MIPSRegistersUsed[2]
				, CurrentCodeSeg->MIPSRegistersUsedCount);
		}
	}
	else if (!CMD_CMP(1, "value"))
	{
		uint32_t count = 10;
		uint32_t* addr;

		if (!CurrentCodeSeg)
		{
			addr = CurrentCodeSeg->ARMcode;
		}
		else
		{
			addr = (uint32_t*)0x81000000;
		}

		if (strlen(userInput[3]))
		{
			count = Mstrtoul(userInput[3], &tailPointer, 0);
			addr = (uint32_t*)((Mstrtoul(userInput[2], &tailPointer, 0))&~0x3);
		}
		else if (strlen(userInput[2]))
		{
			addr = (uint32_t*)Mstrtoul(userInput[2], &tailPointer, 0);
			count = 1;
		}

		for (x=0; x< count; x++)
		{
			printf("0x%08x\t0x%08x\t%11d\n",(uint32_t)((uint32_t*)addr + x), *((uint32_t*)addr + x), *((uint32_t*)addr + x));
		}

		printLine();
	}
	else if (!CMD_CMP(1, "intermediate"))
	{
		CodeSeg_print(CurrentCodeSeg);
	}
	else if (!CMD_CMP(1, "literals"))
	{
		Intermediate_Literals_print(CurrentCodeSeg);
	}
	else if (!CMD_CMP(1, "reg"))
	{
		if (strlen(userInput[2]))
		{
			if (!CMD_CMP(2, "mips"))
			{
				DebugRuntimePrintMIPS();
			}
			else if (!CMD_CMP(2, "arm"))
			{
				printregs(context,regs);
			}
		}
		else
		{
			DebugRuntimePrintMIPS();

			printregs(context,regs);
		}

		printLine();
	}
	else if (!CMD_CMP(1, "lookup"))
	{
		uint32_t val = Mstrtoul(userInput[2], &tailPointer, 0);
		printf("0x%08x => 0x%08x\n", (val),(uint32_t)getSegmentAt(val));
	}
	else
	{
		printf(HELP_PRINT);
	}
	return 1;
}

static int Debugger_seg(const code_segment_data_t* const segmentData)
{
	int32_t val;
	char *tailPointer;

	if (!strlen(userInput[1]))
	{
		printf("First Segment   0x%x\n", (uint32_t)getSegmentAt(0x88000040));

		printf("Current Segment 0x%x\n"
				"\tMIPS            ARM\n"
				"\t0x%08X %u\t0x%08X %u \tEntry Point: 0x%08X\n\n"
				, (uint32_t)CurrentCodeSeg
				, (uint32_t)CurrentCodeSeg->MIPScode
				, CurrentCodeSeg->MIPScodeLen
				, (uint32_t)CurrentCodeSeg->ARMcode
				, CurrentCodeSeg->ARMcodeLen
				, (uint32_t)CurrentCodeSeg->ARMEntryPoint);
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
	else if (!CMD_CMP(1, "branch"))
	{
		CurrentCodeSeg = segmentData->segBranchUnknown;
	}
	else if (!CMD_CMP(1, "interrupt"))
	{
		CurrentCodeSeg = segmentData->segInterrupt;
	}
	else if (!CMD_CMP(1, "get"))
	{
		uint32_t count = 10;
		uint32_t *addr;

		addr = (uint32_t*)Mstrtoul(userInput[2], &tailPointer, 0);

		if (strlen(userInput[3]))
		{
			count = Mstrtoul(userInput[3], &tailPointer, 0);
		}

		int i;

		for (i=0; i < count; i++)
		{
			printf("0x%08x\t0x%08x\n", (uint32_t)(addr + i), (uint32_t)getSegmentAt((size_t)(addr + i)));
		}
	}
	else
	{
		val = (uint32_t*)Mstrtoul(userInput[1], &tailPointer, 0);
		int ok = 0;

		code_seg_t* tempCodeSeg;

		if ((uint32_t)val > 2)
		{
			tempCodeSeg=getSegmentAt((size_t)val);

			if (tempCodeSeg)
			{
				CurrentCodeSeg = tempCodeSeg;
				ok = 1;
			}
			else
			{
				CurrentCodeSeg = (code_seg_t*)val;
				ok = 1;
			}
		}else if (!CurrentCodeSeg->MIPSReturnRegister)
		{
			Instruction_t ins;
			mips_decode(CurrentCodeSeg->MIPScode[CurrentCodeSeg->MIPScodeLen -1], &ins);


			code_seg_t* branch = getSegmentAt((size_t)CurrentCodeSeg->MIPScode + CurrentCodeSeg->MIPScodeLen -1 + ins.offset);

			if (0 == val)
			{
				CurrentCodeSeg = getSegmentAt(0x88000040);
				ok = 1;
			}
			else if (1 == val && CurrentCodeSeg->pContinueNext != NULL)
			{
				CurrentCodeSeg = CurrentCodeSeg->pContinueNext;
				ok = 1;
			}
			else if (2 == val && branch != NULL)
			{
				CurrentCodeSeg = branch;
				ok = 1;
			}
		}
		else
		{
			tempCodeSeg = getSegmentAt(0x88000040);
			int x = 1;
			while (tempCodeSeg != NULL)
			{
				if (tempCodeSeg == (code_seg_t*)val)
				{
					CurrentCodeSeg = (code_seg_t*)val;
					ok = 1;
					break;
				}
				else if (tempCodeSeg->pBranchNext == CurrentCodeSeg)
				{
					if (x == val)
					{
						CurrentCodeSeg = tempCodeSeg->pBranchNext;
						ok = 1;
						break;
					}
					x++;
				}

				tempCodeSeg = tempCodeSeg->next;
			}
		}
		if (!ok) printf("Invalid entry\n");
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

	for (x = 0U; x < COUNTOF(bounds); x++)
	{
			if (!strlen(userInput[1+x])) break;

			val = Mstrtoul(userInput[1+x], &tailPointer, 0);

			//test for numbers
			if (tailPointer != userInput[1+x])
			{
				bounds[x] = val;
			}
			else
			{
				// scan for names
				for (y = 0U; y < COUNTOF(Translations); y++)
				{
					if (!CMD_CMP(1 + x, Translations[y].name)){
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
		if (strlen(userInput[2])
				&& !CMD_CMP(2, "help"))
		{
			int x;
			for (x=0; x < COUNTOF(Translations); x++)
			{
				printf("%d. %s\n", x, Translations[x].name);
			}
			return 0;
		}

		if (!strlen(userInput[2]))
		{
			bounds[1] = bounds[0];
		}

		for (x = bounds[0]; x <= bounds[1]; x++)
		{
			printf("Translate_%s:\n", Translations[x].name);
			Translations[x].function(CurrentCodeSeg);
		}

		if (x < COUNTOF(Translations))
		{
			CodeSeg_print(CurrentCodeSeg);
		}
		else
		{
			uint32_t count = CurrentCodeSeg->ARMcodeLen;
			uint32_t* addr = CurrentCodeSeg->ARMcode;
			uint32_t x;

			for (x = 0U; x < count; x++)
			{
				if (addr + x == (uint32_t*)CurrentCodeSeg->ARMEntryPoint)
				{
					printf(".EntryPoint:\n");
					printf_arm((uint32_t)((uint32_t*)addr + x), *((uint32_t*)addr + x));
				}
				else if (addr + x < (uint32_t*)CurrentCodeSeg->ARMEntryPoint)
					printf("\t.word\t%12d (0x%08x)\n", *((uint32_t*)addr + x), *((uint32_t*)addr + x));
				else
					printf_arm((uint32_t)((uint32_t*)addr + x), *((uint32_t*)addr + x));
			}
		}
	}

	return 0;
}


static code_seg_t* breakpoints[] = {NULL, NULL, NULL, NULL, NULL};

static Debugger_breakpoint()
{
	uint32_t x;
	char* tailPointer;

	if (!CMD_CMP(0, "breakpoint"))
	{
		if (strlen(userInput[1]))
		{
			uint32_t v = Mstrtoul(userInput[1], &tailPointer, 0);
			for (x=0U; x < COUNTOF(breakpoints); x++)
			{
				if (breakpoints[x] == NULL)
				{
					breakpoints[x] = (code_seg_t*)v;
					break;
				}
			}
		}
		else
		{
			for (x=0U; x < COUNTOF(breakpoints); x++)
			{
				if (breakpoints[x] != NULL)
				{
					printf("%d. 0x%x\n", x, breakpoints[x]);
				}
			}
		}
	}
	else if (!CMD_CMP(0, "delete"))
	{
		if (strlen(userInput[1]))
		{
			uint32_t v = Mstrtoul(userInput[1], &tailPointer, 0);

			if (v < COUNTOF(breakpoints))
			{
				breakpoints[v] = NULL;
			}
			else
			{
				code_seg_t* delSegBreakpoint = (code_seg_t*)v;
				for (x=0U; x < COUNTOF(breakpoints); x++)
				{
					if (breakpoints[x] == delSegBreakpoint)
					{
						breakpoints[x] = NULL;
						break;
					}
				}
			}
		}
	}
}


void ServiceBreakPoint(code_seg_t* codeSeg, size_t* regs)
{
	uint32_t x;

	for (x = 0U; x < COUNTOF(breakpoints); x++)
	{
		if (codeSeg == breakpoints[x])
		{
			while(Debugger_start(&segmentData, NULL, regs));
		}
	}
}

void DebugRuntimePrintSegment()
{
	static code_seg_t* lastCurrentSegment = NULL;

	//Only print out when the current segment changes
	if (lastCurrentSegment != segmentData.dbgCurrentSegment)
	{
		printf("Current Segment 0x%08x. ARM start 0x%08x. MIPS 0x%08x\n", (uint32_t)segmentData.dbgCurrentSegment, (uint32_t)segmentData.dbgCurrentSegment->ARMcode, (uint32_t)segmentData.dbgCurrentSegment->MIPScode);
		lastCurrentSegment = (code_seg_t*)segmentData.dbgCurrentSegment;
	}

	//DebugRuntimePrintMIPS();
}

void DebugRuntimePrintMIPS()
{
	uint32_t x;

	for (x=0U; x < 16U; x++)
	{
		printf( "\tr%-2d 0x%08x%08x\tr%-2d 0x%08x%08x\n"
				,  0U + x, *(uint32_t*)(MMAP_FP_BASE + RegMemByteOffset(x + REG_WIDE)), *(uint32_t*)(MMAP_FP_BASE + RegMemByteOffset(x))
				, 16U + x, *(uint32_t*)(MMAP_FP_BASE + RegMemByteOffset(16 + x + REG_WIDE)), *(uint32_t*)(MMAP_FP_BASE + RegMemByteOffset(16 + x)));
	}

	printf("\n");

	for (x=0U; x < 16U; x++)
	{
		printf( "\tf%-2d 0x%08x%08x\tf%-2d 0x%08x%08x\n"
						,  0U + x, *(uint32_t*)(MMAP_FP_BASE + RegMemByteOffset(x + REG_WIDE + REG_FP)), *(uint32_t*)(MMAP_FP_BASE + RegMemByteOffset(x + REG_FP))
						, 16U + x, *(uint32_t*)(MMAP_FP_BASE + RegMemByteOffset(16 + x+REG_WIDE + REG_FP)), *(uint32_t*)(MMAP_FP_BASE + RegMemByteOffset(16 + x + REG_FP)));
	}

	printf("\n\tBadVaddr 0x%08x  PC     0x%08x%08x\n"
			"\tCount    0x%08x  FCR0   0x%08x\n"
			"\tEntryHi  0x%08x  FCR31  0x%08x\n"
			"\tCompare  0x%08x  MultHi 0x%08x%08x\n"
			"\tStatus   0x%08x  MultLo 0x%08x%08x\n"
			"\tCause    0x%08x  LLBit  0x%01x\n"
			"\tContext  0x%08x  EPC    0x%08x\n"
			,*(uint32_t*)(MMAP_FP_BASE + REG_BADVADDR*4)
			,*(uint32_t*)(MMAP_FP_BASE + REG_PC*4)
			,*(uint32_t*)(MMAP_FP_BASE + REG_PC*4+4)
			,*(uint32_t*)(MMAP_FP_BASE + REG_COUNT*4)
			,*(uint32_t*)(MMAP_FP_BASE + REG_FCR0*4)
			,*(uint32_t*)(MMAP_FP_BASE + REG_ENTRYHI*4)
			,*(uint32_t*)(MMAP_FP_BASE + REG_FCR31*4)
			,*(uint32_t*)(MMAP_FP_BASE + REG_COMPARE*4)
			,*(uint32_t*)(MMAP_FP_BASE + REG_MULTHI*4)
			,*(uint32_t*)(MMAP_FP_BASE + REG_MULTHI*4+4)
			,*(uint32_t*)(MMAP_FP_BASE + REG_STATUS*4)
			,*(uint32_t*)(MMAP_FP_BASE + REG_MULTLO*4)
			,*(uint32_t*)(MMAP_FP_BASE + REG_MULTLO*4+4)
			,*(uint32_t*)(MMAP_FP_BASE + REG_CAUSE*4)
			,*(uint32_t*)(MMAP_FP_BASE + REG_LLBIT*4)
			,*(uint32_t*)(MMAP_FP_BASE + REG_CONTEXT*4)
			,*(uint32_t*)(MMAP_FP_BASE + REG_EPC*4));

	return;
}

void Debugger_wrapper(size_t* regs)
{
	while (Debugger_start(&segmentData, 0, regs));
}

void Debugger_set(const code_segment_data_t* const segmentData)
{
	if (strlen(userInput[1]) > 0
			&& strlen(userInput[2]) > 0)
	{
		char* tailPointer;
		uint64_t v = strtoul(userInput[2], &tailPointer, 0);

		if (!CMD_CMP(1, "showPrintSegmentDelete"))
		{
			showPrintSegmentDelete = (uint32_t)v;
			printf("showPrintSegmentDelete = %u\n", (uint32_t)v);
		}
		else if (!CMD_CMP(1, "showRegTranslationMap"))
		{
			showRegTranslationMap = (uint32_t)v;
			printf("showRegTranslationMap = %u\n", (uint32_t)v);
		}
		else if (!CMD_CMP(1, "showRegTranslationMapProgress"))
		{
			showRegTranslationMapProgress = (uint32_t)v;
			printf("showRegTranslationMapProgress = %u\n", (uint32_t)v);
		}
		else
		{
			printf(HELP_SET);
		}
	}
	else
	{
		printf(HELP_SET);
	}
}

int Debugger_start(const code_segment_data_t* const segmentData, mcontext_t* context, size_t* regs)
{
	//find segment
	if (!CurrentCodeSeg) CurrentCodeSeg = (code_seg_t*)segmentData->dbgCurrentSegment;

	printf("> "); fflush(stdin);
	getCmd();

	if (userInput[0][0] == '\0') return 1;

	if (!CMD_CMP(0, "quit"))
	{
		exit(0);
	}
	else if (!CMD_CMP(0, "continue"))
	{
		CurrentCodeSeg = NULL;
		return 0;
	}
	else if (!CMD_CMP(0, "print"))
	{
		Debugger_print(segmentData, context, regs);
	}
	else if (!CMD_CMP(0, "segment"))
	{
		Debugger_seg(segmentData);
	}
	else if (!CMD_CMP(0, "set"))
	{
		Debugger_set(segmentData);
	}
	else if (!CMD_CMP(0, "translate"))
	{
		Debugger_translate(segmentData);
	}
	else if (!CMD_CMP(0, "compile"))
	{
		char* tailPointer;
		uint32_t val = Mstrtoul(userInput[1], &tailPointer, 0);

		if (!val)
		{
			printf(HELP_COMP);
			return 1;
		}

		CurrentCodeSeg = CompileCodeAt((uint32_t*)val);

	}
	else if (!CMD_CMP(0, "start"))
	{
		pfvru1 run = (pfvru1)segmentData->segStart->ARMEntryPoint;

		printf("Starting ...\n");

		CurrentCodeSeg = NULL; // So that if emulation errors then Debugger will look at correct segment
		uint32_t ret = run();

		printf("End run: %u (0x%x)\n", ret, ret);
	}
	else if (!CMD_CMP(0, "breakpoint")
			|| !CMD_CMP(0, "delete"))
	{
		Debugger_breakpoint();
	}
	else if (!CMD_CMP(0, "help"))
	{
		if 		(!CMD_CMP(1, "print"))		printf(HELP_PRINT);
		else if (!CMD_CMP(1, "segment"))	printf(HELP_SEG);
		else if (!CMD_CMP(1, "compile"))	printf(HELP_COMP);
		else if (!CMD_CMP(1, "translate"))
		{
			int x;
			for (x=0; x < COUNTOF(Translations); x++)
			{
				printf("%d. %s\n", x, Translations[x].name);
			}
		}
		else printf(HELP_GEN);
	}
	else
	{
		printf("unknown command: %s\n", userInput[0]);
	}
	return 1;
}
