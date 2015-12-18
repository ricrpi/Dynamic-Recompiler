/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *                                                                         *
 *  Dynamic-Recompiler - Turns MIPS code into ARM code                       *
 *  Original source: http://github.com/ricrpi/Dynamic-Recompiler             *
 *  Copyright (C) 2015  Richard Hender                                       *
 *                                                                           *
 *  This program is free software: you can redistribute it and/or modify     *
 *  it under the terms of the GNU General Public License as published by     *
 *  the Free Software Foundation, either version 3 of the License, or        *
 *  (at your option) any later version.                                      *
 *                                                                           *
 *  This program is distributed in the hope that it will be useful,          *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *  GNU General Public License for more details.                             *
 *                                                                           *
 *  You should have received a copy of the GNU General Public License        *
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.    *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "Translate.h"
#include "string.h"
#include "DebugDefines.h"

#define COUNTOF(x)	(sizeof(x)/sizeof(x[0]))

static unsigned char RegsAvailable[] = {
#if 0
	//0,
	1,
	2,
	3,
#endif
#if (REG_EMU_DEBUG1 != REG_HOST_R4 && REG_EMU_FP != REG_HOST_R4 && REG_EMU_CC_FP != REG_HOST_R4)
	4
#endif
#if (REG_EMU_DEBUG1 != REG_HOST_R5 && REG_EMU_FP != REG_HOST_R5 && REG_EMU_CC_FP != REG_HOST_R5)
	, 5
#endif
#if (REG_EMU_DEBUG1 != REG_HOST_R6 && REG_EMU_FP != REG_HOST_R6 && REG_EMU_CC_FP != REG_HOST_R6)
	, 6
#endif
#if (REG_EMU_DEBUG1 != REG_HOST_R7 && REG_EMU_FP != REG_HOST_R7 && REG_EMU_CC_FP != REG_HOST_R7)
	, 7
#endif
#if (REG_EMU_DEBUG1 != REG_HOST_R8 && REG_EMU_FP != REG_HOST_R8 && REG_EMU_CC_FP != REG_HOST_R8)
	, 8
#endif
#if (REG_EMU_DEBUG1 != REG_HOST_R9 && REG_EMU_FP != REG_HOST_R9 && REG_EMU_CC_FP != REG_HOST_R9)
	, 9
#endif
#if (REG_EMU_DEBUG1 != REG_HOST_R10 && REG_EMU_FP != REG_HOST_R10 && REG_CC_FP != REG_HOST_R10)
	, 10
#endif
#if (REG_EMU_DEBUG1 != REG_HOST_R11 && REG_EMU_FP != REG_HOST_R11 && REG_CC_FP != REG_HOST_R11)
	, 11
#endif
#if (REG_EMU_DEBUG1 != REG_HOST_R12 && REG_EMU_FP != REG_HOST_R12 && REG_CC_FP != REG_HOST_R12)
	, 12
#endif
};
//TODO we could add lr register but it needs to be marked as already in use. Only helpful when we need to push/pop registers due to lack of space

static 	uint8_t RegLoaded[REG_TEMP];


uint32_t showRegTranslationMap = 0U;
uint32_t showRegTranslationMapProgress = 0U;

uint32_t RegMemByteOffset(regID_t reg)
{
	uint32_t offset = 0U;	// number of 32bits

	if (reg&REG_WIDE) offset += 1U;

	if (reg < REG_CO)
	{
		if ((reg&REG_FP) != NULL)
		{
			offset += 64U;
		}

		offset += ((reg & ~(REG_WIDE | REG_FP))*2U);
	}
	else
	{
		offset += ((reg & ~(REG_WIDE)));
	}

	return offset * 4U;
}

