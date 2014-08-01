/*
 * Translate.c
 *
 *  Created on: 16 Apr 2014
 *      Author: rjhender
 */

#include "CodeSegments.h"
#include "InstructionSet.h"
#include "InstructionSetMIPS4.h"
#include "InstructionSetARM6hf.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include "memory.h"
#include "Translate.h"

#define ADD_LL_NEXT(x, y) (x)->nextInstruction = (y)->nextInstruction; \
			(y)->nextInstruction = (x); \
			(y) = (y)->nextInstruction;

typedef enum
{
	AVAILABLE,
	CLEAN,
	DIRTY,
	CONSTANT,
	RESERVED
} RegisterState;

uint32_t bCountSaturates = 0;
uint32_t uiCountFrequency = 40;	// must be less than 128 else may not be able to encode in imm8
uint32_t bMemoryInlineLookup = 0;
uint32_t bMemoryOffsetCheck = 0;
uint32_t bDoDMAonInterrupt = 1;

#if 0
/*
 * Code to generate Emulation-time Code within 32MB of dynamically compiled code.
 *
 * When a Code segment is written into memory, it may not be known where it branches to. In which case
 * It will be necessary to branch to this Stub code.
 *
 * Requirements:
 * 	The address of the branch instruction to patch
 * 	The tgt address/offset to branch to
 *
 * Method:
 *
 * 	The address of the instruction to patch is LR-4
 *
 * 	If we know what the current segment is then we can lookup the target address/offset from the raw MIPS code
 *
 */
Instruction_t* Generate_BranchStubCode()
{
	Instruction_t* firstInstruction;
	Instruction_t* newInstruction;

	firstInstruction 	= newInstrPUSH(AL, REG_HOST_STM_LR);

	newInstruction 		= newInstr(ADD, AL, REG_HOST_LR, REG_HOST_PC, REG_NOT_USED, 4);

	firstInstruction->nextInstruction = newInstruction;

	newInstruction 		= newInstrPOP(AL, REG_HOST_STM_PC);

	return firstInstruction;

}
#endif

/*
 * Function to generate Emulation-time code for calculating the Hosts memory address to use.
 *
 * Emulation Args:
 * 			R0 base
 * 			R1 offset
 *
 * Emulation Returns:
 *  		R0 Host memory address
 *			Z=1 if !bDoDMAonInterrupt && last byte is 0x5, so run DMA?
 */
