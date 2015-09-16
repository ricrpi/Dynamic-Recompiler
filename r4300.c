/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - r4300.c                                                 *
 *   Mupen64Plus homepage: http://code.google.com/p/mupen64plus/           *
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

#include <stdlib.h>
#include <string.h>

//#include "api/m64p_types.h"
//#include "api/callbacks.h"
//#include "api/debugger.h"
#include "memory.h"
//#include "main/main.h"
#include "rom.h"

//#include "r4300.h"
//#include "ops.h"
//#include "exception.h"
//#include "interupt.h"
//#include "macros.h"
//#include "recomp.h"
//#include "recomph.h"
//#include "new_dynarec/new_dynarec.h"

#include "InstructionSet.h"
#include "tlb.h"

static uint64_t* reg;
static uint32_t* reg_cop0;

#ifdef DBG
#include "debugger/dbg_types.h"
#include "debugger/debugger.h"
#endif

unsigned int r4300emu = 0;
int no_compiled_jump = 0;
int llbit, rompause;
#if NEW_DYNAREC != NEW_DYNAREC_ARM
int stop;
long long int reg[32], hi, lo;
unsigned int reg_cop0[32];
float *reg_cop1_simple[32];
double *reg_cop1_double[32];
int FCR0, FCR31;
unsigned int next_interupt;
precomp_instr *PC;
#endif
long long int local_rs;
long long int reg_cop1_fgr_64[32];
tlb tlb_e[32];
unsigned int delay_slot, skip_jump = 0, dyna_interp = 0, last_addr;
unsigned long long int debug_count = 0;
unsigned int CIC_Chip;
char invalid_code[0x100000];

//precomp_block *blocks[0x100000], *actual;
int rounding_mode = 0x33F, trunc_mode = 0xF3F, round_mode = 0x33F,
    ceil_mode = 0xB3F, floor_mode = 0x73F;

// -----------------------------------------------------------
// Cached interpreter functions (and fallback for dynarec).
// -----------------------------------------------------------
#ifdef DBG
#define UPDATE_DEBUGGER() if (g_DebuggerActive) update_debugger(PC->addr)
#else
#define UPDATE_DEBUGGER() do { } while(0)
#endif

#define PCADDR PC->addr
#define ADD_TO_PC(x) PC += x;
#define DECLARE_INSTRUCTION(name) static void name(void)

#define CHECK_MEMORY() \
   if (!invalid_code[address>>12]) \
      if (blocks[address>>12]->block[(address&0xFFF)/4].ops != \
          current_instruction_table.NOTCOMPILED) \
         invalid_code[address>>12] = 1;

// -----------------------------------------------------------
// Flow control 'fake' instructions
// -----------------------------------------------------------
#if 0
static void FIN_BLOCK(void)
{
   if (!delay_slot)
     {
    jump_to((PC-1)->addr+4);
/*#ifdef DBG
            if (g_DebuggerActive) update_debugger(PC->addr);
#endif
Used by dynarec only, check should be unnecessary
*/
    PC->ops();
    if (r4300emu == CORE_DYNAREC) dyna_jump();
     }
   else
     {
    precomp_block *blk = actual;
    precomp_instr *inst = PC;
    jump_to((PC-1)->addr+4);

/*#ifdef DBG
            if (g_DebuggerActive) update_debugger(PC->addr);
#endif
Used by dynarec only, check should be unnecessary
*/
    if (!skip_jump)
      {
         PC->ops();
         actual = blk;
         PC = inst+1;
      }
    else
      PC->ops();

    if (r4300emu == CORE_DYNAREC) dyna_jump();
     }
}


static void NOTCOMPILED(void)
{
   unsigned int *mem = fast_mem_access(blocks[PC->addr>>12]->start);
#ifdef CORE_DBG
   DebugMessage(M64MSG_INFO, "NOTCOMPILED: addr = %x ops = %lx", PC->addr, (long) PC->ops);
#endif

   if (mem != NULL)
      recompile_block((int *)mem, blocks[PC->addr >> 12], PC->addr);
   else
      DebugMessage(M64MSG_ERROR, "not compiled exception");

/*#ifdef DBG
            if (g_DebuggerActive) update_debugger(PC->addr);
#endif
The preceeding update_debugger SHOULD be unnecessary since it should have been
called before NOTCOMPILED would have been executed
*/
   PC->ops();
   if (r4300emu == CORE_DYNAREC)
     dyna_jump();
}