// Function:	FindRegNextUsedAgain
//
// Description:	Function to step through the instruction chain to find when the provided Register is next used.
//
// Parameters:	const Instruction_t* const ins		The instruction to start searching from
//				const regID_t Reg					The register to search for
//
// Returns:		The number of instruction to execute before the register is used again.
//				If the register is overwritten then -1 is returned. (The register can be reused immediately)
//				If the register is never referenced again then -2 is returned. (The register can be saved to free it)
//
static int32_t FindRegNextUsedAgain(const Instruction_t* const ins, const regID_t Reg)
{
	const Instruction_t* in = ins->nextInstruction;
	uint32_t x = 0;

	while (in
			&& in->instruction != DR_INT_BRANCH)
	{
		if (in->R1.regID == Reg || in->R2.regID == Reg || in->R3.regID == Reg || in->R4.regID == Reg)
			return x;

		// make sure that x > 0 in case the first instruction in the chain is updating it
		// must also check the condition as the register may not have changed
		if (x > 0U
				&& ((in->Rd1.regID == Reg && in->cond == AL) || (in->Rd2.regID == Reg && in->cond == AL)))
			return -1;
		x++;
		in = in->nextInstruction;
	}

	return -2;
}

static int32_t FindRegNextUpdated(const Instruction_t* const ins, const regID_t Reg)
{
	const Instruction_t* in = ins->nextInstruction;

	while (in)
	{
		if (in->cond == AL && (in->Rd1.regID == Reg || in->Rd2.regID == Reg))
			return -1;

		in = in->nextInstruction;
	}

	return -2;
}