code_seg_t* Generate_MemoryTranslationCode(pfu1ru1 f)
{
	code_seg_t* 	code_seg 		= newSegment();
	Instruction_t* 	newInstruction;
	Instruction_t* 	ins 			= NULL;

	if (bMemoryInlineLookup){
		printf("Generate_MemoryTranslationCode() and bMemoryInlineLookup\n");
		return NULL;
	}

	//Add the base and offset together
	newInstruction 		= newInstr(ARM_ADD, AL, REG_TEMP_MEM1, REG_HOST_R0, REG_HOST_R1, 0);
	code_seg->Intermcode = ins = newInstruction;

	//shift Right so that we have the final Byte
	newInstruction 		= newInstr(SRL, AL, REG_TEMP_MEM2, REG_TEMP_MEM1, REG_NOT_USED, 0);
	newInstruction->I = 1;
	newInstruction->shift = 24;
	ADD_LL_NEXT(newInstruction, ins);

	//If the final Byte is 0x80 then all is good
	newInstruction 		= newInstr(ARM_CMP, AL, REG_NOT_USED, REG_TEMP_MEM2, REG_NOT_USED, 0x80);
	newInstruction->I = 1;
	ADD_LL_NEXT(newInstruction, ins);

	//Move the address to R0
	newInstruction 		= newInstr(ARM_MOV, EQ, REG_HOST_R0, REG_NOT_USED, REG_TEMP_MEM1, 0);
	ADD_LL_NEXT(newInstruction, ins);

	// Return
	newInstruction 		= newInstr(ARM_MOV, EQ, REG_HOST_PC, REG_NOT_USED, REG_HOST_LR, 0);
	ADD_LL_NEXT(newInstruction, ins);

	//Else is the final byte 0xA0?
	newInstruction 		= newInstr(ARM_CMP, AL, REG_NOT_USED, REG_TEMP_MEM2, REG_NOT_USED, 0xA0);
	newInstruction->I = 1;
	ADD_LL_NEXT(newInstruction, ins);

	//If so then clear bit 0x40
	newInstruction 		= newInstr(ARM_BIC, EQ, REG_HOST_R0, REG_TEMP_MEM1, REG_NOT_USED, 0x40);
	newInstruction->I = 1;
	newInstruction->shift = 24;
	newInstruction->shiftType = LOGICAL_LEFT;
	ADD_LL_NEXT(newInstruction, ins);

	// Return
	newInstruction 		= newInstr(ARM_MOV, EQ, REG_HOST_PC, REG_NOT_USED, REG_HOST_LR, 0);
	ADD_LL_NEXT(newInstruction, ins);

	//The address must be Virtual

	newInstruction 		= newInstrPUSH(AL, REG_HOST_STM_EABI | REG_HOST_STM_LR);
	ADD_LL_NEXT(newInstruction, ins);

	//TODO call C function for Lookup?
	newInstruction 		= newInstr(ARM_MOV, EQ, REG_HOST_LR, REG_NOT_USED, REG_HOST_PC, 0);
	//The literal !!!
	newInstruction 		= newInstr(ARM_LDR_LIT, EQ, REG_HOST_PC, REG_HOST_PC, REG_NOT_USED, 0);

	newInstruction 		= newInstrPOP(AL, REG_HOST_STM_EABI| REG_HOST_STM_LR);
	ADD_LL_NEXT(newInstruction, ins);

	//Return
	newInstruction 		= newInstr(ARM_MOV, AL, REG_HOST_PC, REG_NOT_USED, REG_HOST_LR, 0);
	ADD_LL_NEXT(newInstruction, ins);

	Translate_Registers(code_seg);

	return code_seg;
}


//=============================================================

static Instruction_t* insertCall(Instruction_t* ins, const Condition_e cond, const int32_t offset)
{
	Instruction_t* newInstruction;

	//push lr
	newInstruction 	= newInstrPUSH(AL, REG_HOST_STM_LR);
	ADD_LL_NEXT(newInstruction, ins);

	//set lr
	newInstruction 	= newInstr(ADD, AL, REG_HOST_LR, REG_HOST_PC, REG_NOT_USED, 4);
	ADD_LL_NEXT(newInstruction, ins);

	// load function address from [fp + offset] into PC
	newInstruction 	= newInstr(ARM_LDR, cond, REG_HOST_PC, REG_HOST_FP, REG_NOT_USED, offset);
	ADD_LL_NEXT(newInstruction, ins);

	// pop lr
	newInstruction 	= newInstrPOP(AL, REG_HOST_STM_LR);
	ADD_LL_NEXT(newInstruction, ins);

	return ins;
}


/*
 * MIPS4300 executes instruction after a branch or jump if it is not LINK_LIKELY
 * Therefore the instruction after the branch may need to be moved into the segment.
 *
 * For ARM we shall use the PSR to signal when to execute the DelaySlot instruction(s)
 *
 * i.e.
 *
 * ...								...
 * ADD 	R1	R2	R3					ADD		R1	R2	R3
 * SUBI	R1	R1	#4					SUMI	R1	R1	#4
 * BNE	R1			#-1		==>	  + ADDI	R2	R2 	#3
 * ====New Segment====				BNE		R1		#-1
 * ADDI	R2	R2	#3				  + msr		s		#1		// TODO lookup what imm is for setting Z
 * ...								====New Segment====		// Will be a BLOCK_START_CONT Segment
 * ...						    +/- ADDIne	R2	R2	#3		// the delaySlot instruction(s)
 * ...								...
 *
 * All segments must clear the Z status flag before jumping to a BLOCK_START_CONT segment.
 *
 */
