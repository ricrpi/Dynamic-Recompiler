/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - rom.h                                                   *
 *   Mupen64Plus homepage: http://code.google.com/p/mupen64plus/           *
 *   Copyright (C) 2008 Tillin9                                            *
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

#ifndef __ROM_H__
#define __ROM_H__

/* ROM Loading and Saving functions */

typedef struct
{
	unsigned char init_PI_BSB_DOM1_LAT_REG;  /* 0x00 */
	unsigned char init_PI_BSB_DOM1_PGS_REG;  /* 0x01 */
	unsigned char init_PI_BSB_DOM1_PWD_REG;  /* 0x02 */
	unsigned char init_PI_BSB_DOM1_PGS_REG2; /* 0x03 */
	unsigned int ClockRate;                  /* 0x04 */
	unsigned int PC;                         /* 0x08 */
	unsigned int Release;                    /* 0x0C */
	unsigned int CRC1;                       /* 0x10 */
	unsigned int CRC2;                       /* 0x14 */
	unsigned int Unknown[2];                 /* 0x18 */
	unsigned char Name[20];                  /* 0x20 */
	unsigned int unknown;                    /* 0x34 */
	unsigned int Manufacturer_ID;            /* 0x38 */
	unsigned short Cartridge_ID;             /* 0x3C - Game serial number  */
	unsigned short Country_code;             /* 0x3E */
} m64p_rom_header;

/* Supported rom compressiontypes. */
enum 
{
    UNCOMPRESSED,
    ZIP_COMPRESSION,
    GZIP_COMPRESSION,
    BZIP2_COMPRESSION,
    LZMA_COMPRESSION,
    SZIP_COMPRESSION
};

/* Supported rom image types. */
enum 
{
    Z64IMAGE,
    V64IMAGE,
    N64IMAGE
};

/* Supported CIC chips. */
enum
{
    CIC_NUS_6101,
    CIC_NUS_6102,
    CIC_NUS_6103,
    CIC_NUS_6105,
    CIC_NUS_6106
};

/* Supported save types. */
enum
{
    EEPROM_4KB,
    EEPROM_16KB,
    SRAM,
    FLASH_RAM,
    CONTROLLER_PACK,
    NONE
};


void swap_rom(unsigned char* localrom, unsigned char* imagetype, int loadlength);


#endif /* __ROM_H__ */

