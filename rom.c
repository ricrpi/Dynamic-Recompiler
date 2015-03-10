/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - rom.c                                                   *
 *   Mupen64Plus homepage: http://code.google.com/p/mupen64plus/           *
 *   Copyright (C) 2014 Richard Hender                                     *
 *   Copyright (C) 2002 Hacktarux                                          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "rom.h"
#include "m64p_types.h"

rom_params        ROM_PARAMS;

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