static void NOTCOMPILED2(void)
{
   NOTCOMPILED();
}


static unsigned int update_invalid_addr(unsigned int addr)
{
   if (addr >= 0x80000000 && addr < 0xa0000000)
     {
    if (invalid_code[addr>>12]) invalid_code[(addr+0x20000000)>>12] = 1;
    if (invalid_code[(addr+0x20000000)>>12]) invalid_code[addr>>12] = 1;
    return addr;
     }
   else if (addr >= 0xa0000000 && addr < 0xc0000000)
     {
    if (invalid_code[addr>>12]) invalid_code[(addr-0x20000000)>>12] = 1;
    if (invalid_code[(addr-0x20000000)>>12]) invalid_code[addr>>12] = 1;
    return addr;
     }
   else
     {
    unsigned int paddr = virtual_to_physical_address(addr, 2);
    if (paddr)
      {
         unsigned int beg_paddr = paddr - (addr - (addr&~0xFFF));
         update_invalid_addr(paddr);
         if (invalid_code[(beg_paddr+0x000)>>12]) invalid_code[addr>>12] = 1;
         if (invalid_code[(beg_paddr+0xFFC)>>12]) invalid_code[addr>>12] = 1;
         if (invalid_code[addr>>12]) invalid_code[(beg_paddr+0x000)>>12] = 1;
         if (invalid_code[addr>>12]) invalid_code[(beg_paddr+0xFFC)>>12] = 1;
      }
    return paddr;
     }
}

#define addr jump_to_address
unsigned int jump_to_address;
void jump_to_func(void)
{
   unsigned int paddr;
   if (skip_jump) return;
   paddr = update_invalid_addr(addr);
   if (!paddr) return;
   actual = blocks[addr>>12];
   if (invalid_code[addr>>12])
     {
    if (!blocks[addr>>12])
      {
         blocks[addr>>12] = (precomp_block *) malloc(sizeof(precomp_block));
         actual = blocks[addr>>12];
         blocks[addr>>12]->code = NULL;
         blocks[addr>>12]->block = NULL;
         blocks[addr>>12]->jumps_table = NULL;
         blocks[addr>>12]->riprel_table = NULL;
      }
    blocks[addr>>12]->start = addr & ~0xFFF;
    blocks[addr>>12]->end = (addr & ~0xFFF) + 0x1000;
    init_block(blocks[addr>>12]);
     }
   PC=actual->block+((addr-actual->start)>>2);

   if (r4300emu == CORE_DYNAREC) dyna_jump();
}
#undef addr

void generic_jump_to(unsigned int address)
{
   if (r4300emu == CORE_PURE_INTERPRETER)
      PC->addr = address;
   else {
#ifdef NEW_DYNAREC
      if (r4300emu == CORE_DYNAREC)
         last_addr = pcaddr;
      else
         jump_to(address);
#else
      jump_to(address);
#endif
   }
}

