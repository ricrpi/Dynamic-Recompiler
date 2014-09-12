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

#define ADD_LL(x, y) (x)->nextInstruction = (y)->nextInstruction; \
			(y)->nextInstruction = (x);

uint32_t bCountSaturates = 0;
uint32_t uiCountFrequency = 40;	// must be less than 128 else may not be able to encode in imm8
uint32_t bMemoryInlineLookup = 0;
uint32_t bMemoryOffsetCheck = 0;
uint32_t bDoDMAonInterrupt = 1;
uint8_t uMemoryBase = 0x80;


//=============================================================

static Instruction_t* insertCall(Instruction_t* ins, const Condition_e cond, const int32_t offset)
{
	Instruction_t* newInstruction;

	//push lr
	newInstruction 	= newInstrPUSH(AL, REG_HOST_STM_LR);
	ADD_LL_NEXT(newInstruction, ins);

	//set lr
	newInstruction 	= newInstrI(ADD, AL, REG_HOST_LR, REG_HOST_PC, REG_NOT_USED, 8);
	ADD_LL_NEXT(newInstruction, ins);

	// load function address from [fp + offset] into PC
	newInstruction 	= newInstrI(ARM_LDR, cond, REG_HOST_PC, REG_HOST_FP, REG_NOT_USED, offset);
	ADD_LL_NEXT(newInstruction, ins);

	// pop lr
	newInstruction 	= newInstrPOP(AL, REG_HOST_STM_LR);
	ADD_LL_NEXT(newInstruction, ins);

	return ins;
}

static int32_t FindRegNextUsedAgain(const Instruction_t* const ins, const regID_t Reg)
{
	const Instruction_t* in = ins;
	uint32_t x = 0;

	while (in)
	{
		if (ins->R1.regID == Reg || ins->R2.regID == Reg || ins->R3.regID == Reg)	return x;
		if (ins->Rd1.regID == Reg || ins->Rd2.regID == Reg) return -1;

		x++;
		in = in->nextInstruction;
	}

	return x;
}

static void UpdateRegWithReg(Instruction_t* const ins, const regID_t RegFrom, const regID_t RegTo, uint32_t MaxInstructions)
{
	Instruction_t* in = ins;
	uint32_t x = MaxInstructions;

	if (!x) x = 0xffffffff;

	if (RegFrom >= REG_HOST)
	{
		if (RegTo >= REG_HOST) 	printf("Reg host %3d => host %3d\n", RegFrom-REG_HOST, RegTo-REG_HOST);
		else					printf("Reg host %3d =>      %3d\n", RegFrom-REG_HOST, RegTo);
	}
	else
	{
		if (RegTo >= REG_HOST) 	printf("Reg      %3d => host %3d\n", RegFrom, RegTo-REG_HOST);
		else					printf("Reg      %3d =>      %3d\n", RegFrom, RegTo);
	}


	while (x && in)
	{
		if (in->Rd1.regID == RegFrom && in->Rd1.state == RS_REGISTER) in->Rd1.regID = RegTo;
		if (in->Rd2.regID == RegFrom && in->Rd2.state == RS_REGISTER) in->Rd2.regID = RegTo;
		if (in->R1.regID == RegFrom && in->R1.state == RS_REGISTER) in->R1.regID = RegTo;
		if (in->R2.regID == RegFrom && in->R2.state == RS_REGISTER) in->R2.regID = RegTo;
		if (in->R3.regID == RegFrom && in->R3.state == RS_REGISTER) in->R3.regID = RegTo;
		x--;
		in = in->nextInstruction;
	}
}

//=============================================================



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
code_seg_t* Generate_MemoryTranslationCode(code_segment_data_t* seg_data, pfu1ru1 f)
{
	code_seg_t* 	code_seg 		= newSegment();
	Instruction_t* 	newInstruction;
	Instruction_t* 	ins 			= NULL;

	if (bMemoryInlineLookup){
		printf("Generate_MemoryTranslationCode() and bMemoryInlineLookup\n");
		return NULL;
	}

	//Add the base and offset together
	newInstruction 		= newInstr(ARM_ADD, AL, REG_TEMP_MEM1, REG_HOST_R0, REG_HOST_R1);
	code_seg->Intermcode = ins = newInstruction;

	//shift Right so that we have the final Byte
	newInstruction 		= newInstr(SRL, AL, REG_TEMP_MEM2, REG_TEMP_MEM1, REG_NOT_USED);
	newInstruction->I = 1;
	newInstruction->shift = 24;
	ADD_LL_NEXT(newInstruction, ins);

	//If the final Byte is 0x80 then all is good
	newInstruction 		= newInstrI(ARM_CMP, AL, REG_NOT_USED, REG_TEMP_MEM2, REG_NOT_USED, 0x80);
	newInstruction->I = 1;
	ADD_LL_NEXT(newInstruction, ins);

	//Move the address to R0
	newInstruction 		= newInstr(ARM_MOV, EQ, REG_HOST_R0, REG_NOT_USED, REG_TEMP_MEM1);
	ADD_LL_NEXT(newInstruction, ins);

	// Return
	newInstruction 		= newInstr(ARM_MOV, EQ, REG_HOST_PC, REG_NOT_USED, REG_HOST_LR);
	ADD_LL_NEXT(newInstruction, ins);

	//Else is the final byte 0xA0?
	newInstruction 		= newInstrI(ARM_CMP, AL, REG_NOT_USED, REG_TEMP_MEM2, REG_NOT_USED, 0xA0);
	newInstruction->I = 1;
	ADD_LL_NEXT(newInstruction, ins);

	//If so then clear bit 0x40
	newInstruction 		= newInstrI(ARM_BIC, EQ, REG_HOST_R0, REG_TEMP_MEM1, REG_NOT_USED, 0x40);
	newInstruction->I = 1;
	newInstruction->shift = 24;
	newInstruction->shiftType = LOGICAL_LEFT;
	ADD_LL_NEXT(newInstruction, ins);

	// Return
	newInstruction 		= newInstr(ARM_MOV, EQ, REG_HOST_PC, REG_NOT_USED, REG_HOST_LR);
	ADD_LL_NEXT(newInstruction, ins);

	//The address must be Virtual

	newInstruction 		= newInstrPUSH(AL, REG_HOST_STM_EABI | REG_HOST_STM_LR);
	ADD_LL_NEXT(newInstruction, ins);

	//TODO call C function for Lookup?
	newInstruction 		= newInstr(ARM_MOV, EQ, REG_HOST_LR, REG_NOT_USED, REG_HOST_PC);
	//The literal !!!
	newInstruction 		= newInstr(ARM_LDR_LIT, EQ, REG_HOST_PC, REG_HOST_PC, REG_NOT_USED);

	newInstruction 		= newInstrPOP(AL, REG_HOST_STM_EABI| REG_HOST_STM_LR);
	ADD_LL_NEXT(newInstruction, ins);

	//Return
	newInstruction 		= newInstr(ARM_MOV, AL, REG_HOST_PC, REG_NOT_USED, REG_HOST_LR);
	ADD_LL_NEXT(newInstruction, ins);

	Translate_Registers(code_seg);

	return code_seg;
}