void Translate_DelaySlot(code_seg_t* const codeSegment)
{
	Instruction_e ops = ops_type(*(codeSegment->MIPScode + codeSegment->MIPScodeLen -1));
	Instruction_t* newInstruction;
	Instruction_t* ins;

	ins 			= codeSegment->Intermcode;

	if (ins == NULL)
		{
			printf("Not initialized this code segment. Please run 'optimize intermediate'\n");
			return;
		}

	//if the last instruction is a branch or call and is not Likely
	if ((ops & (OPS_BRANCH | OPS_CALL))
			&& !(ops & OPS_LIKELY))
	{
		newInstruction 	= newEmptyInstr();

		//goto second last instruction
		while (ins->nextInstruction->nextInstruction)
			ins = ins->nextInstruction;

		mips_decode(*(codeSegment->MIPScode + codeSegment->MIPScodeLen), newInstruction);

		ADD_LL_NEXT(newInstruction, ins);

		//Set Status Register setting
		/*	APSR.N = imm32<31>;
			APSR.Z = imm32<30>;
			APSR.C = imm32<29>;
			APSR.V = imm32<28>;
			APSR.Q = imm32<27>;
		 */

		ins = ins->nextInstruction;	// move to the branch instruction

		newInstruction 	= newInstr(ARM_MSR,AL, REG_NOT_USED, REG_NOT_USED, REG_NOT_USED, 0x40);
		newInstruction->rotate = 4;	// To load into bits 24-31
		newInstruction->I = 1;

		ADD_LL_NEXT(newInstruction, ins);
	}
	// if segment before this one can continue, then we must make first instruction conditional
	else if(codeSegment->Type == SEG_START
			|| codeSegment->Type == SEG_ALONE)
	{
		//goto second last instruction
		/*while (ins->nextInstruction->nextInstruction)
			ins = ins->nextInstruction;

		//Clear Status Register setting
		newInstruction 	= newInstr(ARM_MSR,AL, REG_NOT_USED, REG_NOT_USED, REG_NOT_USED, 0);
		newInstruction->rotate = 4;	// To load into bits 24-31
		newInstruction->I = 1;

		ADD_LL_NEXT(newInstruction, ins);*/
	}

	// if segment before this one can continue, then we must make first instruction conditional
	else if (codeSegment->Type == SEG_SANDWICH
			|| codeSegment->Type == SEG_END){

		codeSegment->Intermcode->cond = NE;	//Make first instruction conditional
	}
}

/*
 * MIPS4300 has a COUNT register that is decremented every instruction
 * when it underflows, an interrupt is triggered.
 */
void Translate_CountRegister(code_seg_t* const codeSegment)
{
	Instruction_t*ins = codeSegment->Intermcode;
	uint32_t instrCount =0;
	uint32_t instrCountRemaining = codeSegment->MIPScodeLen;

	if (ins == NULL)
	{
		printf("Not initialized this code segment. Please run 'optimize intermediate'\n");
		return;
	}

	if (bCountSaturates)
	{
		printf("Optimize_CountRegister failed. Not implemented QSUB \n");
		return;
	}

	//loop through the instructions and update COUNT every countFrequency

	while (ins->nextInstruction->nextInstruction)
	{
		instrCount++;

		if (instrCount >= uiCountFrequency && instrCountRemaining >= uiCountFrequency)
		{
			//add COUNT update
			Instruction_t* newInstruction 	= newEmptyInstr();

			if (bCountSaturates)
			{
				//TODO QSUB
			}
			else
			{
				newInstruction = newInstrS(ARM_SUB,AL,REG_COUNT,REG_COUNT,REG_NOT_USED,uiCountFrequency);
				ADD_LL_NEXT(newInstruction, ins);

				instrCountRemaining -= uiCountFrequency;

				ins = insertCall(ins, MI, FUNC_GEN_INTERRUPT);
				instrCount = 0;
			}
		}

		ins = ins->nextInstruction;
	}
	//now add a final update before end of function

	if (instrCount && instrCountRemaining)
	{
		Instruction_t* newInstruction 	= newEmptyInstr();

		//create COUNT update instructions
		if (bCountSaturates)
		{
			//TODO QSUB
		}
		else
		{
			newInstruction = newInstrS(ARM_SUB,AL,REG_COUNT,REG_COUNT,REG_NOT_USED,instrCountRemaining);

			newInstruction->nextInstruction = ins->nextInstruction;
			ADD_LL_NEXT(newInstruction, ins);

			ins = insertCall(ins, MI, FUNC_GEN_INTERRUPT);

			return;
		}
	}
}

