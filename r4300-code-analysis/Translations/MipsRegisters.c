/*
 * MipsRegisters.c
 *
 *  Created on: 23 Oct 2014
 *      Author: rjhender
 */

#include "Translate.h"
#include "string.h"
#include "DebugDefines.h"

static int32_t FindRegNextUsedAgain(const Instruction_t* const ins, const regID_t Reg)
{
	const Instruction_t* in = ins;
	uint32_t x = 0;

	while (in)
	{
		if (in->R1.regID == Reg || in->R2.regID == Reg || in->R3.regID == Reg)
			return x;
		if (x && (in->Rd1.regID == Reg || in->Rd2.regID == Reg))
			return -1;

		x++;
		in = in->nextInstruction;
	}

	return 0;
}

static void UpdateRegWithReg(Instruction_t* const ins, const regID_t RegFrom, const regID_t RegTo, uint32_t MaxInstructions)
{
	Instruction_t* in = ins;
	uint32_t x = MaxInstructions;

	if (!x) x = 0xffffffff;

#if defined(SHOW_REG_TRANSLATION_MAP)
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
#endif

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
}

void Translate_LoadCachedRegisters(code_seg_t* const codeSegment)
{
	Instruction_t*ins = codeSegment->Intermcode;
	Instruction_t*prev_ins = NULL;

#if defined(USE_INSTRUCTION_COMMENTS)
	currentTranslation = "LoadCachedRegisters";
#endif

	Instruction_t*new_ins;

	while (ins)
	{
		if (ins->R1.regID < REG_TEMP)
		{
			new_ins = newInstrI(ARM_LDR, AL, ins->R1.regID, REG_NOT_USED, REG_EMU_FP, ins->R1.regID * 4);
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

		if (ins->R2.regID < REG_TEMP)
		{
			new_ins = newInstrI(ARM_LDR, AL, ins->R2.regID, REG_NOT_USED, REG_EMU_FP, ins->R2.regID * 4);
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

		if (ins->R3.regID < REG_TEMP)
		{
			new_ins = newInstrI(ARM_LDR, AL, ins->R3.regID, REG_NOT_USED, REG_EMU_FP, ins->R3.regID * 4);
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

static void getNextRegister(Instruction_t* ins, uint32_t* uiCurrentRegister)
{
	uint32_t uiLastRegister = *uiCurrentRegister;

	while ((FindRegNextUsedAgain(ins, REG_HOST + *uiCurrentRegister) > 0))
	{
		(*uiCurrentRegister)++;
		if (*uiCurrentRegister > 10) *uiCurrentRegister = 0;

		// Have we looped round all registers?
		if (uiLastRegister == *uiCurrentRegister){
			abort();
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

#if defined(SHOW_REG_TRANSLATION_MAP)
	printf("Segment 0x%x uses %d registers\n",(uint32_t)codeSegment, NumberRegUsed);
#endif

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

		getNextRegister(ins, &uiCurrentRegister);

		while (ins)
		{
			if (ins->Rd1.regID != REG_NOT_USED  && ins->Rd1.regID < REG_HOST){
				UpdateRegWithReg(ins,ins->Rd1.regID, REG_HOST + uiCurrentRegister, 0);
				getNextRegister(ins, &uiCurrentRegister);
			}

			if (ins->Rd2.regID != REG_NOT_USED && ins->Rd2.regID < REG_HOST){
				UpdateRegWithReg(ins,ins->Rd2.regID, REG_HOST + uiCurrentRegister, 0);
				getNextRegister(ins, &uiCurrentRegister);
			}

			if (ins->R1.regID != REG_NOT_USED && ins->R1.regID < REG_HOST){
				UpdateRegWithReg(ins,ins->R1.regID, REG_HOST + uiCurrentRegister, 0);
				getNextRegister(ins, &uiCurrentRegister);
			}

			if (ins->R2.regID != REG_NOT_USED && ins->R2.regID < REG_HOST){
				UpdateRegWithReg(ins,ins->R2.regID, REG_HOST + uiCurrentRegister, 0);
				getNextRegister(ins, &uiCurrentRegister);
			}

			if (ins->R3.regID != REG_NOT_USED && ins->R3.regID < REG_HOST){
				UpdateRegWithReg(ins,ins->R3.regID, REG_HOST + uiCurrentRegister, 0);
				getNextRegister(ins, &uiCurrentRegister);
			}

			ins = ins->nextInstruction;
		}
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
					&& ins->Rd1.state == RS_REGISTER
				)
			{
				int32_t nextUsed = FindRegNextUsedAgain(ins, ins->Rd1.regID);

				//Register will be over-written before next use so don't bother saving
				if (nextUsed == -1)
				{

				}
				else if (nextUsed == 0)
				{
					new_ins = newInstrI(ARM_STR_LIT, AL, REG_NOT_USED, ins->Rd1.regID, REG_EMU_FP, ins->Rd1.regID * 4);
					new_ins->nextInstruction = ins->nextInstruction;
					ins->nextInstruction = new_ins;
					ins = ins->nextInstruction;
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

				new_ins = newInstrI(ARM_STR_LIT, AL, REG_NOT_USED, REG_TEMP_STR_CONST, REG_EMU_FP, ins->Rd1.regID * 4);
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
					new_ins = newInstrI(ARM_STR_LIT, AL, REG_NOT_USED, ins->Rd2.regID, REG_EMU_FP, ins->Rd2.regID * 4);
					new_ins->nextInstruction = ins->nextInstruction;
					ins->nextInstruction = new_ins;
					ins = ins->nextInstruction;
				}
			}
			else if (ins->Rd2.regID < REG_TEMP)
			{
				abort();
			}

			ins = ins->nextInstruction;
		}
}