/*
 * Function Called to Begin Running Dynamic compiled code.
 */
code_seg_t* Generate_CodeStart(code_segment_data_t* seg_data)
{
	code_seg_t* 	code_seg 		= newSegment();
	Instruction_t* 	newInstruction;
	Instruction_t* 	ins 			= NULL;

	seg_data->dbgCurrentSegment = code_seg;

	code_seg->Type= SEG_START;

	newInstruction 		= newInstrPUSH(AL, REG_HOST_STM_EABI2 );
	code_seg->Intermcode = ins = newInstruction;

	regID_t base;
	int32_t offset;

	addLiteral(code_seg, &base, &offset,(uint32_t)MMAP_FP_BASE);
	newInstruction 		= newInstrI(ARM_LDR_LIT, EQ, REG_HOST_FP, REG_NOT_USED, base, offset);

	assert(base == REG_HOST_PC);

	// Need to get segment for 0x88000040, lookup the ARM address and jump to it.
	code_seg_t* start_seg = seg_data->StaticBounds[0x40/4];
	uint32_t* arm_address = start_seg->ARMcode;

	//branch to start of ARM code
	newInstruction 		= newInstrI(ARM_LDR_LIT, EQ, REG_HOST_PC, REG_NOT_USED, REG_HOST_FP, FUNC_GEN_START);
	ADD_LL_NEXT(newInstruction, ins);

	Translate_Registers(code_seg);

	return code_seg;
}

code_seg_t* Generate_CodeStop(code_segment_data_t* seg_data)
{
	code_seg_t* 	code_seg 		= newSegment();
	Instruction_t* 	newInstruction;
	Instruction_t* 	ins 			= NULL;

	seg_data->dbgCurrentSegment = code_seg;

	newInstruction 		= newInstrPOP(AL, REG_HOST_STM_EABI2 );
	code_seg->Intermcode = ins = newInstruction;

	// Return
	newInstruction 		= newInstr(ARM_MOV, EQ, REG_HOST_PC, REG_NOT_USED, REG_HOST_LR);
	ADD_LL_NEXT(newInstruction, ins);

	Translate_Registers(code_seg);

	return code_seg;
}

code_seg_t* Generate_ISR(code_segment_data_t* seg_data)
{
	code_seg_t* 	code_seg 		= newSegment();
	Instruction_t* 	newInstruction;
	Instruction_t* 	ins 			= NULL;

	seg_data->dbgCurrentSegment = code_seg;

	newInstruction 		= newInstrPUSH(AL, REG_HOST_STM_EABI2 );
	code_seg->Intermcode = ins = newInstruction;

	// Call interrupt C function

	newInstruction 		= newInstrPOP(AL, REG_HOST_STM_EABI2 );
	ADD_LL_NEXT(newInstruction, ins);

	// Return
	newInstruction 		= newInstr(ARM_MOV, EQ, REG_HOST_PC, REG_NOT_USED, REG_HOST_LR);
	ADD_LL_NEXT(newInstruction, ins);

	Translate_Registers(code_seg);

	return code_seg;
}

//=============================================================

