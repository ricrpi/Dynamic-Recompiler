

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
	unsigned char *ROM_buffer = (unsigned char *) malloc(romlength);
	m64p_rom_header ROM_HEADER;

	if (fread(ROM_buffer, 1, romlength, fPtr) != romlength)
	{
		printf("could not read ROM\n");
		return 3;
	}

	swap_rom(ROM_buffer, &imagetype, romlength);

	memcpy(&ROM_HEADER, ROM_buffer, sizeof(m64p_rom_header));

	printf("Name: %s\n", ROM_HEADER.Name);
	printf("Rom size: %ld bytes (or %ld Mb or %ld Megabits)\n", romlength, romlength/1024/1024, romlength/1024/1024*8);
	printf("ClockRate = %x\n", sl(ROM_HEADER.ClockRate));
	printf("Version: %x\n", sl(ROM_HEADER.Release));
	printf("PC = 0x%x\n\n", sl((unsigned int)ROM_HEADER.PC));

	int x;
	//program blocks start at
	for (x=sizeof(m64p_rom_header)/4; x < 1024; x++)
		decode(x*4, sl(ROM_buffer[x]));

	free(ROM_buffer);

	printf("\nFinished processing ROM\n");
	return 0;
}