/*
 * Function to turn 64bit registers into multiple 32-bit registers
 *
 */
void Translate_32BitRegisters(code_seg_t* const codeSegment)
{
	//Instruction_t* i = newInstr();

	Instruction_t*ins;
	ins = codeSegment->Intermcode;

	while (ins)
	{
	/*	switch (ins->instruction)
		{
		case DADDIU: //TODO this is an immediate.
			//Change into 32bit then add instruction to accumulate
			ins->instruction = ADDIU;

			i->instruction = ARM_ADC;
			i->M_Rs = 32 + ins->M_Rs;
			i->M_Rt = 32 + ins->M_Rt;
			i->Rd = 32 + ins->Rd;

			break;
		default:
			break;
		}
*/
		ins = ins->nextInstruction;
	}
}


static uint32_t FindRegNextUsedAgain(const Instruction_t* const ins, const reg_t Reg)
{
	const Instruction_t* in = ins;
	uint32_t x = 0;

	while (in)
	{
		if (ins->R1 == Reg || ins->R2 == Reg || ins->R3 == Reg)	return x;
		x++;
		in = in->nextInstruction;
	}

	return x;
}

static void UpdateRegWithReg(Instruction_t* const ins, const reg_t RegFrom, const reg_t RegTo, uint32_t MaxInstructions)
{
	Instruction_t* in = ins;
	uint32_t x = MaxInstructions;

	if (!x) x = 0xffffffff;

	while (x && in)
	{
		if (in->Rd1 == RegFrom) in->Rd1 = RegTo;
		if (in->Rd2 == RegFrom) in->Rd2 = RegTo;
		if (in->R1 == RegFrom) in->R1 = RegTo;
		if (in->R2 == RegFrom) in->R2 = RegTo;
		if (in->R3 == RegFrom) in->R3 = RegTo;
		x--;
		in = in->nextInstruction;
	}
}

/*
 * Function to re-number / reduce the number of registers so that they fit the HOST
 *
 * This function will need to scan a segment and when more than the number of spare
 * HOST registers is exceeded, choose to either save register(s) into the emulated registers
 * referenced by the Frame Pointer or push them onto the stack for later use.
 *
 * Pushing onto the stack may make it easier to use LDM/SDM where 32/64 bit is not compatible
 * with the layout of the emulated register space.
 *
 */