void Translate_init(code_seg_t* const codeSegment)
{
	int x;
	Instruction_t*newInstruction;
	Instruction_t*prevInstruction = NULL;

	freeIntermediateInstructions(codeSegment);

	//now build new Intermediate code
	for (x=0; x < codeSegment->MIPScodeLen; x++)
	{
		// Filter out No-ops
		if (0 != *(codeSegment->MIPScode + x))
		{
			newInstruction = newEmptyInstr();

			mips_decode(*(codeSegment->MIPScode + x), newInstruction);

			if (NULL == prevInstruction)
			{
				codeSegment->Intermcode = newInstruction;

			}
			else
			{
				prevInstruction->nextInstruction = newInstruction;
			}
			prevInstruction = newInstruction;
		}

	}

	return;
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
void Translate_DelaySlot(code_seg_t*  codeSegment)
{
	Instruction_e ops = ops_type(*(codeSegment->MIPScode + codeSegment->MIPScodeLen -1));
	Instruction_t* newInstruction;

	Instruction_t* ins = codeSegment->Intermcode;
	Instruction_t* prev_ins;

	if (ins == NULL)
		{
			printf("Not initialized this code segment. Please run 'optimize intermediate'\n");
			return;
		}

	//if the last instruction is a branch or jump and is not Likely
	if ((ops & (OPS_BRANCH | OPS_JUMP))
			&& !(ops & OPS_LIKELY))
	{
		newInstruction 	= newEmptyInstr();

		mips_decode(*(codeSegment->MIPScode + codeSegment->MIPScodeLen), newInstruction);

		//Is this is a one instruction segment?
		if (NULL == ins->nextInstruction)
		{
			newInstruction->nextInstruction = codeSegment->Intermcode;
			codeSegment->Intermcode = newInstruction;

			// ins will still be pointing at the last instruction
		}
		else
		{
			while (ins->nextInstruction->nextInstruction)
			ins = ins->nextInstruction;

			ADD_LL_NEXT(newInstruction, ins);		//ins will be pointing to newInstruction
			ins = ins->nextInstruction;				// move to the last instruction
		}

		//Set Status Register setting
		/*	APSR.N = imm32<31>;
			APSR.Z = imm32<30>;
			APSR.C = imm32<29>;
			APSR.V = imm32<28>;
			APSR.Q = imm32<27>;
		 */

		if (ops & OPS_LINK)
		{
			newInstruction 	= newInstrI(ARM_MSR,AL, REG_NOT_USED, REG_NOT_USED, REG_NOT_USED, 0x40000000);
			ADD_LL_NEXT(newInstruction, ins);
		}
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

	//	codeSegment->Intermcode->cond = NE;	//Make first instruction conditional
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
		abort();
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
				newInstruction = newInstrIS(ARM_SUB,AL,REG_COUNT,REG_COUNT,REG_NOT_USED,uiCountFrequency);
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
			//TODO think this might be add and on, overflow or positive, we branch
			newInstruction = newInstrIS(ARM_SUB,AL,REG_COUNT,REG_COUNT,REG_NOT_USED,instrCountRemaining);

			newInstruction->nextInstruction = ins->nextInstruction;
			ADD_LL_NEXT(newInstruction, ins);

			ins = insertCall(ins, MI, FUNC_GEN_INTERRUPT);

			return;
		}
	}
}

/*
 *
 */
void Translate_Constants(code_seg_t* const codeSegment)
{
	Instruction_t*ins = codeSegment->Intermcode;

	//First off r0 is ALWAYS 0 so lets do that first
	while (ins)
	{
		if ((ins->Rd1.regID & ~REG_WIDE) == 0)
		{
			ins->Rd1.state = RS_CONSTANT_U8;
			ins->Rd1.u8 = 0;
		}

		if ((ins->Rd2.regID & ~REG_WIDE) == 0)
		{
			ins->Rd2.state = RS_CONSTANT_U8;
			ins->Rd2.u8 = 0;
		}

		if ((ins->R1.regID & ~REG_WIDE) == 0)
		{
			ins->R1.state = RS_CONSTANT_U8;
			ins->R1.u8 = 0;
		}

		if ((ins->R2.regID & ~REG_WIDE) == 0)
		{
			ins->R2.state = RS_CONSTANT_U8;
			ins->R2.u8 = 0;
		}

		if ((ins->R3.regID & ~REG_WIDE) == 0)
		{
			ins->R3.state = RS_CONSTANT_U8;
			ins->R3.u8 = 0;
		}

		ins = ins->nextInstruction;
	}

	ins = codeSegment->Intermcode;

	while (ins)
	{
		switch (ins->instruction)
		{
		case LUI:
			ins->Rd1.state = RS_CONSTANT_I8;
			if (ins->immediate < 0)
			{
				ins->Rd1.i8 = 0xffffffff00000000ll | (ins->immediate << 16);
			}else
			{
			ins->Rd1.i8 = ins->immediate;
			}
			break;
		default: break;
		}
		ins = ins->nextInstruction;
	}
}

/*
 * Function to turn 64bit registers into multiple 32-bit registers
 *
 */