/* Refer to Figure 6-2 on page 155 and explanation on page B-11
   of MIPS R4000 Microprocessor User's Manual (Second Edition)
   by Joe Heinrich.
*/
void shuffle_fpr_data(int oldStatus, int newStatus)
{
#if defined(M64P_BIG_ENDIAN)
    const int isBigEndian = 1;
#else
    const int isBigEndian = 0;
#endif

    if ((newStatus & 0x04000000) != (oldStatus & 0x04000000))
    {
        int i;
        int temp_fgr_32[32];

        // pack or unpack the FGR register data
        if (newStatus & 0x04000000)
        {   // switching into 64-bit mode
            // retrieve 32 FPR values from packed 32-bit FGR registers
            for (i = 0; i < 32; i++)
            {
                temp_fgr_32[i] = *((int *) &reg_cop1_fgr_64[i>>1] + ((i & 1) ^ isBigEndian));
            }
            // unpack them into 32 64-bit registers, taking the high 32-bits from their temporary place in the upper 16 FGRs
            for (i = 0; i < 32; i++)
            {
                int high32 = *((int *) &reg_cop1_fgr_64[(i>>1)+16] + (i & 1));
                *((int *) &reg_cop1_fgr_64[i] + isBigEndian)     = temp_fgr_32[i];
                *((int *) &reg_cop1_fgr_64[i] + (isBigEndian^1)) = high32;
            }
        }
        else
        {   // switching into 32-bit mode
            // retrieve the high 32 bits from each 64-bit FGR register and store in temp array
            for (i = 0; i < 32; i++)
            {
                temp_fgr_32[i] = *((int *) &reg_cop1_fgr_64[i] + (isBigEndian^1));
            }
            // take the low 32 bits from each register and pack them together into 64-bit pairs
            for (i = 0; i < 16; i++)
            {
                unsigned int least32 = *((unsigned int *) &reg_cop1_fgr_64[i*2] + isBigEndian);
                unsigned int most32 = *((unsigned int *) &reg_cop1_fgr_64[i*2+1] + isBigEndian);
                reg_cop1_fgr_64[i] = ((unsigned long long) most32 << 32) | (unsigned long long) least32;
            }
            // store the high bits in the upper 16 FGRs, which wont be accessible in 32-bit mode
            for (i = 0; i < 32; i++)
            {
                *((int *) &reg_cop1_fgr_64[(i>>1)+16] + (i & 1)) = temp_fgr_32[i];
            }
        }
    }
}

void set_fpr_pointers(int newStatus)
{
    int i;
#if defined(M64P_BIG_ENDIAN)
    const int isBigEndian = 1;
#else
    const int isBigEndian = 0;
#endif

    // update the FPR register pointers
    if (newStatus & 0x04000000)
    {
        for (i = 0; i < 32; i++)
        {
            reg_cop1_double[i] = (double*) &reg_cop1_fgr_64[i];
            reg_cop1_simple[i] = ((float*) &reg_cop1_fgr_64[i]) + isBigEndian;
        }
    }
    else
    {
        for (i = 0; i < 32; i++)
        {
            reg_cop1_double[i] = (double*) &reg_cop1_fgr_64[i>>1];
            reg_cop1_simple[i] = ((float*) &reg_cop1_fgr_64[i>>1]) + ((i & 1) ^ isBigEndian);
        }
    }
}

int check_cop1_unusable(void)
{
   if (!(Status & 0x20000000))
     {
    Cause = (11 << 2) | 0x10000000;
    exception_general();
    return 1;
     }
   return 0;
}

void update_count(void)
{
#ifdef NEW_DYNAREC
    if (r4300emu != CORE_DYNAREC)
    {
#endif
        Count = Count + (PC->addr - last_addr)/2;
        last_addr = PC->addr;
#ifdef NEW_DYNAREC
    }
#endif

#ifdef COMPARE_CORE
   if (delay_slot)
     CoreCompareCallback();
#endif
/*#ifdef DBG
   if (g_DebuggerActive && !delay_slot) update_debugger(PC->addr);
#endif
*/
}

void init_blocks(void)
{
   int i;
   for (i=0; i<0x100000; i++)
   {
      invalid_code[i] = 1;
      blocks[i] = NULL;
   }
}

void free_blocks(void)
{
   int i;
   for (i=0; i<0x100000; i++)
   {
        if (blocks[i])
        {
            free_block(blocks[i]);
            free(blocks[i]);
            blocks[i] = NULL;
        }
    }
}

