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
#include "InstructionSet_ascii.h"

/*
 * Function to turn 64bit registers into multiple 32-bit registers
 *
 */
void Translate_ALU(code_seg_t* const codeSegment)
{
	Instruction_t*ins;
	Instruction_t*new_ins;
	regID_t Rd1, R1, R2;
	ins = codeSegment->Intermcode;

#if USE_INSTRUCTION_COMMENTS
	currentTranslation = "ALU";
#endif

	while (ins)
	{
		Rd1 = ins->Rd1.regID;
		R1 = ins->R1.regID;
		R2 = ins->R2.regID;
		int32_t imm = ins->immediate;
		switch (ins->instruction)
		{
			case MIPS_SLL:
				{
					Instr(ins, ARM_MOV, AL, Rd1 , REG_NOT_USED, R1);
					ins->shift = imm;
					ins->shiftType = LOGICAL_LEFT;
				}break;
			case MIPS_SRL:
				{
					Instr(ins, ARM_MOV, AL, Rd1 , REG_NOT_USED, R1);
					ins->shift = imm;
					ins->shiftType = LOGICAL_RIGHT;
				}break;
			case MIPS_SRA:
				{
					Instr(ins, ARM_MOV, AL, Rd1 , REG_NOT_USED, R1);
					ins->shift = imm;
					ins->shiftType = ARITHMETIC_RIGHT;
				}break;
			case MIPS_SLLV:
				{
					Instr(ins, ARM_MOV, AL, Rd1 , REG_NOT_USED, R1);
					ins->R3.regID = R2;
					ins->shiftType = LOGICAL_LEFT;
				}break;
			case MIPS_SRLV:
				{
					Instr(ins, ARM_MOV, AL, Rd1 , REG_NOT_USED, R1);
					ins->R3.regID = R2;
					ins->shiftType = LOGICAL_RIGHT;
				}break;
			case MIPS_SRAV:
				{
					Instr(ins, ARM_MOV, AL, Rd1 , REG_NOT_USED, R1);
					ins->R3.regID = R2;
					ins->shiftType = ARITHMETIC_RIGHT;
				}break;
			case MIPS_DSLLV:
				/*
				 *		Rd1 W        Rd1            R1 W           R1               R2 W          R2
				 * [FF FF FF FF | FF FF FF FE] = [FF FF FF FF | FF FF FF FF] << [00 00 00 00 | 00 00 00 3F]
				 *
				 *
				 */

				// 1. Work out lower Word
				Instr(ins, ARM_MOV, AL, Rd1 , REG_NOT_USED, R1);
				ins->shiftType = LOGICAL_LEFT;
				ins->R3.regID = R2;

				// 2. Work out upper word
				new_ins = newInstr(ARM_MOV, AL, Rd1 | REG_WIDE, REG_NOT_USED, R1 | REG_WIDE);
				new_ins->shiftType = LOGICAL_LEFT;
				new_ins->R3.regID = R2;
				ADD_LL_NEXT(new_ins, ins);

				// 3. Work out lower shifted to upper
				new_ins = newInstrIS(ARM_RSB, AL, REG_TEMP_SCRATCH2, REG_NOT_USED, R2, 32);
				ADD_LL_NEXT(new_ins, ins);

				new_ins = newInstr(ARM_ORR, PL, Rd1 | REG_WIDE, REG_NOT_USED, R1);
				new_ins->shiftType = LOGICAL_RIGHT;
				new_ins->R3.regID = REG_TEMP_SCRATCH2;
				ADD_LL_NEXT(new_ins, ins);

				// 4. Work out R1 << into Rd1 W (i.e. where R2 > 32) If this occurs then Step 1 and 2 didn't do anything
				new_ins = newInstrIS(ARM_SUB, AL, REG_TEMP_SCRATCH1, REG_NOT_USED, R1, 32);
				ADD_LL_NEXT(new_ins, ins);

				new_ins = newInstr(ARM_ORR, PL, Rd1 | REG_WIDE, R1, REG_TEMP_SCRATCH1);
				ADD_LL_NEXT(new_ins, ins);

				break;
			case MIPS_DSRLV:
				/*
				 *
				 * [7F FF FF FF | FF FF FF FF] = [FF FF FF FF | FF FF FF FF] >> [00 00 00 00 | 00 00 00 3F]
				 *
				 *
				 */

				//Work out lower Word
				Instr(ins, ARM_MOV, AL, ins->Rd1.regID, REG_NOT_USED, ins->R1.regID);
				ins->shiftType = LOGICAL_RIGHT;
				ins->R3.regID = R2;

				//Work out upper word
				new_ins = newInstr(ARM_MOV, AL, ins->Rd1.regID| REG_WIDE, REG_NOT_USED, ins->Rd1.regID | REG_WIDE);
				new_ins->shiftType = LOGICAL_RIGHT;
				new_ins->R3.regID = ins->R2.regID;
				ADD_LL_NEXT(new_ins, ins);

				//Work out upper shifted to lower
				new_ins = newInstrIS(ARM_SUB, AL, REG_TEMP_SCRATCH1, REG_NOT_USED, ins->R1.regID, 32);
				ADD_LL_NEXT(new_ins, ins);

				new_ins = newInstr(ARM_MOV, PL, REG_TEMP_SCRATCH2, ins->R1.regID, REG_NOT_USED);
				new_ins->shiftType = LOGICAL_RIGHT;
				new_ins->R3.regID = REG_TEMP_SCRATCH1;
				ADD_LL_NEXT(new_ins, ins);

				new_ins = newInstr(ARM_ORR, PL, ins->Rd1.regID| REG_WIDE, ins->Rd1.regID | REG_WIDE, REG_TEMP_SCRATCH1);
				ADD_LL_NEXT(new_ins, ins);
				break;
			case MIPS_DSRAV: break;
			case MIPS_MULT: break;
			case MIPS_MULTU: break;
			case MIPS_DIV:					//TODO DIV uses signed!
				/*
				 *  clz  r3, r0                 r3 ← CLZ(r0) Count leading zeroes of N
					clz  r2, r1                 r2 ← CLZ(r1) Count leading zeroes of D
					sub  r3, r2, r3             r3 ← r2 - r3.
												 This is the difference of zeroes
												 between D and N.
												 Note that N >= D implies CLZ(N) <= CLZ(D)
					add r3, r3, #1              Loop below needs an extra iteration count

					mov r2, #0                  r2 ← 0
					b .Lloop_check4
					.Lloop4:
					  cmp r0, r1, lsl r3        Compute r0 - (r1 << r3) and update cpsr
					  adc r2, r2, r2            r2 ← r2 + r2 + C.
												  Note that if r0 >= (r1 << r3) then C=1, C=0 otherwise
					  subcs r0, r0, r1, lsl r3  r0 ← r0 - (r1 << r3) if C = 1 (this is, only if r0 >= (r1 << r3) )
					.Lloop_check4:
					  subs r3, r3, #1           r3 ← r3 - 1
					  bpl .Lloop4               if r3 >= 0 (N=0) then branch to .Lloop1

					mov r0, r2
				 */
				{
					// Count leading Zeros of N
					Instr(ins, ARM_CLZ, AL, REG_TEMP_SCRATCH3, R1, REG_NOT_USED);

					// Count leading Zeros of D
					new_ins = newInstr(ARM_CLZ, AL, REG_TEMP_SCRATCH2, R2, REG_NOT_USED);
					ADD_LL_NEXT(new_ins, ins);

					new_ins = newInstr(ARM_MOV, AL, REG_TEMP_SCRATCH0, REG_NOT_USED, R1);
					ADD_LL_NEXT(new_ins, ins);

					new_ins = newInstr(ARM_SUB, AL, REG_TEMP_SCRATCH3, REG_TEMP_GEN2, REG_TEMP_SCRATCH3);
					ADD_LL_NEXT(new_ins, ins);

					new_ins = newInstrI(ARM_ADD, AL, REG_TEMP_SCRATCH3, REG_TEMP_GEN3, REG_NOT_USED, 1);
					ADD_LL_NEXT(new_ins, ins);

					new_ins = newInstrI(ARM_MOV, AL, REG_TEMP_SCRATCH2, REG_NOT_USED, REG_NOT_USED, 0);
					ADD_LL_NEXT(new_ins, ins);

					new_ins = newInstrB(AL, 4, 0);
					new_ins->I = 0;
					ADD_LL_NEXT(new_ins, ins);

					new_ins = newInstr(ARM_CMP, AL, REG_NOT_USED, R1, REG_TEMP_SCRATCH0);
					new_ins->R3.regID = REG_TEMP_GEN3;
					new_ins->shiftType = LOGICAL_LEFT;
					ADD_LL_NEXT(new_ins, ins);

					new_ins = newInstrI(ARM_ADC, AL, REG_TEMP_SCRATCH2, REG_TEMP_SCRATCH2, REG_TEMP_SCRATCH2, 0);
					ADD_LL_NEXT(new_ins, ins);

					new_ins = newInstr(ARM_SUB, CS, REG_TEMP_SCRATCH0, REG_TEMP_SCRATCH0, R2);
					new_ins->R3.regID = REG_TEMP_GEN3;
					new_ins->shiftType = LOGICAL_LEFT;
					ADD_LL_NEXT(new_ins, ins);

					new_ins = newInstrIS(ARM_SUB, AL, REG_TEMP_SCRATCH2, REG_NOT_USED, REG_NOT_USED, 1);
					ADD_LL_NEXT(new_ins, ins);

					new_ins = newInstrB(PL, -4, 0);
					new_ins->I = 0;
					ADD_LL_NEXT(new_ins, ins);

					new_ins = newInstr(ARM_MOV, AL, Rd1, REG_NOT_USED, REG_TEMP_SCRATCH0);
					ADD_LL_NEXT(new_ins, ins);

					TRANSLATE_ABORT();
				}
				break;
			case MIPS_DIVU:
			case MIPS_DMULT:
			case MIPS_DMULTU:
			case MIPS_DDIV:
			case MIPS_DDIVU:
				TRANSLATE_ABORT();
				break;
			case MIPS_ADD:	// TODO TRAP
				{
					// Destination register is conditionally changed so load it
					Instr(ins, DR_NO_OP, AL, REG_NOT_USED, Rd1, REG_NOT_USED);

					new_ins = newInstrS( ARM_ADD, AL, REG_TEMP_SCRATCH0, R1, R2);
					ADD_LL_NEXT(new_ins, ins);

					new_ins = newInstr(ARM_MOV, VC, Rd1, REG_NOT_USED, REG_TEMP_SCRATCH0);
					ADD_LL_NEXT(new_ins, ins);
				} break;
			case MIPS_ADDU:
				{
					Instr(ins, ARM_ADD, AL, Rd1, R1, R2);
				} break;
			case MIPS_AND:
				{
					Instr(ins, ARM_AND, AL, Rd1, R1, R2);

					new_ins = newInstr(ARM_AND, AL, Rd1 | REG_WIDE, R1| REG_WIDE, R2 | REG_WIDE);
					ADD_LL_NEXT(new_ins, ins);
				}break;
			case MIPS_OR:
				{
					Instr(ins, ARM_ORR, AL, Rd1, R1, R2);

					new_ins = newInstr(ARM_ORR, AL, Rd1 | REG_WIDE, R1| REG_WIDE, R2 | REG_WIDE);
					ADD_LL_NEXT(new_ins, ins);
				}
				break;
			case MIPS_XOR:
				{
					Instr(ins, ARM_EOR, AL, Rd1, R1, R2);

					new_ins = newInstr(ARM_EOR, AL, Rd1 | REG_WIDE, R1| REG_WIDE, R2 | REG_WIDE);
					ADD_LL_NEXT(new_ins, ins);
				}
				break;
			case MIPS_NOR:
				Instr(ins, ARM_MVN, AL, REG_TEMP_SCRATCH0, REG_NOT_USED, R2);

				new_ins = newInstr(ARM_ORR, AL, Rd1, REG_TEMP_SCRATCH0, R2);
				ADD_LL_NEXT(new_ins, ins);
				break;
			case MIPS_SLT:
				Instr(ins, ARM_CMP, AL, REG_NOT_USED, R1, R2);

				new_ins = newInstrI(ARM_MOV, LT, Rd1, REG_NOT_USED, REG_NOT_USED, 1);
				ADD_LL_NEXT(new_ins, ins);

				new_ins = newInstrI(ARM_MOV, GE, Rd1, REG_NOT_USED, REG_NOT_USED, 0);
				ADD_LL_NEXT(new_ins, ins);
				break;
			case MIPS_SLTU:
				Instr(ins, ARM_CMP, AL, REG_NOT_USED, R1, R2);

				new_ins = newInstrI(ARM_MOV, CC, Rd1, REG_NOT_USED, REG_NOT_USED, 1);
				ADD_LL_NEXT(new_ins, ins);

				new_ins = newInstrI(ARM_MOV, CS, Rd1, REG_NOT_USED, REG_NOT_USED, 0);
				ADD_LL_NEXT(new_ins, ins);
				break;
			case MIPS_DADD:	// TRAP
			case MIPS_DADDU:
				{
					InstrS(ins, ARM_ADD, AL, Rd1, R1, R2);

					new_ins = newInstr(ARM_ADC, AL, Rd1 | REG_WIDE, R1| REG_WIDE, R2 | REG_WIDE);
					ADD_LL_NEXT(new_ins, ins);
				}
				break;
			case MIPS_TGE:
			case MIPS_TGEU:
			case MIPS_TLT:
			case MIPS_TLTU:
			case MIPS_TEQ:
			case MIPS_TNE:
				TRANSLATE_ABORT();
				break;
			case MIPS_DSLL:
				{
					assert(imm < 32);

					InstrI(ins, ARM_MOV, AL, Rd1 | REG_WIDE, REG_NOT_USED, R1| REG_WIDE, imm);
					new_ins->shiftType = LOGICAL_LEFT;

					new_ins = newInstrI(ARM_ORR, AL, Rd1 | REG_WIDE, Rd1 | REG_WIDE, R1, 32 - imm);
					new_ins->shiftType = LOGICAL_RIGHT;
					ADD_LL_NEXT(new_ins, ins);

					new_ins = newInstrI(ARM_MOV, AL, Rd1 , REG_NOT_USED, R1, imm);
					ins->shiftType = LOGICAL_LEFT;
					ADD_LL_NEXT(new_ins, ins);
				}break;
			case MIPS_DSRL:
				{
					assert(imm < 32);

					InstrI(ins, ARM_MOV, AL, Rd1 , REG_NOT_USED, R1, imm);
					ins->shiftType = LOGICAL_RIGHT;

					new_ins = newInstrI(ARM_ORR, AL, Rd1 , Rd1 , R1| REG_WIDE, 32 - imm);
					new_ins->shiftType = LOGICAL_LEFT;
					ADD_LL_NEXT(new_ins, ins);

					new_ins = newInstrI(ARM_MOV, AL, Rd1 | REG_WIDE, REG_NOT_USED, R1| REG_WIDE, imm);
					new_ins->shiftType = LOGICAL_RIGHT;
					ADD_LL_NEXT(new_ins, ins);
				}break;
			case MIPS_DSRA:
				{
					assert(imm < 32);

					InstrI(ins, ARM_MOV, AL, Rd1 , REG_NOT_USED, R1, imm);
					ins->shiftType = LOGICAL_RIGHT;

					new_ins = newInstrI(ARM_ORR, AL, Rd1 , Rd1 , R1| REG_WIDE, 32 - imm);
					new_ins->shiftType = LOGICAL_LEFT;
					ADD_LL_NEXT(new_ins, ins);

					new_ins = newInstrI(ARM_MOV, AL, Rd1 | REG_WIDE, REG_NOT_USED, R1| REG_WIDE, imm);
					new_ins->shiftType = ARITHMETIC_RIGHT;
					ADD_LL_NEXT(new_ins, ins);
				}break;
			case MIPS_DSLL32:
				{
					InstrI(ins, ARM_MOV, AL, Rd1 |REG_WIDE , REG_NOT_USED, R1, imm);
					ins->shiftType = LOGICAL_LEFT;
				}
				break;
			case MIPS_DSRL32:
				{
					InstrI(ins, ARM_MOV, AL, Rd1 , REG_NOT_USED, R1|REG_WIDE , imm);
					ins->shiftType = LOGICAL_RIGHT;
				}
				break;
			case MIPS_DSRA32:
				{
					InstrIS(ins, ARM_MOV, AL, Rd1 , REG_NOT_USED, R1|REG_WIDE , imm);
					ins->shiftType = ARITHMETIC_RIGHT;

					new_ins = newInstrI(ARM_MVN, MI, Rd1 | REG_WIDE, REG_NOT_USED, REG_NOT_USED, 0);
					ADD_LL_NEXT(new_ins, ins);
				}
				break;
			case MIPS_TGEI:
			case MIPS_TGEIU:
			case MIPS_TLTI:
			case MIPS_TLTIU:
			case MIPS_TEQI:
			case MIPS_TNEI:
				TRANSLATE_ABORT();
				break;
			case MIPS_ADDI:	// TRAP
			{
				if (imm < 0)
				{
					int32_t ImmShift = Imm8Shift((uint16_t)-ins->immediate);

					if (ImmShift == -1)
					{
						if (0 == R1)
						{
							InstrI(ins, ARM_MOV, AL, Rd1, REG_NOT_USED, REG_NOT_USED, (-imm&0xFF));

							new_ins = newInstrI(ARM_SUB, AL, Rd1, Rd1, REG_NOT_USED, -imm&0xFF00);
							ADD_LL_NEXT(new_ins, ins);
						}
						else
						{
							Instr(ins, DR_NO_OP, AL, Rd1, Rd1, REG_NOT_USED);

							new_ins = newInstrI(ARM_SUB, AL, Rd1, R1, REG_NOT_USED, (-imm&0xFF));
							ADD_LL_NEXT(new_ins, ins);

							new_ins = newInstrI(ARM_SUB, AL, REG_TEMP_SCRATCH0, REG_TEMP_SCRATCH0, REG_NOT_USED, -imm&0xFF00);
							ADD_LL_NEXT(new_ins, ins);

							new_ins = newInstr(ARM_ADD, VC, Rd1, Rd1, REG_TEMP_SCRATCH0);
							ADD_LL_NEXT(new_ins, ins);
						}
					}
					else
					{
						InstrI(ins, ARM_SUB, AL, Rd1, R1, REG_NOT_USED, (-imm));
					}
				}
				else if (imm > 0)
				{
					int32_t ImmShift = Imm8Shift((uint16_t)ins->immediate);

					if (ImmShift == -1)
					{
						if (0 == R1)
						{
							InstrI(ins, ARM_MOV, AL, Rd1, REG_NOT_USED, REG_NOT_USED, (imm&0xFF));

							new_ins = newInstrI(ARM_ADD, AL, Rd1, Rd1, REG_NOT_USED, imm&0xFF00);
							ADD_LL_NEXT(new_ins, ins);
						}
						else
						{
							Instr(ins, DR_NO_OP, AL, REG_NOT_USED, Rd1, REG_NOT_USED);

							new_ins = newInstrIS(ARM_ADD, AL, REG_TEMP_SCRATCH0, R1, REG_NOT_USED, (imm&0xFF));
							ADD_LL_NEXT(new_ins, ins);

							new_ins = newInstrIS(ARM_ADD, VC, REG_TEMP_SCRATCH0, REG_TEMP_SCRATCH0, REG_NOT_USED, imm&0xFF00);
							ADD_LL_NEXT(new_ins, ins);

							new_ins = newInstr(ARM_ADD, VC, Rd1, Rd1, REG_TEMP_SCRATCH0);
							ADD_LL_NEXT(new_ins, ins);
						}
					}
					else
					{
						if (0 == R1)
						{
							InstrI(ins, ARM_MOV, AL, Rd1, REG_NOT_USED, REG_NOT_USED, imm);
						}
						else
						{
							InstrIS(ins, ARM_ADD, AL, REG_TEMP_SCRATCH0, R1, REG_NOT_USED, imm);

							new_ins = newInstr(ARM_MOV, VC, Rd1, REG_NOT_USED, REG_TEMP_SCRATCH0);
							ADD_LL_NEXT(new_ins, ins);
						}
					}
				}
				else if (Rd1 == R1) // imm = 0
				{
					Instr(ins, DR_NO_OP, AL, REG_NOT_USED, REG_NOT_USED, REG_NOT_USED);
				}
			}
			break;
			case MIPS_ADDIU:
				{
					if (imm < 0)
					{
						int32_t ImmShift = Imm8Shift((uint16_t)-ins->immediate);

						if (ImmShift == -1)
						{
							if (0 == R1)
							{
								InstrI(ins, ARM_MOV, AL, Rd1, REG_NOT_USED, REG_NOT_USED, (-imm&0xFF));

								new_ins = newInstrI(ARM_SUB, AL, Rd1, Rd1, REG_NOT_USED, -imm&0xFF00);
								ADD_LL_NEXT(new_ins, ins);
							}
							else
							{
								InstrI(ins, ARM_SUB, AL, Rd1, R1, REG_NOT_USED, (-imm&0xFF));

								new_ins = newInstrI(ARM_SUB, AL, REG_TEMP_SCRATCH0, R1, REG_NOT_USED, -imm&0xFF00);
								ADD_LL_NEXT(new_ins, ins)

								new_ins = newInstr(ARM_ADD, AL, Rd1, Rd1, REG_TEMP_SCRATCH0);
								ADD_LL_NEXT(new_ins, ins);
							}
						}
						else
						{
							InstrI(ins, ARM_SUB, AL, Rd1, R1, REG_NOT_USED, (-imm));
						}
					}
					else if (imm > 0)
					{
						int32_t ImmShift = Imm8Shift((uint16_t)ins->immediate);

						if (ImmShift == -1)
						{
							if (0 == R1)
							{
								InstrI(ins, ARM_MOV, AL, Rd1, REG_NOT_USED, REG_NOT_USED, (imm&0xFF));

								new_ins = newInstrI(ARM_ADD, AL, Rd1, Rd1, REG_NOT_USED, imm&0xFF00);
								ADD_LL_NEXT(new_ins, ins);
							}
							else
							{
								InstrI(ins, ARM_ADD, AL, REG_TEMP_SCRATCH0, R1, REG_NOT_USED, (imm&0xFF));

								new_ins = newInstrI(ARM_ADD, AL, REG_TEMP_SCRATCH0, REG_TEMP_SCRATCH0, REG_NOT_USED, imm&0xFF00);
								ADD_LL_NEXT(new_ins, ins)

								new_ins = newInstr(ARM_ADD, AL, Rd1, Rd1, REG_TEMP_SCRATCH0);
								ADD_LL_NEXT(new_ins, ins);
							}
						}
						else
						{
							if (0 == R1)
							{
								InstrI(ins, ARM_MOV, AL, Rd1, REG_NOT_USED, REG_NOT_USED, imm);
							}
							else
							{
								InstrI(ins, ARM_ADD, AL, Rd1, R1, REG_NOT_USED, imm);
							}
						}
					}
					else if (Rd1 == R1) // imm = 0
					{
						Instr(ins, DR_NO_OP, AL, REG_NOT_USED, REG_NOT_USED, REG_NOT_USED);
					}
				}
				break;
			case MIPS_SLTI:
				if (imm < 0)	//TODO no idea if this is right
				{
					InstrI(ins, ARM_MVN, AL, REG_TEMP_SCRATCH0, REG_NOT_USED, REG_NOT_USED, (imm)&0xff);

					if (imm < 255)
					{
						new_ins = newInstrI(ARM_SUB, AL, Rd1, R1, REG_NOT_USED, (-imm)&0xFF00);
						ADD_LL_NEXT(new_ins, ins);
					}
				}
				else
				{
					InstrI(ins, ARM_MOV, AL, REG_TEMP_SCRATCH0, REG_NOT_USED, REG_NOT_USED, (imm)&0xff);

					if (imm > 255)
					{
						new_ins = newInstrI(ARM_ADD, AL, REG_TEMP_SCRATCH0, REG_NOT_USED, REG_NOT_USED, (imm)&0xFF00);
						ADD_LL_NEXT(new_ins, ins);
					}
				}

				new_ins = newInstr(ARM_CMP, AL, REG_NOT_USED, R1, REG_TEMP_SCRATCH0);
				ADD_LL_NEXT(new_ins, ins);

				new_ins = newInstrI(ARM_MOV, LT, Rd1, REG_NOT_USED, REG_NOT_USED, 1);
				ADD_LL_NEXT(new_ins, ins);

				new_ins = newInstrI(ARM_MOV, GE, Rd1, REG_NOT_USED, REG_NOT_USED, 0);
				ADD_LL_NEXT(new_ins, ins);

				break;
			case MIPS_SLTIU:
				if (imm < 0)	//TODO no idea if this is right
				{
					InstrI(ins, ARM_MVN, AL, REG_TEMP_SCRATCH0, REG_NOT_USED, REG_NOT_USED, (imm)&0xff);

					if (imm < 255)
					{
						new_ins = newInstrI(ARM_SUB, AL, Rd1, R1, REG_NOT_USED, (-imm)&0xFF00);
						ADD_LL_NEXT(new_ins, ins);
					}
				}
				else
				{
					InstrI(ins, ARM_MOV, AL, REG_TEMP_SCRATCH0, REG_NOT_USED, REG_NOT_USED, (imm)&0xff);

					if (imm > 255)
					{
						new_ins = newInstrI(ARM_ADD, AL, REG_TEMP_SCRATCH0, REG_NOT_USED, REG_NOT_USED, (imm)&0xFF00);
						ADD_LL_NEXT(new_ins, ins);
					}
				}

				new_ins = newInstr(ARM_CMP, AL, REG_NOT_USED, R1, REG_TEMP_SCRATCH0);
				ADD_LL_NEXT(new_ins, ins);

				new_ins = newInstrI(ARM_MOV, CC, Rd1, REG_NOT_USED, REG_NOT_USED, 1);
				ADD_LL_NEXT(new_ins, ins);

				new_ins = newInstrI(ARM_MOV, CS, Rd1, REG_NOT_USED, REG_NOT_USED, 0);
				ADD_LL_NEXT(new_ins, ins);

				break;
			case MIPS_ANDI:
			{
				int32_t ImmShift = Imm8Shift((uint16_t)ins->immediate);

				if (ImmShift == -1)
				{

					InstrI(ins, ARM_AND, AL, Rd1, R1, REG_NOT_USED, (imm)&0x0000FF00);

					new_ins = newInstrI(ARM_AND, AL, REG_TEMP_SCRATCH0, R1, REG_NOT_USED, (imm)&0x000000FF);
					ADD_LL_NEXT(new_ins, ins);

					new_ins = newInstr(ARM_ORR, AL, Rd1, Rd1, REG_TEMP_SCRATCH0);
					ADD_LL_NEXT(new_ins, ins);
				}
				else
				{
					InstrI(ins, ARM_AND, AL, Rd1, R1, REG_NOT_USED, (imm)&0x0000FFFF);
					//ADD_LL_NEXT(new_ins, ins);
				}

				// 64 bit part
				new_ins = newInstrI(ARM_MOV, AL, Rd1 | REG_WIDE, REG_NOT_USED, REG_NOT_USED, 0);
				ADD_LL_NEXT(new_ins, ins);

				break;
			}
			case MIPS_ORI:
			{
				int32_t ImmShift = Imm8Shift((uint16_t)imm);

				if (ImmShift == -1)
				{

					InstrI(ins, ARM_ORR, AL, Rd1, R1, REG_NOT_USED, (imm)&0x0000FF00);

					new_ins = newInstrI(ARM_ORR, AL, Rd1, Rd1, REG_NOT_USED, (imm)&0x000000FF);
					ADD_LL_NEXT(new_ins, ins);
				}
				else
				{
					InstrI(ins, ARM_ORR, AL, Rd1, R1, REG_NOT_USED, (imm)&0x0000FFFF);
				}

				// 64 bit part
				new_ins = newInstr(ARM_MOV, AL, Rd1 | REG_WIDE, REG_NOT_USED, R1 | REG_WIDE);
				ADD_LL_NEXT(new_ins, ins);

				break;
			}
			case MIPS_XORI:
			{
				int32_t ImmShift = Imm8Shift((uint16_t)imm);

				if (ImmShift == -1)
				{
					InstrI(ins, ARM_EOR, AL, Rd1, R1, REG_NOT_USED, (imm)&0x0000FF00);

					new_ins = newInstrI(ARM_EOR, AL, Rd1, Rd1, REG_NOT_USED, (imm)&0x000000FF);
					ADD_LL_NEXT(new_ins, ins);
				}
				else
				{
					InstrI(ins, ARM_EOR, AL, Rd1, R1, REG_NOT_USED, (imm)&0x0000FFFF);;
				}

				// 64 bit part
				new_ins = newInstr(ARM_MOV, AL, Rd1 | REG_WIDE, REG_NOT_USED, R1 | REG_WIDE);
				ADD_LL_NEXT(new_ins, ins);

				break;
			}
			case MIPS_DADDI:
			{
				if (imm < 0)
				{
					int32_t ImmShift = Imm8Shift((uint16_t)-ins->immediate);

					if (ImmShift == -1)
					{
						if (0 == R1)
						{
							InstrI(ins, ARM_MOV, AL, Rd1, REG_NOT_USED, REG_NOT_USED, (-imm&0xFF));

							new_ins = newInstrI(ARM_SUB, AL, Rd1, Rd1, REG_NOT_USED, -imm&0xFF00);
							ADD_LL_NEXT(new_ins, ins);
						}
						else
						{
							Instr(ins, DR_NO_OP, AL, Rd1, Rd1, REG_NOT_USED);

							new_ins = newInstrI(ARM_SUB, AL, Rd1, R1, REG_NOT_USED, (-imm&0xFF));
							ADD_LL_NEXT(new_ins, ins);

							new_ins = newInstrI(ARM_SUB, AL, REG_TEMP_SCRATCH0, REG_TEMP_SCRATCH0, REG_NOT_USED, -imm&0xFF00);
							ADD_LL_NEXT(new_ins, ins);

							new_ins = newInstr(ARM_ADD, VC, Rd1, Rd1, REG_TEMP_SCRATCH0);
							ADD_LL_NEXT(new_ins, ins);
						}
					}
					else
					{
						InstrI(ins, ARM_SUB, AL, Rd1, R1, REG_NOT_USED, (-imm));
					}
				}
				else if (imm > 0)
				{
					int32_t ImmShift = Imm8Shift((uint16_t)ins->immediate);

					if (ImmShift == -1)
					{
						if (0 == R1)
						{
							InstrI(ins, ARM_MOV, AL, Rd1, REG_NOT_USED, REG_NOT_USED, (imm&0xFF));

							new_ins = newInstrI(ARM_ADD, AL, Rd1, Rd1, REG_NOT_USED, imm&0xFF00);
							ADD_LL_NEXT(new_ins, ins);
						}
						else
						{
							Instr(ins, DR_NO_OP, AL, REG_NOT_USED, Rd1, REG_NOT_USED);

							new_ins = newInstrIS(ARM_ADD, AL, REG_TEMP_SCRATCH0, R1, REG_NOT_USED, (imm&0xFF));
							ADD_LL_NEXT(new_ins, ins);

							new_ins = newInstrIS(ARM_ADD, VC, REG_TEMP_SCRATCH0, REG_TEMP_SCRATCH0, REG_NOT_USED, imm&0xFF00);
							ADD_LL_NEXT(new_ins, ins);

							new_ins = newInstr(ARM_ADD, VC, Rd1, Rd1, REG_TEMP_SCRATCH0);
							ADD_LL_NEXT(new_ins, ins);
						}
					}
					else
					{
						if (0 == R1)
						{
							InstrI(ins, ARM_MOV, AL, Rd1, REG_NOT_USED, REG_NOT_USED, imm);
						}
						else
						{
							InstrIS(ins, ARM_ADD, AL, REG_TEMP_SCRATCH0, R1, REG_NOT_USED, imm);

							new_ins = newInstr(ARM_MOV, VC, Rd1, REG_NOT_USED, REG_TEMP_SCRATCH0);
							ADD_LL_NEXT(new_ins, ins);
						}
					}
				}
				else if (Rd1 == R1) // imm = 0
				{
					Instr(ins, DR_NO_OP, AL, REG_NOT_USED, REG_NOT_USED, REG_NOT_USED);
				}
				break;
			}
			case MIPS_DADDIU:
				TRANSLATE_ABORT();
				break;
		default: break;
		}

		ins = ins->nextInstruction;
	}
}
