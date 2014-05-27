
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "CodeSegments.h"
#include "Debugger.h"
#include "InstructionSetMIPS4.h"


static char userInput[20][20];

static code_seg_t* CurrentCodeSeg;

void getCmd() {

	unsigned char line[400];
    int c,d;
    int argN = 0;

    for(c=0;c<sizeof(userInput[argN])-1;c++) {
        line[c] = fgetc(stdin);

        if (line[c] == '\n' || line[c] == '\0')
        {
            	line[c+1] = '\0';
            	break;
        }
        else if (line[c] == 128 && c > 0)
        {
        	c--;
        }
    }
    c=d=0;

    while (line[c] != '\0')
    {
    	if (line[c] == ' ')
    	{
    		userInput[argN][d] = '\0';
    		argN++;
    		d = 0;
    	}
    	else
    	{
    		userInput[argN][d++] = line[c];
    	}

    	c++;
    }



    return;
}

static void Debugger_seg_returnAddr(const code_segment_data_t* segmentData, uint32_t val, code_seg_t* CurrentCodeSeg)
{
	int x;

	if (!CurrentCodeSeg->MIPSReturnRegister)
	{
		printf("Seg  0x%08x  \t0x%08x\tblock type: %d\n"
				"next segments:\n"
				" (1) 0x%08x  \t0x%08x\n"
				" (2) 0x%08x  \t0x%08x\n"
				, (uint32_t)CurrentCodeSeg, (uint32_t)CurrentCodeSeg->MIPScode, CurrentCodeSeg->blockType
				, (uint32_t)CurrentCodeSeg->pCodeSegmentTargets[0], (uint32_t)CurrentCodeSeg->pCodeSegmentTargets[0]->MIPScode
				, (uint32_t)CurrentCodeSeg->pCodeSegmentTargets[1], (uint32_t)CurrentCodeSeg->pCodeSegmentTargets[1]->MIPScode);
	}
	else
	{
		printf("Seg  0x%08x  \t0x%08x\tblock type: %d\nnext segments:\n"
				, (uint32_t)CurrentCodeSeg, (uint32_t)CurrentCodeSeg->MIPScode, CurrentCodeSeg->blockType);

		code_seg_t*  tempCodeSeg=segmentData->FirstSegment;
		x=1;
		while (tempCodeSeg !=NULL)
		{
			if (tempCodeSeg->pCodeSegmentTargets[0] == CurrentCodeSeg)
			{
				printf(" (%d) 0x%08x  \t0x%08x\n", x, (uint32_t)tempCodeSeg->pCodeSegmentTargets[0], (uint32_t)tempCodeSeg->pCodeSegmentTargets[0]->MIPScode);
				x++;
			}
			else if (tempCodeSeg->pCodeSegmentTargets[1] == CurrentCodeSeg)
			{
				printf(" (%d) 0x%08x  \t0x%08x\n", x, (uint32_t)tempCodeSeg->pCodeSegmentTargets[1], (uint32_t)tempCodeSeg->pCodeSegmentTargets[1]->MIPScode);
				x++;
			}

			tempCodeSeg = tempCodeSeg->nextCodeSegmentLinkedList;
		}
	}
}



static int Debugger_print(const code_segment_data_t* segmentData)
{
	printf(HELP_PRINT);
	return 1;
}

