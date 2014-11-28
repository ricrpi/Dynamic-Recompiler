/*
 * MipsRegisters.c
 *
 *  Created on: 23 Oct 2014
 *      Author: rjhender
 */

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
#if (REG_EMU_DEBUG1 != REG_HOST_R4 && REG_EMU_FLAGS != REG_HOST_R4 && REG_EMU_FP != REG_HOST_R4 && REG_EMU_CC_FP != REG_HOST_R4)
	4
#endif
#if (REG_EMU_DEBUG1 != REG_HOST_R5 && REG_EMU_FLAGS != REG_HOST_R5 && REG_EMU_FP != REG_HOST_R5 && REG_EMU_CC_FP != REG_HOST_R5)
	, 5
#endif
#if (REG_EMU_DEBUG1 != REG_HOST_R6 && REG_EMU_FLAGS != REG_HOST_R6 && REG_EMU_FP != REG_HOST_R6 && REG_EMU_CC_FP != REG_HOST_R6)
	, 6
#endif
#if (REG_EMU_DEBUG1 != REG_HOST_R7 && REG_EMU_FLAGS != REG_HOST_R7 && REG_EMU_FP != REG_HOST_R7 && REG_EMU_CC_FP != REG_HOST_R7)
	, 7
#endif
#if (REG_EMU_DEBUG1 != REG_HOST_R8 && REG_EMU_FLAGS != REG_HOST_R8 && REG_EMU_FP != REG_HOST_R8 && REG_EMU_CC_FP != REG_HOST_R8)
	, 8
#endif
#if (REG_EMU_DEBUG1 != REG_HOST_R9 && REG_EMU_FLAGS != REG_HOST_R9 && REG_EMU_FP != REG_HOST_R9 && REG_EMU_CC_FP != REG_HOST_R9)
	, 9
#endif
#if (REG_EMU_DEBUG1 != REG_HOST_R10 && REG_EMU_FLAGS != REG_HOST_R10 && REG_EMU_FP != REG_HOST_R10 && REG_CC_FP != REG_HOST_R10)
	, 10
#endif
#if (REG_EMU_DEBUG1 != REG_HOST_R11 && REG_EMU_FLAGS != REG_HOST_R11 && REG_EMU_FP != REG_HOST_R11 && REG_CC_FP != REG_HOST_R11)
	, 11
#endif
#if (REG_EMU_DEBUG1 != REG_HOST_R12 && REG_EMU_FLAGS != REG_HOST_R12 && REG_EMU_FP != REG_HOST_R12 && REG_CC_FP != REG_HOST_R12)
	, 12
#endif
};
//TODO we could add lr register but it needs to be marked as already in use. Only helpful when we need to push/pop registers due to lack of space

static 	uint8_t RegLoaded[REG_TEMP];


uint32_t showRegTranslationMap = 0;
uint32_t showRegTranslationMapProgress = 0;

uint32_t RegMemByteOffset(regID_t reg)
{
	if (reg < REG_CO)
	{
		uint32_t offset = 0;	// number of 32bits

		if (reg&REG_WIDE) offset += 1;
		if (reg&REG_FP) offset += 32*2;

		offset += ((reg & ~(REG_WIDE|REG_FP)) * 2);

		return offset*4;
	}
	else
		return reg*4;
}

static int32_t FindRegNextUsedAgain(const Instruction_t* const ins, const regID_t Reg)
{
	const Instruction_t* in = ins->nextInstruction;
	uint32_t x = 0;

	while (in)
	{
		if (in->R1.regID == Reg || in->R2.regID == Reg || in->R3.regID == Reg)
			return x;
		if (x && (in->Rd1.regID == Reg || in->Rd2.regID == Reg))					//might overwrite itself
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
		Instr_print(ins,1);
		in = ins->nextInstruction;
		while(in->nextInstruction)
		{
			Instr_print(in,0);
			in = in->nextInstruction;
		}

		printf("===========================================================\n");
	}
}