#endif
/* this hard reset function simulates the boot-up state of the R4300 CPU */
void r4300_reset_hard(void)
{
    unsigned int i;

    reg = (uint64_t*)MMAP_FP_BASE;
    reg_cop0 = (uint32_t*)MMAP_FP_BASE + REG_CO;

    // clear r4300 registers and TLB entries
    for (i = 0; i < 32; i++)
    {
        reg[i]=0;
        reg_cop0[i]=0;
        reg_cop1_fgr_64[i]=0;

        // --------------tlb------------------------
        tlb_e[i].mask=0;
        tlb_e[i].vpn2=0;
        tlb_e[i].g=0;
        tlb_e[i].asid=0;
        tlb_e[i].pfn_even=0;
        tlb_e[i].c_even=0;
        tlb_e[i].d_even=0;
        tlb_e[i].v_even=0;
        tlb_e[i].pfn_odd=0;
        tlb_e[i].c_odd=0;
        tlb_e[i].d_odd=0;
        tlb_e[i].v_odd=0;
        tlb_e[i].r=0;
        //tlb_e[i].check_parity_mask=0x1000;

        tlb_e[i].start_even=0;
        tlb_e[i].end_even=0;
        tlb_e[i].phys_even=0;
        tlb_e[i].start_odd=0;
        tlb_e[i].end_odd=0;
        tlb_e[i].phys_odd=0;
    }
    for (i=0; i<0x100000; i++)
    {
        tlb_LUT_r[i] = 0;
        tlb_LUT_w[i] = 0;
    }
    *((uint32_t*)MMAP_FP_BASE + REG_LLBIT) =0;
    *((uint32_t*)MMAP_FP_BASE + REG_MULTHI) = 0;
    *((uint32_t*)MMAP_FP_BASE + REG_MULTLO) = 0;
    *((uint32_t*)MMAP_FP_BASE + REG_FCR0) = 0x511;
    *((uint32_t*)MMAP_FP_BASE + REG_FCR31) = 0;

    // set COP0 registers
    *((uint32_t*)MMAP_FP_BASE + REG_RANDOM) = 31;
    *((uint32_t*)MMAP_FP_BASE + REG_STATUS) = 0x34000000;

    //set_fpr_pointers(Status);

    *((uint32_t*)MMAP_FP_BASE + REG_CONFIG)= 0x6e463;
    *((uint32_t*)MMAP_FP_BASE + REG_PRID) = 0xb00;
    *((uint32_t*)MMAP_FP_BASE + REG_COUNT) = 0x5000;
    *((uint32_t*)MMAP_FP_BASE + REG_CAUSE) = 0x5C;
    *((uint32_t*)MMAP_FP_BASE + REG_CONTEXT) = 0x7FFFF0;
    *((uint32_t*)MMAP_FP_BASE + REG_EPC) = 0xFFFFFFFF;
    *((uint32_t*)MMAP_FP_BASE + REG_BADVADDR) = 0xFFFFFFFF;
    *((uint32_t*)MMAP_FP_BASE + REG_ERROREPC) = 0xFFFFFFFF;

    rounding_mode = 0x33F;
}

/* this soft reset function simulates the actions of the PIF ROM, which may vary by region
 * TODO: accurately simulate the effects of the PIF ROM in the case of a soft reset
 *       (e.g. Goldeneye crashes) */
