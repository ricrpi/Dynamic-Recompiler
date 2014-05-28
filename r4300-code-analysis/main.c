

/*
 * decomp.c
 *
 *  Created on: 19 Mar 2014
 *      Author: rjhender
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rom.h"
#include "memory.h"
#include "CodeSegments.h"
#include "InstructionSetMIPS4.h"
#include "Debugger.h"

uint32_t * ROM_buffer = NULL;

int main(int argc, char* argv[])
{
	code_segment_data_t* segmentData;

	if (argc <= 1)
	{
		printf("Please specify a file\n");
		return 1;
	}

	printf("R4300 Decompiler\n\nOpening %s\n",argv[1]);

	FILE *fPtr = fopen(argv[1], "rb");
	if (fPtr == NULL) return 2;

	long romlength = 0;
	fseek(fPtr, 0L, SEEK_END);
	romlength = ftell(fPtr);
	fseek(fPtr, 0L, SEEK_SET);

	unsigned char imagetype;
	ROM_buffer = (uint32_t *) malloc(romlength);
	m64p_rom_header ROM_HEADER;

	if (fread(ROM_buffer, 1, romlength, fPtr) != romlength)
	{
		printf("could not read ROM\n");
		return 3;
	}

	swap_rom((unsigned char*)ROM_buffer, &imagetype, romlength);

	memcpy(&ROM_HEADER, ROM_buffer, sizeof(m64p_rom_header));

	printf("Name: %s\n", ROM_HEADER.Name);
	printf("Rom size: %ld bytes (or %ld Mb or %ld Megabits)\n", romlength, romlength/1024/1024, romlength/1024/1024*8);
	printf("ClockRate = %x\n", sl(ROM_HEADER.ClockRate));
	printf("Version: %x\n", sl(ROM_HEADER.Release));
	printf("PC = 0x%x\n\n", sl((unsigned int)ROM_HEADER.PC));

	int x;

#if 1
	for (x=0; x< romlength/4; x++)
		ROM_buffer[x] = sl(ROM_buffer[x]);
#endif


#if 0
	for (x=0x40/4; x< 172/4; x++ )
	{
		ops_decode((uint32_t)&ROM_buffer[x], ROM_buffer[x]);
	}

	printf("----------------------------\n");
#endif

	//Find all code where we don't know which registers are used
#if 1
	printf("Unknown register usage on these instructions:\n");
	for (x=0x40/4; x< romlength/4; x++ )
	{
		uint32_t temp;
		if (ops_regs_input(ROM_buffer[x],&temp,&temp,&temp) == 2
				|| ops_regs_output(ROM_buffer[x],&temp,&temp,&temp) == 2) ops_decode((x)*4, ROM_buffer[x]);

	}

	printf("----------------------------\n");
#endif


	segmentData = GenerateCodeSegmentData(ROM_buffer, romlength);

	printf("MIPS Address            Length   Regs-cpu   fpu      sp     used Next       Block type 2=end,3=br\n");

	code_seg_t* nextCodeSeg = segmentData->FirstSegment;
	int count =0;
	while (nextCodeSeg != NULL && count < 200)
	{
		count++;

		if (nextCodeSeg->MIPSReturnRegister)
		{
			printf("0x%08X 0x%08X %5d      0x%08X %08X %03X     %2d     r%d\n",
				(uint32_t)nextCodeSeg->MIPScode,
				(uint32_t)(nextCodeSeg->MIPScode+nextCodeSeg->MIPScodeLen),
				nextCodeSeg->MIPScodeLen,
				nextCodeSeg->MIPSRegistersUsed[0],
				nextCodeSeg->MIPSRegistersUsed[1],
				nextCodeSeg->MIPSRegistersUsed[2],
				nextCodeSeg->MIPSRegistersUsedCount,
				nextCodeSeg->MIPSReturnRegister);
		}
		else
		{
			printf("0x%08X 0x%08X %5d      0x%08X %08X %02X     %2d   0x%08X %d\n",
				(uint32_t)nextCodeSeg->MIPScode,
				(uint32_t)(nextCodeSeg->MIPScode+nextCodeSeg->MIPScodeLen),
				nextCodeSeg->MIPScodeLen,
				nextCodeSeg->MIPSRegistersUsed[0],
				nextCodeSeg->MIPSRegistersUsed[1],
				nextCodeSeg->MIPSRegistersUsed[2],
				nextCodeSeg->MIPSRegistersUsedCount,
				(uint32_t)nextCodeSeg->MIPSnextInstructionIndex,
				nextCodeSeg->blockType);
		}

		nextCodeSeg = nextCodeSeg->nextCodeSegmentLinkedList;
	}



	printf("%d code segments generated\n", segmentData->count);

	printf("\nFinished processing ROM\n");

	Debugger_start(segmentData);


	free(ROM_buffer);

	printf("\nEND\n");
	return 0;
}



