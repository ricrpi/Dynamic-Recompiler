/*
 * memory.h
 *
 *  Created on: 24 Mar 2014
 *      Author: ric_rpi
 */

#ifndef MEMORY_H_
#define MEMORY_H_

#include <stdint.h>

extern uint8_t uMemoryBase;

#define sl(mot) \
		( \
				((mot & 0x000000FF) << 24) | \
				((mot & 0x0000FF00) <<  8) | \
				((mot & 0x00FF0000) >>  8) | \
				((mot & 0xFF000000) >> 24) \
		)

#define MMAP_BASE			((uint32_t)uMemoryBase << 24)	// This is the base address for MMAP. It is also where RDRAM starts



/*
 *  MMAP_FP_BASE is where the REG_HOST_FP predominately points to i.e. where the emulated MIPS registers
 *  reside in memory. It will also contain the Function pointers for C code so that recompiled code can jump
 *  just using LDR pc [fp, #offset].
 */

#define MMAP_CODE_SEG_BASE  (MMAP_BASE - (sizeof(void*) * 8 * 1024 * 1024 / 4))
#define MMAP_STATIC_REGION  (MMAP_BASE + (0x08000000))
#define MMAP_DR_BASE		(MMAP_BASE + (0x01000000))	//Base address where Dynamic compiled code is written to.
#define MMAP_FP_BASE		(MMAP_BASE + (0x03F01028))	// must be > 0x03F00027 + 4096 (4096 being for global literals)

//#define MMAP_FUNC			(MMAP_FP_BASE + 676)
#define MMAP_FUNC			(676)
#define MMAP_DR_SIZE		(0x02000000)	//Dynamic compiled code space size (32MB).
#define RD_RAM_SIZE 		(0x00800000)
#define MMAP_BASE_SIZE 		(0x08000000 + sizeof(void*) * 8 * 1024 * 1024 / 4 )	// This is the base size (excluding ROM and PIF areas)



#define FUNC_GEN_START			(MMAP_FUNC + 0)
#define FUNC_GEN_STOP			(MMAP_FUNC + 4)
#define FUNC_GEN_INTERRUPT 		(MMAP_FUNC + 8)
#define FUNC_GEN_BRANCH_UNKNOWN (MMAP_FUNC + 12)
#define FUNC_GEN_LOOKUP			(MMAP_FUNC + 16)

#define ROM_ADDRESS		 	(MMAP_STATIC_REGION)

#endif /* MEMORY_H_ */