void r4300_reset_soft(void)
{
    long long CRC = 0;
    unsigned int i;

    extern unsigned int SP_DMEM[0x1000/4*2];
    extern unsigned int *SP_IMEM;

    // copy boot code from ROM to SP_DMEM
    memcpy((char *)SP_DMEM+0x40, (uint32_t*)(ROM_ADDRESS+0x40), 0xFC0);

   // the following values are extracted from the pj64 source code
   // thanks to Zilmar and Jabo

   reg[6] = 0xFFFFFFFFA4001F0CLL;
   reg[7] = 0xFFFFFFFFA4001F08LL;
   reg[8] = 0x00000000000000C0LL;
   reg[10]= 0x0000000000000040LL;
   reg[11]= 0xFFFFFFFFA4000040LL;
   reg[29]= 0xFFFFFFFFA4001FF0LL;

    // figure out which ROM type is loaded
   for (i = 0x40/4; i < (0x1000/4); i++)
     CRC += SP_DMEM[i];

   switch(CRC) {
    case 0x000000D0027FDF31LL:
    case 0x000000CFFB631223LL:
      CIC_Chip = 1;
      break;
    case 0x000000D057C85244LL:
      CIC_Chip = 2;
      break;
    case 0x000000D6497E414BLL:
      CIC_Chip = 3;
      break;
    case 0x0000011A49F60E96LL:
      CIC_Chip = 5;
      break;
    case 0x000000D6D5BE5580LL:
      CIC_Chip = 6;
      break;
    default:
      CIC_Chip = 2;
   }

   switch(ROM_PARAMS.systemtype)
   {
      case SYSTEM_PAL:
    switch (CIC_Chip) {
     case 2:
       reg[5] = 0xFFFFFFFFC0F1D859LL;
       reg[14]= 0x000000002DE108EALL;
       break;
     case 3:
       reg[5] = 0xFFFFFFFFD4646273LL;
       reg[14]= 0x000000001AF99984LL;
       break;
     case 5:
       SP_IMEM[1] = 0xBDA807FC;
       reg[5] = 0xFFFFFFFFDECAAAD1LL;
       reg[14]= 0x000000000CF85C13LL;
       reg[24]= 0x0000000000000002LL;
       break;
     case 6:
       reg[5] = 0xFFFFFFFFB04DC903LL;
       reg[14]= 0x000000001AF99984LL;
       reg[24]= 0x0000000000000002LL;
       break;
    }
    reg[23]= 0x0000000000000006LL;
    reg[31]= 0xFFFFFFFFA4001554LL;
    break;
      case SYSTEM_NTSC:
      default:
    switch (CIC_Chip) {
     case 2:
       reg[5] = 0xFFFFFFFFC95973D5LL;
       reg[14]= 0x000000002449A366LL;
       break;
     case 3:
       reg[5] = 0xFFFFFFFF95315A28LL;
       reg[14]= 0x000000005BACA1DFLL;
       break;
     case 5:
       SP_IMEM[1] = 0x8DA807FC;
       reg[5] = 0x000000005493FB9ALL;
       reg[14]= 0xFFFFFFFFC2C20384LL;
       break;
     case 6:
       reg[5] = 0xFFFFFFFFE067221FLL;
       reg[14]= 0x000000005CD2B70FLL;
       break;
    }
    reg[20]= 0x0000000000000001LL;
    reg[24]= 0x0000000000000003LL;
    reg[31]= 0xFFFFFFFFA4001550LL;
   }

   switch (CIC_Chip) {
    case 1:
      reg[22]= 0x000000000000003FLL;
      break;
    case 2:
      reg[1] = 0x0000000000000001LL;
      reg[2] = 0x000000000EBDA536LL;
      reg[3] = 0x000000000EBDA536LL;
      reg[4] = 0x000000000000A536LL;
      reg[12]= 0xFFFFFFFFED10D0B3LL;
      reg[13]= 0x000000001402A4CCLL;
      reg[15]= 0x000000003103E121LL;
      reg[22]= 0x000000000000003FLL;
      reg[25]= 0xFFFFFFFF9DEBB54FLL;
      break;
    case 3:
      reg[1] = 0x0000000000000001LL;
      reg[2] = 0x0000000049A5EE96LL;
      reg[3] = 0x0000000049A5EE96LL;
      reg[4] = 0x000000000000EE96LL;
      reg[12]= 0xFFFFFFFFCE9DFBF7LL;
      reg[13]= 0xFFFFFFFFCE9DFBF7LL;
      reg[15]= 0x0000000018B63D28LL;
      reg[22]= 0x0000000000000078LL;
      reg[25]= 0xFFFFFFFF825B21C9LL;
      break;
    case 5:
      SP_IMEM[0] = 0x3C0DBFC0;
      SP_IMEM[2] = 0x25AD07C0;
      SP_IMEM[3] = 0x31080080;
      SP_IMEM[4] = 0x5500FFFC;
      SP_IMEM[5] = 0x3C0DBFC0;
      SP_IMEM[6] = 0x8DA80024;
      SP_IMEM[7] = 0x3C0BB000;
      reg[2] = 0xFFFFFFFFF58B0FBFLL;
      reg[3] = 0xFFFFFFFFF58B0FBFLL;
      reg[4] = 0x0000000000000FBFLL;
      reg[12]= 0xFFFFFFFF9651F81ELL;
      reg[13]= 0x000000002D42AAC5LL;
      reg[15]= 0x0000000056584D60LL;
      reg[22]= 0x0000000000000091LL;
      reg[25]= 0xFFFFFFFFCDCE565FLL;
      break;
    case 6:
      reg[2] = 0xFFFFFFFFA95930A4LL;
      reg[3] = 0xFFFFFFFFA95930A4LL;
      reg[4] = 0x00000000000030A4LL;
      reg[12]= 0xFFFFFFFFBCB59510LL;
      reg[13]= 0xFFFFFFFFBCB59510LL;
      reg[15]= 0x000000007A3C07F4LL;
      reg[22]= 0x0000000000000085LL;
      reg[25]= 0x00000000465E3F72LL;
      break;
   }

}