static void UpdateRegWithReg(Instruction_t* const ins, const regID_t RegFrom, const regID_t RegTo, uint32_t MaxInstructions)
{
	Instruction_t* in = ins;
	uint32_t x = MaxInstructions;

	if (!x) x = 0xffffffff;

#if SHOW_REG_TRANSLATION_MAP == 2
	{
#elif SHOW_REG_TRANSLATION_MAP == 1
	if (showRegTranslationMap)
	{
#else
	if (0)
	{
#endif
		if (RegFrom >= REG_HOST)
		{
			if (RegTo >= REG_HOST) 	printf("Reg host %3d => host %3d\n", RegFrom-REG_HOST, RegTo-REG_HOST);
			else					printf("Reg host %3d =>      %3d\n", RegFrom-REG_HOST, RegTo);
		}
		else if (RegFrom >= REG_TEMP)
		{
			if (RegTo >= REG_HOST) 	printf("Reg temp %3d => host %3d\n", RegFrom-REG_TEMP, RegTo-REG_HOST);
			else					printf("Reg temp %3d =>      %3d\n", RegFrom-REG_TEMP, RegTo);
		}
		else if (RegFrom >= REG_CO)
		{
			if (RegTo >= REG_HOST) 	printf("Reg   co %3d => host %3d\n", RegFrom-REG_CO, RegTo-REG_HOST);
			else					printf("Reg   co %3d =>      %3d\n", RegFrom-REG_CO, RegTo);
		}
		else if (RegFrom >= REG_WIDE)
		{
			if (RegTo >= REG_HOST) 	printf("Reg wide %3d => host %3d\n", RegFrom-REG_WIDE, RegTo-REG_HOST);
			else					printf("Reg wide %3d =>      %3d\n", RegFrom-REG_WIDE, RegTo);
		}
		else
		{
			if (RegTo >= REG_HOST) 	printf("Reg      %3d => host %3d\n", RegFrom, RegTo-REG_HOST);
			else					printf("Reg      %3d =>      %3d\n", RegFrom, RegTo);
		}
	}

	while (x && in)
	{
		if (in->Rd1.regID == RegFrom) in->Rd1.regID = RegTo;
		if (in->Rd2.regID == RegFrom) in->Rd2.regID = RegTo;
		if (in->R1.regID == RegFrom) in->R1.regID = RegTo;
		if (in->R2.regID == RegFrom) in->R2.regID = RegTo;
		if (in->R3.regID == RegFrom) in->R3.regID = RegTo;
		if (in->R4.regID == RegFrom) in->R4.regID = RegTo;

		x--;
		in = in->nextInstruction;
	}

#if SHOW_REG_TRANSLATION_MAP_PROGRESS == 2
	{
#elif SHOW_REG_TRANSLATION_MAP_PROGRESS == 1
	if (showRegTranslationMapProgress)
	{
#else
	if (0)
	{
#endif
		printf_Intermediate(ins,1);
		in = ins->nextInstruction;
		while(in)
		{
			printf_Intermediate(in,0);
			in = in->nextInstruction;
		}

		printf("===========================================================\n");
	}
}

void Translate_LoadCachedRegisters(code_seg_t* const codeSegment)
{
	Instruction_t*ins = codeSegment->Intermcode;

#if USE_INSTRUCTION_COMMENTS
	currentTranslation = "LoadCachedRegisters";
#endif

	Instruction_t* copied_ins;

	memset(RegLoaded,0,sizeof(RegLoaded));

	while (ins)
	{
		regID_t R1 = ins->R1.regID;
		regID_t R2 = ins->R2.regID;
		regID_t R3 = ins->R3.regID;
		regID_t R4 = ins->R4.regID;

		if (R1 < REG_TEMP && !RegLoaded[R1])
		{
			copied_ins = newInstrCopy(ins);

			ins = InstrI(ins, ARM_LDR, AL, R1, REG_NOT_USED, REG_EMU_FP, RegMemByteOffset(R1));

#if USE_INSTRUCTION_COMMENTS
			if (currentTranslation)
			{
				strcpy(ins->comment, currentTranslation);
			}
#endif

#if USE_INSTRUCTION_INIT_REGS
			ins->Rd1_init.regID = R1;
#endif
			ins->U = 1U;
			ins->nextInstruction = copied_ins;

			ins = copied_ins;

			RegLoaded[R1] = 1;
		}

		if (R2 < REG_TEMP && !RegLoaded[R2])
		{
			copied_ins = newInstrCopy(ins);

			ins = InstrI(ins, ARM_LDR, AL, R2, REG_NOT_USED, REG_EMU_FP, RegMemByteOffset(R2));
#if USE_INSTRUCTION_COMMENTS
			if (currentTranslation)
			{
				strcpy(ins->comment, currentTranslation);
			}
#endif
#if USE_INSTRUCTION_INIT_REGS
			ins->Rd1_init.regID = R2;
#endif
			ins->U = 1U;
			ins->nextInstruction = copied_ins;
			ins = copied_ins;

			RegLoaded[R2] = 1;
		}

		if (R3 < REG_TEMP && !RegLoaded[R3])
		{
			copied_ins = newInstrCopy(ins);

			ins = InstrI(ins, ARM_LDR, AL, R3, REG_NOT_USED, REG_EMU_FP, RegMemByteOffset(R3));
#if USE_INSTRUCTION_COMMENTS
			if (currentTranslation)
			{
				strcpy(ins->comment, currentTranslation);
			}
#endif

#if USE_INSTRUCTION_INIT_REGS
			ins->Rd1_init.regID = R3;
#endif
			ins->U = 1U;
			ins->nextInstruction = copied_ins;
			ins = copied_ins;

			RegLoaded[R3] = 1;
		}

		if (R4 < REG_TEMP && !RegLoaded[R4])
		{
			copied_ins = newInstrCopy(ins);

			ins = InstrI(ins, ARM_LDR, AL, R4, REG_NOT_USED, REG_EMU_FP, RegMemByteOffset(R4));
#if USE_INSTRUCTION_COMMENTS
			if (currentTranslation)
			{
				strcpy(ins->comment, currentTranslation);
			}
#endif
#if USE_INSTRUCTION_INIT_REGS
			ins->Rd1_init.regID = R4;
#endif
			ins->U = 1U;
			ins->nextInstruction = copied_ins;
			ins = copied_ins;

			RegLoaded[R4] = 1;
		}

		if (ins->Rd1.regID < REG_TEMP && !RegLoaded[ins->Rd1.regID])
		{
			RegLoaded[ins->Rd1.regID] = 1;
		}

		if (ins->Rd2.regID < REG_TEMP && !RegLoaded[ins->Rd2.regID])
		{
			RegLoaded[ins->Rd2.regID] = 1;
		}

		ins = ins->nextInstruction;
	}
}

static uint32_t pushpopRegister(Instruction_t* ins)
{
	// we need to find a register to push onto stack (or we could save it) so its available for use.
	// When it is next used again then it will need to be loaded back pBestRegNextUsedIns

	// Find best register to push

	uint32_t x;
	uint32_t bestReg = 0U;
	uint32_t bestRegCount = 0U;
	uint32_t regNextUsed[COUNTOF(RegsAvailable)];

	for (x = 0U; x < COUNTOF(RegsAvailable); x++)
	{
		regNextUsed[x] = FindRegNextUsedAgain(ins, RegsAvailable[x]);

		if (regNextUsed[x] > bestRegCount)
		{
			bestRegCount = regNextUsed[x];
			bestReg = x;
		}
	}

	Instruction_t* pBestRegNextUsedIns = ins;
	Instruction_t* new_ins;

	for (x = 0U; x < bestRegCount; x++)
	{
		pBestRegNextUsedIns = pBestRegNextUsedIns->nextInstruction;
	}

	new_ins = newInstrPUSH(AL, 1 << bestReg);
	ADD_LL_NEXT(new_ins, ins);

	new_ins = newInstrPOP(AL, 1 << bestReg);
	ADD_LL_NEXT(new_ins, pBestRegNextUsedIns);

	printf("Pushing register h%d to REG_EMU_FP space. Will be loaded back pBestRegNextUsedIns %d instructions\n", bestReg, bestRegCount);

	abort(); // Only because this has not bee tested yet

	return bestReg;
}

// Function:	getNextRegister
//
// Description:	Finds the next available host register that can be used for translation.
//
// Parametrs:	Instruction_t* ins
//				uint32_t* uiCurrentRegisterIndex
//
static void getNextRegister(Instruction_t* ins, uint32_t* uiCurrentRegisterIndex)
{
	const uint32_t uiLastRegisterIndex = *uiCurrentRegisterIndex;
	int a;

	while ((a = FindRegNextUsedAgain(ins, REG_HOST + RegsAvailable[*uiCurrentRegisterIndex]) >= 0))
	{
		(*uiCurrentRegisterIndex)++;
		if (*uiCurrentRegisterIndex >= COUNTOF(RegsAvailable)) *uiCurrentRegisterIndex = 0U;

#if SHOW_REG_TRANSLATION_MAP == 2
		{
#elif SHOW_REG_TRANSLATION_MAP == 1
		if (showRegTranslationMap)
		{
#else
		if (0)
		{
#endif
			printf("getNextRegister() index %d, Register h%d \n", *uiCurrentRegisterIndex, RegsAvailable[*uiCurrentRegisterIndex]);
		}
		// Have we looped round all registers?
		if (uiLastRegisterIndex == *uiCurrentRegisterIndex){
			*uiCurrentRegisterIndex = pushpopRegister(ins);
			return;
		}
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

#if USE_INSTRUCTION_COMMENTS
	currentTranslation = "Registers";
#endif

	uint32_t x;

#if SHOW_REG_TRANSLATION_MAP == 2
	{
#elif SHOW_REG_TRANSLATION_MAP == 1
	if (showRegTranslationMap)
	{
#else
	if (0)
	{
#endif
		uint32_t NumberRegUsed = 0U;

		uint16_t counts[REG_T_SIZE];
		memset(counts, 0, sizeof(counts));

		// Carry out a count of all the registers used for Debug purposes only
		// TODO RWD
		ins = codeSegment->Intermcode;
		while (ins)
		{
			if (ins->Rd1.regID != REG_NOT_USED) counts[ins->Rd1.regID]++;
			if (ins->Rd2.regID != REG_NOT_USED) counts[ins->Rd2.regID]++;
			if (ins->R1.regID != REG_NOT_USED) counts[ins->R1.regID]++;
			if (ins->R2.regID != REG_NOT_USED) counts[ins->R2.regID]++;
			if (ins->R3.regID != REG_NOT_USED) counts[ins->R3.regID]++;
			if (ins->R4.regID != REG_NOT_USED) counts[ins->R4.regID]++;
			ins = ins->nextInstruction;
		}

		for (x=0U; x < REG_HOST + COUNTOF(RegsAvailable); x++)
		{
			if (counts[x]) NumberRegUsed++;
		}

		printf("Segment 0x%x uses %d registers\n",(uint32_t)codeSegment, NumberRegUsed);
	}

	ins = codeSegment->Intermcode;

	//we should do this in the 'instruction' domain so that non-overlapping register usage can be 'flattened'

	uint32_t uiCurrentRegisterIndex = 0U;

	getNextRegister(ins, &uiCurrentRegisterIndex);

	while (ins)
	{
		if (ins->Rd1.regID != REG_NOT_USED  && ins->Rd1.regID < REG_HOST){
			if (ins->Rd1.regID < REG_TEMP || ins->Rd1.regID > REG_TEMP_SCRATCH3)
			{
				UpdateRegWithReg(ins,ins->Rd1.regID, REG_HOST + RegsAvailable[uiCurrentRegisterIndex], 0U);
				getNextRegister(ins, &uiCurrentRegisterIndex);
			}
			else
			{
				UpdateRegWithReg(ins,ins->Rd1.regID, REG_HOST + ins->Rd1.regID - REG_TEMP, 0U);
			}
		}

		if (ins->Rd2.regID != REG_NOT_USED && ins->Rd2.regID < REG_HOST){
			if (ins->Rd2.regID < REG_TEMP || ins->Rd2.regID > REG_TEMP_SCRATCH3)
			{
				UpdateRegWithReg(ins,ins->Rd2.regID, REG_HOST + RegsAvailable[uiCurrentRegisterIndex], 0U);
				getNextRegister(ins, &uiCurrentRegisterIndex);
			}
			else
			{
				UpdateRegWithReg(ins,ins->Rd2.regID, REG_HOST + ins->Rd2.regID - REG_TEMP, 0);
			}
		}

		if (ins->R1.regID != REG_NOT_USED && ins->R1.regID < REG_HOST){
			if (ins->R1.regID < REG_TEMP || ins->R1.regID > REG_TEMP_SCRATCH3)
			{
				UpdateRegWithReg(ins,ins->R1.regID, REG_HOST + RegsAvailable[uiCurrentRegisterIndex], 0);
				getNextRegister(ins, &uiCurrentRegisterIndex);
			}
			else
			{
				UpdateRegWithReg(ins,ins->R1.regID, REG_HOST + ins->R1.regID - REG_TEMP, 0);
			}
		}

		if (ins->R2.regID != REG_NOT_USED && ins->R2.regID < REG_HOST){
			if (ins->R2.regID < REG_TEMP || ins->R2.regID > REG_TEMP_SCRATCH3)
			{
				UpdateRegWithReg(ins,ins->R2.regID, REG_HOST + RegsAvailable[uiCurrentRegisterIndex], 0);
				getNextRegister(ins, &uiCurrentRegisterIndex);
			}
			else
			{
				UpdateRegWithReg(ins,ins->R2.regID, REG_HOST + ins->R2.regID - REG_TEMP, 0);
			}
		}

		if (ins->R3.regID != REG_NOT_USED && ins->R3.regID < REG_HOST){
			if (ins->R3.regID < REG_TEMP || ins->R3.regID > REG_TEMP_SCRATCH3)
			{
				UpdateRegWithReg(ins,ins->R3.regID, REG_HOST + RegsAvailable[uiCurrentRegisterIndex], 0);
				getNextRegister(ins, &uiCurrentRegisterIndex);
			}
			else
			{
				UpdateRegWithReg(ins,ins->R3.regID, REG_HOST + ins->R3.regID - REG_TEMP, 0);
			}
		}

		if (ins->R4.regID != REG_NOT_USED && ins->R4.regID < REG_HOST){
			if (ins->R4.regID < REG_TEMP || ins->R4.regID > REG_TEMP_SCRATCH3)
			{
				UpdateRegWithReg(ins,ins->R4.regID, REG_HOST + RegsAvailable[uiCurrentRegisterIndex], 0);
				getNextRegister(ins, &uiCurrentRegisterIndex);
			}
			else
			{
				UpdateRegWithReg(ins,ins->R4.regID, REG_HOST + ins->R4.regID - REG_TEMP, 0);
			}
		}

		ins = ins->nextInstruction;
	}

#if DO_HOSTREG_RENUMBER_IN_TRANSLATIONS
	//Strip HOST flag from register ID leaving ARM register ID ready for writing
	ins = codeSegment->Intermcode;
	while (ins)
	{
		if (ins->Rd1.regID != REG_NOT_USED) ins->Rd1.regID &= ~REG_HOST;
		if (ins->Rd2.regID != REG_NOT_USED) ins->Rd2.regID &= ~REG_HOST;
		if (ins->R1.regID != REG_NOT_USED) ins->R1.regID &= ~REG_HOST;
		if (ins->R2.regID != REG_NOT_USED) ins->R2.regID &= ~REG_HOST;
		if (ins->R3.regID != REG_NOT_USED) ins->R3.regID &= ~REG_HOST;
		if (ins->R4.regID != REG_NOT_USED) ins->R4.regID &= ~REG_HOST;

		ins = ins->nextInstruction;
	}
#endif

	// ------------ sanity check --------------

#if !defined (NDEBUG)
	ins = codeSegment->Intermcode;

	while (ins)
	{
		assert( ins->Rd1.state != RS_REGISTER || (ins->Rd1.regID & ~REG_HOST) < 16U || ins->Rd1.regID == REG_NOT_USED);
		assert( ins->Rd2.state != RS_REGISTER || (ins->Rd2.regID & ~REG_HOST) < 16U || ins->Rd2.regID == REG_NOT_USED);

		assert( ins->R1.state != RS_REGISTER || (ins->R1.regID & ~REG_HOST) < 16U || ins->R1.regID == REG_NOT_USED);
		assert( ins->R2.state != RS_REGISTER || (ins->R2.regID & ~REG_HOST) < 16U || ins->R2.regID == REG_NOT_USED);
		assert( ins->R3.state != RS_REGISTER || (ins->R3.regID & ~REG_HOST) < 16U || ins->R3.regID == REG_NOT_USED);
		assert( ins->R4.state != RS_REGISTER || (ins->R4.regID & ~REG_HOST) < 16U || ins->R4.regID == REG_NOT_USED);

		ins = ins->nextInstruction;
	}
#endif
	return;
}

void Translate_StoreCachedRegisters(code_seg_t* const codeSegment)
{
		Instruction_t*ins = codeSegment->Intermcode;

		Instruction_t*new_ins;

#if USE_INSTRUCTION_COMMENTS
	currentTranslation = "StoreCachedRegisters";
#endif
		while (ins)
		{
			regID_t R1 = ins->R1.regID;
			regID_t R2 = ins->R2.regID;
			regID_t R3 = ins->R3.regID;
			regID_t R4 = ins->R4.regID;

			regID_t Rd1 = ins->Rd1.regID;
			regID_t Rd2 = ins->Rd2.regID;
			int32_t off = ins->offset;

			if (Rd1 < REG_TEMP
					&& !(ins->instruction == ARM_LDR && R2 == REG_EMU_FP && off >= 0))	// to account for LoadCachedRegisters()
			{
				int32_t nextUsed = FindRegNextUpdated(ins, Rd1);

				if (nextUsed == -2)
				{
					new_ins = newInstrI(ARM_STR, AL, REG_NOT_USED, Rd1, REG_EMU_FP, RegMemByteOffset(Rd1));
					ADD_LL_NEXT(new_ins, ins);
					RegLoaded[Rd1] = 0;
				}
				if (nextUsed == -1) // Register will be over-written before next use so don't bother saving
				{

				}
			}

			if (Rd2 < REG_TEMP
					&& !(ins->instruction == ARM_LDR && R2 == REG_EMU_FP && off >= 0))
			{
				int32_t nextUsed = FindRegNextUsedAgain(ins, Rd2);

				if (nextUsed == -2)
				{
					new_ins = newInstrI(ARM_STR, AL, REG_NOT_USED, Rd2, REG_EMU_FP, RegMemByteOffset(Rd2));
					ADD_LL_NEXT(new_ins, ins);

					RegLoaded[Rd2] = 0;
				}
				if (nextUsed == -1) // Register will be over-written before next use so don't bother saving
				{

				}
			}

			ins = ins->nextInstruction;
		}
}