void Translate_LoadCachedRegisters(code_seg_t* const codeSegment)
{
	Instruction_t*ins = codeSegment->Intermcode;

#if defined(USE_INSTRUCTION_COMMENTS)
	currentTranslation = "LoadCachedRegisters";
#endif

	Instruction_t* copied_ins;

	memset(RegLoaded,0,sizeof(RegLoaded));

	while (ins)
	{
		regID_t R1 = ins->R1.regID;
		regID_t R2 = ins->R2.regID;
		regID_t R3 = ins->R3.regID;

		if (R1 < REG_TEMP && !RegLoaded[R1])
		{
			copied_ins = newInstrCopy(ins);

			ins = InstrI(ins, ARM_LDR, AL, R1, REG_NOT_USED, REG_EMU_FP, RegMemByteOffset(R1));
#if defined (USE_INSTRUCTION_INIT_REGS)
			ins->Rd1_init.regID = R1;
#endif
			ins->nextInstruction = copied_ins;

			ins = copied_ins;

			RegLoaded[R1] = 1;
		}

		if (R2 < REG_TEMP && !RegLoaded[R2])
		{
			copied_ins = newInstrCopy(ins);

			ins = InstrI(ins, ARM_LDR, AL, R2, REG_NOT_USED, REG_EMU_FP, RegMemByteOffset(R2));
#if defined (USE_INSTRUCTION_INIT_REGS)
			ins->Rd1_init.regID = R2;
#endif
			ins->nextInstruction = copied_ins;
			ins = copied_ins;

			RegLoaded[R2] = 1;
		}

		if (R3 < REG_TEMP && !RegLoaded[R3])
		{
			copied_ins = newInstrCopy(ins);

			ins = InstrI(ins, ARM_LDR, AL, R3, REG_NOT_USED, REG_EMU_FP, RegMemByteOffset(R3));
#if defined (USE_INSTRUCTION_INIT_REGS)
			ins->Rd1_init.regID = R3;
#endif
			ins->nextInstruction = copied_ins;
			ins = copied_ins;

			RegLoaded[R3] = 1;
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

	int x;
	int bestReg;
	int bestRegCount = 0;
	int regNextUsed[COUNTOF(RegsAvailable)];

	for (x = 0; x < COUNTOF(RegsAvailable); x++)
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

	for (x = 0; x < bestRegCount; x++) pBestRegNextUsedIns = pBestRegNextUsedIns->nextInstruction;

	new_ins = newInstrPUSH(AL, 1 << bestReg);
	ADD_LL_NEXT(new_ins, ins);

	new_ins = newInstrPOP(AL, 1 << bestReg);
	ADD_LL_NEXT(new_ins, pBestRegNextUsedIns);

	printf("Pushing register h%d to REG_EMU_FP space. Will be loaded back pBestRegNextUsedIns %d instructions\n", bestReg, bestRegCount);

	abort(); // Only because this has not bee tested yet

	return bestReg;
}

static void getNextRegister(Instruction_t* ins, uint32_t* uiCurrentRegisterIndex)
{
	uint32_t uiLastRegisterIndex = *uiCurrentRegisterIndex;
	int a;

	while ((a = FindRegNextUsedAgain(ins, REG_HOST + RegsAvailable[*uiCurrentRegisterIndex]) >= 0))
	{
		(*uiCurrentRegisterIndex)++;
		if (*uiCurrentRegisterIndex >= COUNTOF(RegsAvailable)) *uiCurrentRegisterIndex = 0;

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
	//Instruction_t*insSearch;

#if defined(USE_INSTRUCTION_COMMENTS)
	currentTranslation = "Registers";
#endif

	uint32_t x;
	uint32_t NumberRegUsed = 0;

	uint16_t counts[REG_T_SIZE];
	memset(counts,0,sizeof(counts));

	ins = codeSegment->Intermcode;
	while (ins)
	{
		if (ins->Rd1.regID != REG_NOT_USED) counts[ins->Rd1.regID]++;
		if (ins->Rd2.regID != REG_NOT_USED) counts[ins->Rd2.regID]++;
		if (ins->R1.regID != REG_NOT_USED) counts[ins->R1.regID]++;
		if (ins->R2.regID != REG_NOT_USED) counts[ins->R2.regID]++;
		if (ins->R3.regID != REG_NOT_USED) counts[ins->R3.regID]++;

		ins = ins->nextInstruction;
	}

	for (x=0; x < REG_HOST + COUNTOF(RegsAvailable); x++)
	{
		if (counts[x]) NumberRegUsed++;
	}

#if SHOW_REG_TRANSLATION_MAP == 2
	{
#elif SHOW_REG_TRANSLATION_MAP == 1
	if (showRegTranslationMap)
	{
#else
	if (0)
	{
#endif
		printf("Segment 0x%x uses %d registers\n",(uint32_t)codeSegment, NumberRegUsed);
	}

	ins = codeSegment->Intermcode;

	//we should do this in the 'instruction' domain so that non-overlapping register usage can be 'flattened'

	uint32_t uiCurrentRegisterIndex = 0;

	getNextRegister(ins, &uiCurrentRegisterIndex);

	while (ins)
	{
		if (ins->Rd1.regID != REG_NOT_USED  && ins->Rd1.regID < REG_HOST){
			if (ins->Rd1.regID < REG_TEMP || ins->Rd1.regID > REG_TEMP_SCRATCH3)
			{
				UpdateRegWithReg(ins,ins->Rd1.regID, REG_HOST + RegsAvailable[uiCurrentRegisterIndex], 0);
				getNextRegister(ins, &uiCurrentRegisterIndex);
			}
			else
			{
				UpdateRegWithReg(ins,ins->Rd1.regID, REG_HOST + ins->Rd1.regID - REG_TEMP, 0);
			}
		}

		if (ins->Rd2.regID != REG_NOT_USED && ins->Rd2.regID < REG_HOST){
			if (ins->Rd2.regID < REG_TEMP || ins->Rd2.regID > REG_TEMP_SCRATCH3)
			{
				UpdateRegWithReg(ins,ins->Rd2.regID, REG_HOST + RegsAvailable[uiCurrentRegisterIndex], 0);
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

		ins = ins->nextInstruction;
	}

#if defined(DO_HOSTREG_RENUMBER_IN_TRANSLATIONS)
	//Strip HOST flag from register ID leaving ARM register ID ready for writing
	ins = codeSegment->Intermcode;
	while (ins)
	{
		if (ins->Rd1.regID != REG_NOT_USED) ins->Rd1.regID &= ~REG_HOST;
		if (ins->Rd2.regID != REG_NOT_USED) ins->Rd2.regID &= ~REG_HOST;
		if (ins->R1.regID != REG_NOT_USED) ins->R1.regID &= ~REG_HOST;
		if (ins->R2.regID != REG_NOT_USED) ins->R2.regID &= ~REG_HOST;
		if (ins->R3.regID != REG_NOT_USED) ins->R3.regID &= ~REG_HOST;

		ins = ins->nextInstruction;
	}
#endif

	// ------------ sanity check --------------

#if !defined (NDEBUG)
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

#if defined(USE_INSTRUCTION_COMMENTS)
	currentTranslation = "StoreCachedRegisters";
#endif
		while (ins)
		{
			if (ins->Rd1.regID < REG_TEMP
					&& !(ins->instruction == ARM_LDR && ins->R2.regID == REG_EMU_FP && ins->offset >= 0))	// to account for LoadCachedRegisters()
			{
				int32_t nextUsed = FindRegNextUpdated(ins, ins->Rd1.regID);

				if (nextUsed == -2)
				{
					new_ins = newInstrI(ARM_STR, AL, REG_NOT_USED, ins->Rd1.regID, REG_EMU_FP, RegMemByteOffset(ins->Rd1.regID));
					ADD_LL_NEXT(new_ins, ins);
				}
				if (nextUsed == -1) // Register will be over-written before next use so don't bother saving
				{

				}
			}

			if (ins->Rd2.regID < REG_TEMP
					&& !(ins->instruction == ARM_LDR && ins->R2.regID == REG_EMU_FP && ins->offset >= 0))
			{
				int32_t nextUsed = FindRegNextUsedAgain(ins, ins->Rd2.regID);

				if (nextUsed == -2)
				{
					new_ins = newInstrI(ARM_STR, AL, REG_NOT_USED, ins->Rd2.regID, REG_EMU_FP, RegMemByteOffset(ins->Rd2.regID));
					ADD_LL_NEXT(new_ins, ins);
				}
				if (nextUsed == -1) // Register will be over-written before next use so don't bother saving
				{

				}
			}

			ins = ins->nextInstruction;
		}
}
