

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


uint32_t * ROM_buffer = NULL;
code_seg_t* First_seg = NULL;
code_seg_t* Last_seg = NULL;

/*
void decode(uint32_t addr)
{
	uint32_t caddr, branch;
	int x;
	caddr = addr;
	uint32_t uiCodeLen;

	int bOK = 1;

	while (bOK)
	{
		if (find_seg(caddr)) return;	//we have compiled this segment before

		//from the supplied address get the code segment
		int bValid = ops_validCodeSegment(ROM_buffer, caddr/4, 4096, &uiCodeLen, &branch);

		//for (x=0; x< 200; x++)
		//	ops_decode((addr+x*4), ROM_buffer[addr/4 + x]);

		// if we have a valid code segment
		if (bValid > 0)
		{
			// get the last code instruction type
			mips_op_t op_type = ops_type(ROM_buffer[caddr/4 + uiCodeLen-1]);

			printf("decoding segment 0x%08X with %d instructions\n", caddr, uiCodeLen);


			for (x=0; x< uiCodeLen; x++) ops_decode((caddr+x*4), ROM_buffer[caddr/4 + x]);

			//the segment could jump or branch
			// if it branches then lets recursively follow
			// else continue processing

			//how do we handle simple loops?

			//This must be a new segment
			new_seg(addr, uiCodeLen, branch);

			if ((op_type & OPS_JUMP) == OPS_JUMP)
			{
				printf("jumping to 0x%08X\n", branch);
				caddr = caddr + uiCodeLen/4 + 4;
			}
			else if ((op_type & OPS_BRANCH) == OPS_BRANCH)
			{
				if (branch < 0x1000000) {
					printf("branching to 0x%08X\n", branch);
					decode(branch);

				}
				caddr = caddr + uiCodeLen*4;
				printf("continue from 0x%08X\n", caddr);
			}
			else
			{
				printf( "_____________________\n");
				printf("we have gone wrong. code_len=%d, type=%d\n\t", uiCodeLen, op_type);
				ops_decode((caddr + uiCodeLen*4), ROM_buffer[caddr/4 + uiCodeLen]);

				for (x=0; x < uiCodeLen + 4; x++)
							ops_decode((caddr+x*4), ROM_buffer[caddr/4 + x]);
				printf( "_____________________\n");

				bOK =0;
			}

		}
		else if (0 == uiCodeLen)
		{
			printf("segment scan length too short\n");
		}
		else
		{
			printf( "_____________________\n"
					"invalid code segment! from 0x%08X\n\n", caddr);
			for (x=-8; x < 2; x++)
			{
				if (-1 == x) printf("\n");
				ops_decode((caddr + (uiCodeLen + x)* 4), ROM_buffer[caddr/4 + uiCodeLen + x]);
				if (-1 == x) printf("\n");
			}
				printf( "_____________________\n");

			bOK = 0;
		}
	}
}*/

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
	for (x=0x40/4; x< 0x00001118/4; x++ )
	{
		ops_decode((x)*4, ROM_buffer[x]);
	}

	printf("----------------------------\n");
#endif

	segmentData = GenerateCodeSegmentData(ROM_buffer, romlength);

	printf("MIPS Address Length   ARM Address Length    Next\n");

	for (x=0; x < segmentData->count; x++)
	{
		if (segmentData->segments[x].ReturnRegister)
		{
			printf("0x%08X %5d      0x%08X %6d      r%d\n",
				segmentData->segments[x].MIPScode,
				segmentData->segments[x].MIPScodeLen,
				segmentData->segments[x].ARMcode,
				segmentData->segments[x].ARMcodeLen,
				segmentData->segments[x].ReturnRegister);
		}
		else
		{
			printf("0x%08X %5d      0x%08X %6d      0x%08X\n",
				segmentData->segments[x].MIPScode,
				segmentData->segments[x].MIPScodeLen,
				segmentData->segments[x].ARMcode,
				segmentData->segments[x].ARMcodeLen,
				(uint32_t*)segmentData->segments[x].next);
		}


	}

	free(ROM_buffer);

	printf("There are %d code segments\n", segmentData->count);

	printf("\nFinished processing ROM\n");
	return 0;
}



