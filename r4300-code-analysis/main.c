

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
#include "mips.h"

uint32_t * ROM_buffer = NULL;
code_seg_t* First_seg = NULL;
code_seg_t* Last_seg = NULL;
static uint32_t uiCountSegments = 0;

void new_seg(uint32_t start, uint32_t uiNumInstructions, uint32_t branch)
{
	code_seg_t* new_seg;
	new_seg = malloc(sizeof(code_seg_t));

	if (First_seg == NULL) First_seg = new_seg;
	else Last_seg->pNext_code_seg = new_seg;
	Last_seg = new_seg;

	new_seg->pNext_code_seg = NULL;
	new_seg->pCodeStart = start;
	new_seg->pBranch = branch;
	new_seg->uiNumInstructions = uiNumInstructions;



	uiCountSegments++;

}

uint32_t find_seg(uint32_t address)
{
	code_seg_t* seg = First_seg;
	while(seg != NULL)
	{
		if (address == seg->pCodeStart) return 1;
		seg = seg->pNext_code_seg;
	}

	return 0;
}

void decode(uint32_t addr)
{
	uint32_t caddr, branch;
	int x;
	caddr = addr;

	int bOK = 1;

	while (bOK)
	{
		if (find_seg(caddr)) return;	//we have compiled this segment before

		//from the supplied address get the code segment
		int code_len = ops_validCodeSegment(&ROM_buffer[caddr/4], 1024, &branch);

		//for (x=0; x< 200; x++)
		//	ops_decode((addr+x*4), ROM_buffer[addr/4 + x]);

		// if we have a valid code segment
		if (code_len > 0)
		{
			// get the last code instruction type
			mips_op_t op_type = ops_type(ROM_buffer[caddr/4 + code_len-1]);

			printf("decoding segment 0x%08X with %d instructions\n", caddr, code_len);


			for (x=0; x< code_len; x++) ops_decode((caddr+x*4), ROM_buffer[caddr/4 + x]);

			//the segment could jump or branch
			// if it branches then lets recursively follow
			// else continue processing

			//how do we handle simple loops?

			//This must be a new segment
			new_seg(addr, code_len, branch);

			if ((op_type & OPS_JUMP) == OPS_JUMP)
			{
				printf("jumping to 0x%08X\n", branch);
				caddr = caddr + code_len/4 + 4;
			}
			else if ((op_type & OPS_BRANCH) == OPS_BRANCH)
			{
				if (branch < 0x1000000) {
					printf("branching to 0x%08X\n", branch);
					decode(branch);

				}
				caddr = caddr + code_len*4;
				printf("continue from 0x%08X\n", caddr);
			}
			else
			{
				printf( "_____________________\n");
				printf("we have gone wrong. code_len=%d, type=%d\n\t", code_len, op_type);
				ops_decode((caddr + code_len*4), ROM_buffer[caddr/4 + code_len]);

				for (x=0; x < code_len + 4; x++)
							ops_decode((caddr+x*4), ROM_buffer[caddr/4 + x]);
				printf( "_____________________\n");

				bOK =0;
			}

		}
		else
		{
			printf( "_____________________\n"
					"invalid code segment! at 0x%08X\n", caddr);
			for (x=-2; x < 8; x++)
				ops_decode((caddr+x*4), ROM_buffer[caddr/4 + x]);
			printf( "_____________________\n");

			bOK = 0;
		}
	}
}

int main(int argc, char* argv[])
{

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

	int address = sizeof(m64p_rom_header);

#if 0
	int x;

	for (x=0; x< romlength/4; x++)
		ROM_buffer[x] = sl(ROM_buffer[x]);
#endif

	decode(address);

	free(ROM_buffer);

	printf("There are %d code segments\n", uiCountSegments);

	printf("\nFinished processing ROM\n");
	return 0;
}