void Translate_32BitRegisters(code_seg_t* const codeSegment)
{
	Instruction_t*ins;
	Instruction_t*new_ins;
	ins = codeSegment->Intermcode;

	while (ins)
	{
		switch (ins->instruction)
		{
			case SLL: break;
			case SRL: break;
			case SRA: break;
			case SLLV: break;
			case SRLV: break;
			case SRAV: break;
			case SYSCALL: break;
			case BREAK: break;
			case SYNC: break;
			case MFHI: break;
			case MTHI: break;
			case MFLO: break;
			case MTLO: break;
			case DSLLV:
				/*
				 *		Rd1 W        Rd1            R1 W           R1               R2 W          R2
				 * [FF FF FF FF | FF FF FF FE] = [FF FF FF FF | FF FF FF FF] << [00 00 00 00 | 00 00 00 3F]
				 *
				 *
				 */

				// 1. Work out lower Word
				new_ins = newInstr(ARM_MOV, AL, ins->Rd1.regID , REG_NOT_USED, ins->R1.regID);
				new_ins->shiftType = LOGICAL_LEFT;
				new_ins->R3.regID = ins->R2.regID;
				ADD_LL_NEXT(new_ins, ins);

				// 2. Work out upper word
				new_ins = newInstr(ARM_MOV, AL, ins->Rd1.regID | REG_WIDE, REG_NOT_USED, ins->R1.regID | REG_WIDE);
				new_ins->shiftType = LOGICAL_LEFT;
				new_ins->R3.regID = ins->R2.regID;
				ADD_LL_NEXT(new_ins, ins);

				// 3. Work out lower shifted to upper
				new_ins = newInstrIS(ARM_RSB, AL, REG_TEMP_GEN2, REG_NOT_USED, ins->R2.regID, 32);
				ADD_LL_NEXT(new_ins, ins);

				new_ins = newInstr(ARM_ORR, PL, ins->Rd1.regID | REG_WIDE, REG_NOT_USED, ins->R1.regID);
				new_ins->shiftType = LOGICAL_RIGHT;
				new_ins->R3.regID = REG_TEMP_GEN2;
				ADD_LL_NEXT(new_ins, ins);

				// 4. Work out R1 << into Rd1 W (i.e. where R2 > 32) If this occurs then Step 1 and 2 didn't do anything
				new_ins = newInstrIS(ARM_SUB, AL, REG_TEMP_GEN1, REG_NOT_USED, ins->R1.regID, 32);
				ADD_LL_NEXT(new_ins, ins);

				new_ins = newInstr(ARM_ORR, PL, ins->Rd1.regID | REG_WIDE, ins->R1.regID, REG_TEMP_GEN1);
				ADD_LL_NEXT(new_ins, ins);

				break;
			case DSRLV:
				/*
				 *
				 * [7F FF FF FF | FF FF FF FF] = [FF FF FF FF | FF FF FF FF] >> [00 00 00 00 | 00 00 00 3F]
				 *
				 *
				 */

				//Work out lower Word
				new_ins = newInstr(ARM_MOV, AL, ins->Rd1.regID, REG_NOT_USED, ins->R1.regID);
				new_ins->shiftType = LOGICAL_RIGHT;
				new_ins->R3.regID = ins->R2.regID;
				ADD_LL_NEXT(new_ins, ins);

				//Work out upper word
				new_ins = newInstr(ARM_MOV, AL, ins->Rd1.regID| REG_WIDE, REG_NOT_USED, ins->Rd1.regID | REG_WIDE);
				new_ins->shiftType = LOGICAL_RIGHT;
				new_ins->R3.regID = ins->R2.regID;
				ADD_LL_NEXT(new_ins, ins);

				//Work out upper shifted to lower
				new_ins = newInstrIS(ARM_SUB, AL, REG_TEMP_GEN1, REG_NOT_USED, ins->R1.regID, 32);
				ADD_LL_NEXT(new_ins, ins);

				new_ins = newInstr(ARM_MOV, PL, REG_TEMP_GEN2, ins->R1.regID, REG_NOT_USED);
				new_ins->shiftType = LOGICAL_RIGHT;
				new_ins->R3.regID = REG_TEMP_GEN1;
				ADD_LL_NEXT(new_ins, ins);

				new_ins = newInstr(ARM_ORR, PL, ins->Rd1.regID| REG_WIDE, ins->Rd1.regID | REG_WIDE, REG_TEMP_GEN1);
				ADD_LL_NEXT(new_ins, ins);
				break;
			case DSRAV: break;
			case MULT: break;
			case MULTU: break;
			case DIV: break;
			case DIVU: break;
			case DMULT: break;
			case DMULTU: break;
			case DDIV: break;
			case DDIVU: break;
			case ADD: break;
			case ADDU: break;
			case SUB: break;
			case SUBU: break;
			case AND: break;
			case OR: break;
			case XOR: break;
			case NOR: break;
			case SLT: break;
			case SLTU: break;
			case DADD: break;
			case DADDU: break;
			case DSUB: break;
			case DSUBU: break;
			case TGE: break;
			case TGEU: break;
			case TLT: break;
			case TLTU: break;
			case TEQ: break;
			case TNE: break;
			case DSLL: break;
			case DSRL: break;
			case DSRA: break;
			case DSLL32: break;
			case DSRL32: break;
			case DSRA32: break;
			case TGEI: break;
			case TGEIU: break;
			case TLTI: break;
			case TLTIU: break;
			case TEQI: break;
			case TNEI: break;
			case ADDI: break;
			case ADDIU: break;
			case SLTI: break;
			case SLTIU: break;
			case ANDI: break;
			case ORI: break;
			case XORI: break;
			case LUI: break;
			case MFC0: break;
			case MTC0: break;
			case TLBR: break;
			case TLBWI: break;
			case TLBWR: break;
			case TLBP: break;
			case ERET: break;
			case MFC1: break;
			case DMFC1: break;
			case CFC1: break;
			case MTC1: break;
			case DMTC1: break;
			case CTC1: break;
			case BC1F: break;
			case BC1T: break;
			case BC1FL: break;
			case BC1TL: break;
			case ADD_S: break;
			case SUB_S: break;
			case MUL_S: break;
			case DIV_S: break;
			case SQRT_S: break;
			case ABS_S: break;
			case MOV_S: break;
			case NEG_S: break;
			case ROUND_L_S: break;
			case TRUNC_L_S: break;
			case CEIL_L_S: break;
			case FLOOR_L_S: break;
			case ROUND_W_S: break;
			case TRUNC_W_S: break;
			case CEIL_W_S: break;
			case FLOOR_W_S: break;
			case CVT_D_S: break;
			case CVT_W_S: break;
			case CVT_L_S: break;
			case C_F_S: break;
			case C_UN_S: break;
			case C_EQ_S: break;
			case C_UEQ_S: break;
			case C_OLT_S: break;
			case C_ULT_S: break;
			case C_OLE_S: break;
			case C_ULE_S: break;
			case C_SF_S: break;
			case C_NGLE_S: break;
			case C_SEQ_S: break;
			case C_NGL_S: break;
			case C_LT_S: break;
			case C_NGE_S: break;
			case C_LE_S: break;
			case C_NGT_S: break;
			case ADD_D: break;
			case SUB_D: break;
			case MUL_D: break;
			case DIV_D: break;
			case SQRT_D: break;
			case ABS_D: break;
			case MOV_D: break;
			case NEG_D: break;
			case ROUND_L_D: break;
			case TRUNC_L_D: break;
			case CEIL_L_D: break;
			case FLOOR_L_D: break;
			case ROUND_W_D: break;
			case TRUNC_W_D: break;
			case CEIL_W_D: break;
			case FLOOR_W_D: break;
			case CVT_S_D: break;
			case CVT_W_D: break;
			case CVT_L_D: break;
			case C_F_D: break;
			case C_UN_D: break;
			case C_EQ_D: break;
			case C_UEQ_D: break;
			case C_OLT_D: break;
			case C_ULT_D: break;
			case C_OLE_D: break;
			case C_ULE_D: break;
			case C_SF_D: break;
			case C_NGLE_D: break;
			case C_SEQ_D: break;
			case C_NGL_D: break;
			case C_LT_D: break;
			case C_NGE_D: break;
			case C_LE_D: break;
			case C_NGT_D: break;
			case CVT_S_W: break;
			case CVT_D_W: break;
			case CVT_S_L: break;
			case CVT_D_L: break;
			case DADDI: break;
			case DADDIU: break;
			case CACHE: break;
			case LL: break;
			case LWC1: break;
			case LLD: break;
			case LDC1: break;
			case LD: break;
			case SC: break;
			case SWC1: break;
			case SCD: break;
			case SDC1: break;
			case SD: break;

			case J: break;
			case JR: break;
			case JAL: break;
			case JALR: break;

			case BLTZ: break;
			case BGEZ: break;
			case BEQ: break;
			case BNE: break;
			case BLEZ: break;
			case BGTZ: break;

			case BLTZL: break;
			case BGEZL: break;
			case BEQL: break;
			case BNEL: break;
			case BLEZL: break;
			case BGTZL: break;

			case BLTZAL: break;
			case BGEZAL: break;
			case BLTZALL: break;
			case BGEZALL: break;

			case SB: break;
			case SH: break;
			case SWL: break;
			case SW: break;
			case SDL: break;
			case SDR: break;
			case SWR: break;

			case LDL: break;
			case LDR: break;
			case LB: break;
			case LH: break;
			case LWL: break;
			case LW: break;
			case LBU: break;
			case LHU: break;
			case LWR: break;
			case LWU: break;

		default: break;
		}

		ins = ins->nextInstruction;
	}
}

