/*
 * rom.c
 *
 *  Created on: 24 Mar 2014
 *      Author: ric_rpi
 */

#include "rom.h"


/* If rom is a .v64 or .n64 image, byteswap or wordswap loadlength amount of
 * rom data to native .z64 before forwarding. Makes sure that data extraction
 * and MD5ing routines always deal with a .z64 image.
 */
void swap_rom(unsigned char* localrom, unsigned char* imagetype, int loadlength)
{
	unsigned char temp;
	int i;

	/* Btyeswap if .v64 image. */
	if(localrom[0]==0x37)
	{
		*imagetype = V64IMAGE;
		for (i = 0; i < loadlength; i+=2)
		{
			temp=localrom[i];
			localrom[i]=localrom[i+1];
			localrom[i+1]=temp;
		}
	}
	/* Wordswap if .n64 image. */
	else if(localrom[0]==0x40)
	{
		*imagetype = N64IMAGE;
		for (i = 0; i < loadlength; i+=4)
		{
			temp=localrom[i];
			localrom[i]=localrom[i+3];
			localrom[i+3]=temp;
			temp=localrom[i+1];
			localrom[i+1]=localrom[i+2];
			localrom[i+2]=temp;
		}
	}
	else
		*imagetype = Z64IMAGE;
}
