
#ifndef TRANSLATE_H_
#define TRANSLATE_H_

#include "InstructionSet.h"
#include "CodeSegments.h"

extern uint32_t bCountSaturates;


// ---------------- Function Pointers -------------------------------

typedef void (*pfu4v)(unsigned int, unsigned int, unsigned int, unsigned int);
typedef void (*pf4v)(int, int, int, int);

typedef void (*pfu1v)(unsigned int);
typedef void (*pf1v)(int);

typedef unsigned int (*pfu4ru1)(unsigned int, unsigned int, unsigned int, unsigned int);
typedef int (*pf4r1)(int, int, int, int);

typedef unsigned int (*pfu1ru1)(unsigned int);
typedef int (*pf1r1)(int);

typedef unsigned int (*pfvru1)();
typedef int (*pfvr1)();

typedef void (*pfvv)(void);

// -----------------------------------------------------------------

typedef enum
{
	VOLATILE,
	CONSTANT_U64,
	CONSTANT_U32,
	CONSTANT_U16,
	CONSTANT_U8,
	CONSTANT_I64,
	CONSTANT_I32,
	CONSTANT_I16,
	CONSTANT_I8,
} regType_e;

typedef struct _RegMap
{
	regType_e 	type;
	uint64_t	value;
	uint8_t		bLoaded;
} regMap_t;



code_seg_t* Generate_MemoryTranslationCode(code_segment_data_t* seg_data, pfu1ru1 f);
code_seg_t* Generate_CodeStart(code_segment_data_t* seg_data);
code_seg_t* Generate_CodeStop(code_segment_data_t* seg_data);
code_seg_t* Generate_ISR(code_segment_data_t* seg_data);
code_seg_t* Generate_BranchUnknown(code_segment_data_t* seg_data);
code_seg_t* Generate_MIPS_Trap(code_segment_data_t* seg_data);


void Translate_init(code_seg_t* const codeSegment);

void Translate_DelaySlot(code_seg_t* codeSegment);

/*
 * MIPS4300 has a COUNT register that is decremented every instruction
 * when it underflows, an interrupt is triggered.
 */
void Translate_CountRegister(code_seg_t* const codeSegment);

void Translate_Constants(code_seg_t* const codeSegment);

/*
 * Function to turn 64bit registers into multiple 32-bit registers
 *
 */
void Translate_32BitRegisters(code_seg_t* const codeSegment);

void Translate_Generic(code_seg_t* const codeSegment);

void Translate_FPU(code_seg_t* const codeSegment);

void Translate_Trap(code_seg_t* const codeSegment);

void Translate_Branch(code_seg_t* const codeSegment);

void Translate_Memory(code_seg_t* const codeSegment);

void Translate_LoadStoreWriteBack(code_seg_t* const codeSegment);

void Translate_LoadCachedRegisters(code_seg_t* const codeSegment);

/*
 * Function to re-number / reduce the number of registers so that they fit the HOST
 *
 * This function will need to scan a segment and when more than the number of spare
 * HOST registers is exceded, choose to eith save register(s) into the emulated registers
 * referenced by the Frame Pointer or push them onto the stack for later use.
 *
 * Pushing onto the stack may make it easier to use LDM/SDM where 32/64 bit is not compatible
 * with the layout of the emulated register space.
 *
 */
void Translate_Registers(code_seg_t* const codeSegment);

void Translate_StoreCachedRegisters(code_seg_t* const codeSegment);

void Translate(code_seg_t* const codeSegment);

void Translate_Literals(const code_seg_t* const codeSegment);

#endif /* TRANSLATE_H_ */