/*
 *	Function to Translate memory operations into Emulated equivalent
 *
 *  Emulated memory access can be cached, non-cached or virtual
 *
 *  As the emulator memory will be mmaped to 0x??000000, non-cached need not do
 *  any translation. cached memory 'just needs' BIC Rd, R1, #1, LSL # 29
 *
 *  Virtual will need to call a function to lookup address
 */
void Translate_Memory(code_seg_t* const codeSegment)
{
	Instruction_t*ins;
	ins = codeSegment->Intermcode;

	//TODO we need to check that memory being modified is not MIPS code!

	regID_t	funcTempReg;
	int32_t	funcTempImm;

	while (ins)
	{
		switch (ins->instruction)
		{
		case MFHI: break;
		case MTHI: break;
		case MFLO: break;
		case MTLO: break;

		case MULT: break;
		case MULTU: break;
		case DIV: break;
		case DIVU: break;
		case DMULT: break;
		case DMULTU: break;
		case DDIV: break;
		case DDIVU: break;

		case SLT: break;
		case SLTU: break;

		case LUI: break;
		case MFC0: break;
		case MTC0: break;

		case MFC1: break;
		case DMFC1: break;
		case CFC1: break;
		case MTC1: break;
		case DMTC1: break;
		case CTC1: break;

		case LL: break;
		case LWC1: break;
		case LLD: break;
		case LDC1: break;
		case LD: break;
		case SC: break;
		case SWC1: break;
		case SCD: break;
		case SDC1: break;
		case SD: break;

		case SB: break;
		case SH: break;
		case SWL: break;
		case SW: break;
		case SDL: break;
		case SDR: break;
		case SWR: break;

		case LDL: break;
		case LDR: break;
		case LB: break;
		case LH: break;
		case LWL: break;
		case LW:

			//TODO test for cache/non-cache or virtual
			funcTempReg = ins->Rd1.regID;
			funcTempImm = ins->immediate;

			ins = InstrI(ins, ARM_TST, AL, REG_NOT_USED, ins->R1.regID, REG_NOT_USED, 0x08 << 24);

			Instruction_t* new_ins = newInstrI(ARM_ADD, NE, REG_TEMP_MEM1, ins->R1.regID, REG_NOT_USED, uMemoryBase << 24);
			ADD_LL_NEXT(new_ins, ins);

			if (funcTempImm > 0xFFF || funcTempImm < -0xFFF)
			{
				new_ins = newInstrI(ARM_ORR, NE, REG_TEMP_MEM1, REG_TEMP_MEM1, REG_NOT_USED, funcTempImm&0xf000);
				ADD_LL_NEXT(new_ins, ins);
			}

			new_ins = newInstrI(ARM_LDR_LIT, NE, funcTempReg, REG_NOT_USED, REG_TEMP_MEM1, funcTempImm&0xfff);
			ADD_LL_NEXT(new_ins, ins);

			ins = insertCall(ins, EQ, FUNC_GEN_LOOKUP);

			break;
		case LBU: break;
		case LHU: break;
		case LWR: break;
		case LWU: break;
		default: break;
		}

		ins = ins->nextInstruction;
	}
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

void Translate_LoadCachedRegisters(code_seg_t* const codeSegment)
{
	Instruction_t*ins = codeSegment->Intermcode;
	Instruction_t*prev_ins = NULL;

	Instruction_t*new_ins;
	uint8_t RegUsed[64];

	memset(RegUsed,0,sizeof(RegUsed));

	while (ins)
	{
		if (ins->R1.state == RS_REGISTER
				&& ins->R1.regID < REG_TEMP
				&& !RegUsed[ins->R1.regID])
		{
			RegUsed[ins->R1.regID]++;
			new_ins = newInstrI(ARM_LDR_LIT, AL, ins->R1.regID, REG_NOT_USED, REG_HOST_FP, ins->R1.regID * 4);
			new_ins->nextInstruction = ins;

			if (!prev_ins)
			{
				codeSegment->Intermcode = new_ins;
			}
			else
			{
				prev_ins->nextInstruction = new_ins;
			}
		}

		if (ins->R2.state == RS_REGISTER
				&& ins->R2.regID < REG_TEMP
				&& !RegUsed[ins->R2.regID])
		{
			RegUsed[ins->R2.regID]++;
			new_ins = newInstrI(ARM_LDR_LIT, AL, ins->R2.regID, REG_NOT_USED, REG_HOST_FP, ins->R2.regID * 4);
			new_ins->nextInstruction = ins;

			if (!prev_ins)
			{
				codeSegment->Intermcode = new_ins;
			}
			else
			{
				prev_ins->nextInstruction = new_ins;
			}
		}

		if (ins->R3.state == RS_REGISTER
				&& ins->R3.regID < REG_TEMP
				&& !RegUsed[ins->R3.regID])
		{
			RegUsed[ins->R3.regID]++;
			new_ins = newInstrI(ARM_LDR_LIT, AL, ins->R3.regID, REG_NOT_USED, REG_HOST_FP, ins->R3.regID * 4);
			new_ins->nextInstruction = ins;

			if (!prev_ins)
			{
				codeSegment->Intermcode = new_ins;
			}
			else
			{
				prev_ins->nextInstruction = new_ins;
			}
		}

		prev_ins = ins;
		ins = ins->nextInstruction;
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
		if (ins->Rd1.regID != REG_NOT_USED && ins->Rd1.state == RS_REGISTER) counts[ins->Rd1.regID]++;
		if (ins->Rd2.regID != REG_NOT_USED && ins->Rd2.state == RS_REGISTER) counts[ins->Rd2.regID]++;
		if (ins->R1.regID != REG_NOT_USED && ins->R1.state == RS_REGISTER) counts[ins->R1.regID]++;
		if (ins->R2.regID != REG_NOT_USED && ins->R2.state == RS_REGISTER) counts[ins->R2.regID]++;
		if (ins->R3.regID != REG_NOT_USED && ins->R3.state == RS_REGISTER) counts[ins->R3.regID]++;

		ins = ins->nextInstruction;
	}

	for (x=0; x < REG_HOST + 11; x++)
	{
		if (counts[x]) NumberRegUsed++;
	}

	printf("Segment 0x%x uses %d registers\n",(uint32_t)codeSegment, NumberRegUsed);
	
	if (NumberRegUsed <= 11)
	{
		ins = codeSegment->Intermcode;
		uint32_t uiCurrentRegister = 0;

		while (counts[REG_HOST + uiCurrentRegister])
			uiCurrentRegister++; // Find the next free HOST register

		for (x = 0; x < REG_HOST; x++ )
		{
			if (counts[x])
			{
				UpdateRegWithReg(ins,(regID_t)x, REG_HOST + uiCurrentRegister, 0);
				uiCurrentRegister++;
				while (counts[REG_HOST + uiCurrentRegister]) uiCurrentRegister++; // Find the next free HOST register
			}
		}
	}
	else
	{
		ins = codeSegment->Intermcode;

		//we should do this in the 'instruction' domain so that non-overlapping register usage can be 'flattened'

		uint32_t uiCurrentRegister = 0;
		uint32_t uiLastRegister = 0;

		while (!(FindRegNextUsedAgain(ins, REG_HOST + uiCurrentRegister)> 0))
		{
			uiCurrentRegister++;
			if (uiCurrentRegister > 10) uiCurrentRegister = 0;

			// Have we looped round all registers?
			if (uiLastRegister == uiCurrentRegister){
				abort();
			}
		}

		while (ins)
		{
			/*
			if (ins->instruction == UNKNOWN)
			{
				printf("Unknown Instruction in segment 0x%x\n", codeSegment);

				uint32_t x;
				for (x=0; x < codeSegment->MIPScodeLen; x++)
				{
					mips_print((uint32_t)codeSegment->MIPScode + x*4, *(codeSegment->MIPScode + x));
				}
				abort();
			}*/

			if (ins->Rd1.regID != REG_NOT_USED  && ins->Rd1.state == RS_REGISTER && ins->Rd1.regID < REG_HOST){
				UpdateRegWithReg(ins,ins->Rd1.regID, REG_HOST + uiCurrentRegister, 0);
				uiLastRegister = uiCurrentRegister;
				while (!(FindRegNextUsedAgain(ins, REG_HOST + uiCurrentRegister) > 0))
				{
					uiCurrentRegister++;
					if (uiCurrentRegister > 10) uiCurrentRegister = 0;

					// Have we looped round all registers?
					if (uiLastRegister == uiCurrentRegister){

						abort();
					}
				}
			}

			if (ins->Rd2.regID != REG_NOT_USED && ins->Rd2.state == RS_REGISTER && ins->Rd2.regID < REG_HOST){
				UpdateRegWithReg(ins,ins->Rd2.regID, REG_HOST + uiCurrentRegister, 0);
				uiLastRegister = uiCurrentRegister;
				while (!(FindRegNextUsedAgain(ins, REG_HOST + uiCurrentRegister) > 0))
				{
					uiCurrentRegister++;
					if (uiCurrentRegister > 10) uiCurrentRegister = 0;

					// Have we looped round all registers?
					if (uiLastRegister == uiCurrentRegister){
						abort();
					}
				}
			}

			if (ins->R1.regID != REG_NOT_USED && ins->R1.state == RS_REGISTER && ins->R1.regID < REG_HOST){
				UpdateRegWithReg(ins,ins->R1.regID, REG_HOST + uiCurrentRegister, 0);
				uiLastRegister = uiCurrentRegister;
				while (!(FindRegNextUsedAgain(ins, REG_HOST + uiCurrentRegister) > 0))
				{
					uiLastRegister = uiCurrentRegister;
					uiCurrentRegister++;
					if (uiCurrentRegister > 10) uiCurrentRegister = 0;

					// Have we looped round all registers?
					if (uiLastRegister == uiCurrentRegister){
						abort();
					}
				}
			}

			if (ins->R2.regID != REG_NOT_USED && ins->R2.state == RS_REGISTER && ins->R2.regID < REG_HOST){
				UpdateRegWithReg(ins,ins->R2.regID, REG_HOST + uiCurrentRegister, 0);
				uiLastRegister = uiCurrentRegister;
				while (!(FindRegNextUsedAgain(ins, REG_HOST + uiCurrentRegister) > 0))
				{
					uiCurrentRegister++;
					if (uiCurrentRegister > 10) uiCurrentRegister = 0;

					// Have we looped round all registers?
					if (uiLastRegister == uiCurrentRegister){
						abort();
					}
				}
			}

			if (ins->R3.regID != REG_NOT_USED && ins->R3.state == RS_REGISTER && ins->R3.regID < REG_HOST){
				UpdateRegWithReg(ins,ins->R3.regID, REG_HOST + uiCurrentRegister, 0);
				uiLastRegister = uiCurrentRegister;
				while (!(FindRegNextUsedAgain(ins, REG_HOST + uiCurrentRegister) > 0))
				{
					uiCurrentRegister++;
					if (uiCurrentRegister > 10) uiCurrentRegister = 0;

					// Have we looped round all registers?
					if (uiLastRegister == uiCurrentRegister){
						abort();
					}
				}
			}

			ins = ins->nextInstruction;
		}
	}

#if 0
	//Strip HOST flag from register ID leaving ARM register ID ready for writing
	ins = codeSegment->Intermcode;
	while (ins)
	{
		if (ins->Rd1.regID != REG_NOT_USED && ins->Rd1.state == RS_REGISTER) ins->Rd1.regID &= ~REG_HOST;
		if (ins->Rd2.regID != REG_NOT_USED && ins->Rd2.state == RS_REGISTER) ins->Rd2.regID &= ~REG_HOST;
		if (ins->R1.regID != REG_NOT_USED && ins->R1.state == RS_REGISTER) ins->R1.regID &= ~REG_HOST;
		if (ins->R2.regID != REG_NOT_USED && ins->R2.state == RS_REGISTER) ins->R2.regID &= ~REG_HOST;
		if (ins->R3.regID != REG_NOT_USED && ins->R3.state == RS_REGISTER) ins->R3.regID &= ~REG_HOST;

		ins = ins->nextInstruction;
	}
#endif

	// ------------ sanity check --------------

#ifndef NDEBUG
	ins = codeSegment->Intermcode;

	while (ins)
	{
		assert( ins->Rd1.state != RS_REGISTER || (ins->Rd1.regID & ~REG_HOST) < 16 || ins->Rd1.regID == REG_NOT_USED);
		assert( ins->Rd2.state != RS_REGISTER || (ins->Rd2.regID & ~REG_HOST) < 16 || ins->Rd2.regID == REG_NOT_USED);
		assert( ins->R1.state != RS_REGISTER || (ins->R1.regID & ~REG_HOST) < 16 || ins->R1.regID == REG_NOT_USED);
		assert( ins->R2.state != RS_REGISTER || (ins->R2.regID & ~REG_HOST) < 16 || ins->R2.regID == REG_NOT_USED);
		assert( ins->R3.state != RS_REGISTER || (ins->R3.regID & ~REG_HOST) < 16 || ins->R3.regID == REG_NOT_USED);
		ins = ins->nextInstruction;
	}
#endif
	return;
}

void Translate_StoreCachedRegisters(code_seg_t* const codeSegment)
{
		Instruction_t*ins = codeSegment->Intermcode;

		Instruction_t*new_ins;

		while (ins)
		{
			if (ins->Rd1.state == RS_REGISTER
					&& ins->Rd1.regID < REG_TEMP)
			{
				int32_t nextUsed = FindRegNextUsedAgain(ins, ins->R1.regID);

				//Register will be over-written before next use so don't bother saving
				if (nextUsed == -1)
				{

				}
				else if (nextUsed == 0)
				{
					new_ins = newInstrI(ARM_STR_LIT, AL, REG_NOT_USED, ins->Rd1.regID, REG_HOST_FP, ins->Rd1.regID * 4);
					new_ins->nextInstruction = ins->nextInstruction;
					ins->nextInstruction = new_ins;
				}
			}
			else if (ins->Rd1.regID < REG_TEMP)
			{
				//TODO depending on literal, we could do a ARM_MOV

				regID_t regBase;
				int32_t offset;

				addLiteral(codeSegment,&regBase,&offset,ins->Rd1.u4);

				new_ins = newInstrI(ARM_LDR_LIT, AL, REG_TEMP_STR_CONST, REG_NOT_USED, regBase, offset);
				ADD_LL_NEXT(new_ins, ins);

				new_ins = newInstrI(ARM_STR_LIT, AL, REG_NOT_USED, REG_TEMP_STR_CONST, REG_HOST_FP, ins->Rd1.regID * 4);
				ADD_LL_NEXT(new_ins, ins);
			}

			if (ins->Rd2.state == RS_REGISTER
					&& ins->Rd2.regID < REG_TEMP)
			{
				int32_t nextUsed = FindRegNextUsedAgain(ins, ins->Rd2.regID);

				//Register will be over-written before next use so don't bother saving
				if (nextUsed == -1)
				{

				}
				else if (nextUsed == 0)
				{
					new_ins = newInstrI(ARM_STR_LIT, AL, REG_NOT_USED, ins->Rd2.regID, REG_HOST_FP, ins->Rd2.regID * 4);
					new_ins->nextInstruction = ins->nextInstruction;
					ins->nextInstruction = new_ins;
				}
			}
			else if (ins->Rd2.regID < REG_TEMP)
			{
				abort();
			}

			ins = ins->nextInstruction;
		}
}


#if 0
void Translate_Generic(code_seg_t* const codeSegment)
{
	Instruction_t*ins;
	Instruction_t*new_ins;
	ins = codeSegment->Intermcode;

	while (ins)
	{
		switch (ins->instruction)
		{
			case SLL: break;
			case SRL: break;
			case SRA: break;
			case SLLV: break;
			case SRLV: break;
			case SRAV: break;
			case SYSCALL: break;
			case BREAK: break;
			case SYNC: break;
			case MFHI: break;
			case MTHI: break;
			case MFLO: break;
			case MTLO: break;
			case DSLLV:break;
			case DSRLV:break;
			case DSRAV: break;
			case MULT: break;
			case MULTU: break;
			case DIV: break;
			case DIVU: break;
			case DMULT: break;
			case DMULTU: break;
			case DDIV: break;
			case DDIVU: break;
			case ADD: break;
			case ADDU: break;
			case SUB: break;
			case SUBU: break;
			case AND: break;
			case OR: break;
			case XOR: break;
			case NOR: break;
			case SLT: break;
			case SLTU: break;
			case DADD: break;
			case DADDU: break;
			case DSUB: break;
			case DSUBU: break;
			case TGE: break;
			case TGEU: break;
			case TLT: break;
			case TLTU: break;
			case TEQ: break;
			case TNE: break;
			case DSLL: break;
			case DSRL: break;
			case DSRA: break;
			case DSLL32: break;
			case DSRL32: break;
			case DSRA32: break;
			case TGEI: break;
			case TGEIU: break;
			case TLTI: break;
			case TLTIU: break;
			case TEQI: break;
			case TNEI: break;
			case ADDI: break;
			case ADDIU: break;
			case SLTI: break;
			case SLTIU: break;
			case ANDI: break;
			case ORI: break;
			case XORI: break;
			case LUI: break;
			case MFC0: break;
			case MTC0: break;
			case TLBR: break;
			case TLBWI: break;
			case TLBWR: break;
			case TLBP: break;
			case ERET: break;
			case MFC1: break;
			case DMFC1: break;
			case CFC1: break;
			case MTC1: break;
			case DMTC1: break;
			case CTC1: break;
			case BC1: break;
			case BC1F: break;
			case BC1T: break;
			case BC1FL: break;
			case BC1TL: break;
			case ADD_S: break;
			case SUB_S: break;
			case MUL_S: break;
			case DIV_S: break;
			case SQRT_S: break;
			case ABS_S: break;
			case MOV_S: break;
			case NEG_S: break;
			case ROUND_L_S: break;
			case TRUNC_L_S: break;
			case CEIL_L_S: break;
			case FLOOR_L_S: break;
			case ROUND_W_S: break;
			case TRUNC_W_S: break;
			case CEIL_W_S: break;
			case FLOOR_W_S: break;
			case CVT_D_S: break;
			case CVT_W_S: break;
			case CVT_L_S: break;
			case C_F_S: break;
			case C_UN_S: break;
			case C_EQ_S: break;
			case C_UEQ_S: break;
			case C_OLT_S: break;
			case C_ULT_S: break;
			case C_OLE_S: break;
			case C_ULE_S: break;
			case C_SF_S: break;
			case C_NGLE_S: break;
			case C_SEQ_S: break;
			case C_NGL_S: break;
			case C_LT_S: break;
			case C_NGE_S: break;
			case C_LE_S: break;
			case C_NGT_S: break;
			case ADD_D: break;
			case SUB_D: break;
			case MUL_D: break;
			case DIV_D: break;
			case SQRT_D: break;
			case ABS_D: break;
			case MOV_D: break;
			case NEG_D: break;
			case ROUND_L_D: break;
			case TRUNC_L_D: break;
			case CEIL_L_D: break;
			case FLOOR_L_D: break;
			case ROUND_W_D: break;
			case TRUNC_W_D: break;
			case CEIL_W_D: break;
			case FLOOR_W_D: break;
			case CVT_S_D: break;
			case CVT_W_D: break;
			case CVT_L_D: break;
			case C_F_D: break;
			case C_UN_D: break;
			case C_EQ_D: break;
			case C_UEQ_D: break;
			case C_OLT_D: break;
			case C_ULT_D: break;
			case C_OLE_D: break;
			case C_ULE_D: break;
			case C_SF_D: break;
			case C_NGLE_D: break;
			case C_SEQ_D: break;
			case C_NGL_D: break;
			case C_LT_D: break;
			case C_NGE_D: break;
			case C_LE_D: break;
			case C_NGT_D: break;
			case CVT_S_W: break;
			case CVT_D_W: break;
			case CVT_S_L: break;
			case CVT_D_L: break;
			case DADDI: break;
			case DADDIU: break;
			case CACHE: break;
			case LL: break;
			case LWC1: break;
			case LLD: break;
			case LDC1: break;
			case LD: break;
			case SC: break;
			case SWC1: break;
			case SCD: break;
			case SDC1: break;
			case SD: break;

			case J: break;
			case JR: break;
			case JAL: break;
			case JALR: break;

			case BLTZ: break;
			case BGEZ: break;
			case BEQ: break;
			case BNE: break;
			case BLEZ: break;
			case BGTZ: break;

			case BLTZL: break;
			case BGEZL: break;
			case BEQL: break;
			case BNEL: break;
			case BLEZL: break;
			case BGTZL: break;

			case BLTZAL: break;
			case BGEZAL: break;
			case BLTZALL: break;
			case BGEZALL: break;

			case SB: break;
			case SH: break;
			case SWL: break;
			case SW: break;
			case SDL: break;
			case SDR: break;
			case SWR: break;

			case LDL: break;
			case LDR: break;
			case LB: break;
			case LH: break;
			case LWL: break;
			case LW: break;
			case LBU: break;
			case LHU: break;
			case LWR: break;
			case LWU: break;

		default: break;
		}

		ins = ins->nextInstruction;
	}
}
#endif

void Translate(code_seg_t* const codeSegment)
{
	Translate_init(codeSegment);

	Translate_DelaySlot(codeSegment);
	Translate_CountRegister(codeSegment);

	Translate_Constants(codeSegment);
	Translate_32BitRegisters(codeSegment);
	Translate_Memory(codeSegment);

	Translate_LoadStoreWriteBack(codeSegment);

	Translate_LoadCachedRegisters(codeSegment);

	Translate_StoreCachedRegisters(codeSegment);

	Translate_Registers(codeSegment);
}