void Translate_Registers(code_seg_t* const codeSegment)
{
	Instruction_t* ins;
	//Instruction_t*insSearch;

	uint32_t x;
	uint32_t NumberRegUsed = 0;

	uint16_t counts[REG_T_SIZE];
	memset(counts,0,sizeof(counts));

	ins = codeSegment->Intermcode;
	while (ins)
	{
		if (ins->Rd1 != REG_NOT_USED) counts[ins->Rd1]++;
		if (ins->Rd2 != REG_NOT_USED) counts[ins->Rd2]++;
		if (ins->R1 != REG_NOT_USED) counts[ins->R1]++;
		if (ins->R2 != REG_NOT_USED) counts[ins->R2]++;
		if (ins->R3 != REG_NOT_USED) counts[ins->R3]++;

		ins = ins->nextInstruction;
	}

	for (x=0; x < REG_HOST + 11; x++)
	{
		if (counts[x]) NumberRegUsed++;
	}

	printf("Segment 0x%x uses %d registers\n",(uint32_t)codeSegment, NumberRegUsed);

	if (NumberRegUsed < 10)
	{
		ins = codeSegment->Intermcode;
		uint32_t CRindex = 0;
		while (counts[REG_HOST + CRindex]) CRindex++;

		for (x = 0; x < REG_HOST; x++ )
		{
			if (counts[x])
			{
				UpdateRegWithReg(ins,(reg_t)x, REG_HOST + CRindex, 0);
				CRindex++;
				while (counts[REG_HOST + CRindex]) CRindex++;
			}
		}
	}
	else
	{
		//TODO
		fprintf(stderr, "Missing code to do register translation\n");
	}

	//Strip HOST flag from register ID leaving ARM register ID ready for writing
	ins = codeSegment->Intermcode;
	while (ins)
	{
		if (ins->Rd1 != REG_NOT_USED) ins->Rd1 &= ~REG_HOST;
		if (ins->Rd2 != REG_NOT_USED) ins->Rd2 &= ~REG_HOST;
		if (ins->R1 != REG_NOT_USED) ins->R1 &= ~REG_HOST;
		if (ins->R2 != REG_NOT_USED) ins->R2 &= ~REG_HOST;
		if (ins->R3 != REG_NOT_USED) ins->R3 &= ~REG_HOST;

		ins = ins->nextInstruction;
	}


	// ------------ sanity check --------------

#ifndef NDEBUG
	ins = codeSegment->Intermcode;

	while (ins)
	{
		assert( ins->Rd1 < 16 || ins->Rd1 == REG_NOT_USED);
		assert( ins->Rd2 < 16 || ins->Rd2 == REG_NOT_USED);
		assert( ins->R1 < 16 || ins->R1 == REG_NOT_USED);
		assert( ins->R2 < 16 || ins->R2 == REG_NOT_USED);
		assert( ins->R3 < 16 || ins->R3 == REG_NOT_USED);
		ins = ins->nextInstruction;
	}
#endif
	return;
}

void Translate_LoadStoreWriteBack(code_seg_t* const codeSegment)
{
	Instruction_t*ins;
	ins = codeSegment->Intermcode;

	while (ins)
	{
		ins = ins->nextInstruction;
	}
}

/*
 *	Function to Translate memory operations into Emulated equivalent
 *
 *  Emulated memory access can be cached, non-cached or virtual
 *
 *  As the emulator memory will be mmaped to 0x80000000, non-cached need not do
 *  any translation. cached memory 'just needs' BIC Rd, R1, #1, LSL # 29
 *
 *  Virtual will need to call a function to lookup address
 */
void Translate_Memory(code_seg_t* const codeSegment)
{
	Instruction_t*ins;
	ins = codeSegment->Intermcode;

	while (ins)
	{
		if (ins->nextInstruction->instruction == LW)
		{

		}

		ins = ins->nextInstruction;
	}
}

void Translate_Generic(code_seg_t* const codeSegment)
{
	Instruction_t*ins;
	ins = codeSegment->Intermcode;

	while (ins)
	{
		switch (ins->instruction)
		{
		case MTC0:
		case MFC0:
			ins->instruction = ARM_MOV;
			ins->R2 = ins->R1;
			ins->R1 = REG_NOT_USED;
			break;
		default: break;
		}

		ins = ins->nextInstruction;
	}
}


void Translate_init(code_seg_t* const codeSegment)
{
	int x;
	Instruction_t*newInstruction;
	Instruction_t*prevInstruction = NULL;

	freeIntermediateInstructions(codeSegment);

	//now build new Intermediate code
	for (x=0; x < codeSegment->MIPScodeLen; x++)
	{
		newInstruction = newEmptyInstr();

		mips_decode(*(codeSegment->MIPScode + x), newInstruction);

		if (x == 0)
		{
			codeSegment->Intermcode = newInstruction;

		}
		else
		{
			prevInstruction->nextInstruction = newInstruction;
		}
		prevInstruction = newInstruction;
	}

	Translate_Generic(codeSegment);
	return;
}

void Translate(code_seg_t* const codeSegment)
{
	Translate_init(codeSegment);

	Translate_CountRegister(codeSegment);
	Translate_DelaySlot(codeSegment);
	Translate_Memory(codeSegment);
	Translate_32BitRegisters(codeSegment);
	Translate_Registers(codeSegment);
}
