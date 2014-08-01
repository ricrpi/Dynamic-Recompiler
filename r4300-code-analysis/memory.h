/*
 * memory.h
 *
 *  Created on: 24 Mar 2014
 *      Author: ric_rpi
 */

#ifndef MEMORY_H_
#define MEMORY_H_

#define sl(mot) \
		( \
				((mot & 0x000000FF) << 24) | \
				((mot & 0x0000FF00) <<  8) | \
				((mot & 0x00FF0000) >>  8) | \
				((mot & 0xFF000000) >> 24) \
		)





#define MMAP_BASE			(0x80000000)	// This is the base address for MMAP. It is also where RDRAM starts

#define MMAP_DR_BASE		(0x81000000)	//Base address where Dynamic compiled code is written to.
#define MMAP_DR_SIZE		(0x02000000)	//Dynamic compiled code space size (32MB).

/*
 *  MMAP_FP_BASE is where the REG_HOST_FP predominately points to i.e. where the emulated MIPS registers
 *  reside in memory. It will also contain the Function pointers for C code so that recompiled code can jump
 *  just using LDR pc [fp, #offset].
 */
#define MMAP_FP_BASE		(0x83F10000)

#define MMAP_FUNC			(MMAP_FP_BASE + 676)

#define FUNC_GEN_INTERRUPT 		(MMAP_FUNC + 0)
#define FUNC_SEG_BRANCH_UNKNOWN (MMAP_FUNC + 4)
#define FUNC_MEM_LOOKUP			(MMAP_FUNC + 8)

#define ROM_ADDRESS		 	(MMAP_BASE + MMAP_BASE_SIZE)


#define RD_RAM_SIZE 		(0x00800000)
#define MMAP_BASE_SIZE 		(0x08000000)	// This is the base size (excluding ROM and PIF areas)

#endif /* MEMORY_H_ */