static int Debugger_seg(const code_segment_data_t* segmentData)
{
	int val;
	int x=0;
	char *tailPointer;
	code_seg_t* tempCodeSeg;

	val = strtoul(userInput[1], &tailPointer, 0);

	if (!strncmp(userInput[1], "\n", 1))
	{
		printf("First Segment at 0x%x, number of segments %d\n", (uint32_t)segmentData->FirstSegment, segmentData->count);

		Debugger_seg_returnAddr(segmentData, val, CurrentCodeSeg);

		return 0;
	}

	if (!strncmp(userInput[1], "arm", 3))
	{
		printf("\naddr 0x%x, len %d\n"
				, (uint32_t)CurrentCodeSeg->ARMcode, CurrentCodeSeg->ARMcodeLen );
	}
	else if (!strncmp(userInput[1], "mips", 4))
	{
		for (x=0; x< CurrentCodeSeg->MIPScodeLen; x++)
		{
			ops_decode((x)*4, *(CurrentCodeSeg->MIPScode + x));
		}

		printf("\naddr 0x%x, len %d, return reg %d\n"
				"0x%08X %08X %02X (%d)\n"
				, (uint32_t)CurrentCodeSeg->MIPScode, CurrentCodeSeg->MIPScodeLen, CurrentCodeSeg->MIPSReturnRegister
				, CurrentCodeSeg->MIPSRegistersUsed[0], CurrentCodeSeg->MIPSRegistersUsed[1], CurrentCodeSeg->MIPSRegistersUsed[2]
				, CurrentCodeSeg->MIPSRegistersUsedCount);
	}
	else
	{
		if (!CurrentCodeSeg->MIPSReturnRegister)
		{
			int ok = 0;
			if (0 == val)
			{
				CurrentCodeSeg = segmentData->FirstSegment;
			}
			else if (1 == val)
			{
				CurrentCodeSeg = CurrentCodeSeg->pCodeSegmentTargets[0];
				ok = 1;
			}
			else if (2 == val && CurrentCodeSeg->pCodeSegmentTargets[1] != 0)
			{
				CurrentCodeSeg = CurrentCodeSeg->pCodeSegmentTargets[1];
				ok = 1;
			}
			else
			{
				tempCodeSeg=segmentData->FirstSegment;

				while (tempCodeSeg != NULL)
				{
					if ((uint32_t)tempCodeSeg == val || (uint32_t)tempCodeSeg->MIPScode == val)
					{
						CurrentCodeSeg = tempCodeSeg;
						ok = 1;
						break;
					}

					tempCodeSeg = tempCodeSeg->nextCodeSegmentLinkedList;
				}
			}

			if (!ok) printf("Invalid entry\n");
		}
		else
		{
			tempCodeSeg=segmentData->FirstSegment;
			x=1;
			while (tempCodeSeg != NULL)
			{
				if (tempCodeSeg == val)
				{
					CurrentCodeSeg = val;
					break;
				}
				else if (tempCodeSeg->pCodeSegmentTargets[0] == CurrentCodeSeg)
				{
					if (x == val)
					{
						CurrentCodeSeg = tempCodeSeg->pCodeSegmentTargets[0];
						break;
					}
					x++;
				}
				else if (tempCodeSeg->pCodeSegmentTargets[1] == CurrentCodeSeg)
				{
					if (x == val)
					{
						CurrentCodeSeg = tempCodeSeg->pCodeSegmentTargets[1];
						break;
					}
					x++;
				}

				tempCodeSeg = tempCodeSeg->nextCodeSegmentLinkedList;
			}
		}

		Debugger_seg_returnAddr(segmentData, val, CurrentCodeSeg);
	}

	return 0;

}


void Debugger_start(code_segment_data_t* segmentData)
{
	uint32_t quit = 0;

	//find segment
	CurrentCodeSeg = segmentData->FirstSegment;

	while (!quit)
	{
		printf("> "); fflush(stdin);
		getCmd();

		if (!strncmp(userInput[0], "quit", 4))
		{
			quit = 1;
		}
		else if (!strncmp(userInput[0], "print", 5))
		{
			Debugger_print(segmentData);
		}
		else if (!strncmp(userInput[0], "seg", 3))
		{
			Debugger_seg(segmentData);
		}
		else if (!strncmp(userInput[0], "help", 4))
		{
			printf(HELP_GEN);
		}
		else
		{
			printf("unknown command: %s", userInput[0]);
		}



	}

}
