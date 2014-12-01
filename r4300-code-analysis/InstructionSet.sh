#!/bin/sh

READING=0

FOLDER="InstructionSets"
OUTPUT="InstructionSet_ascii.h"
INPUT="InstructionSet.h"
ENUM_DECL="_Instruction_e"

#set -x
set -e

cd $FOLDER

echo "#include \"InstructionSet.h\"" > $OUTPUT
echo "#ifndef INSTRUCTIONSET_ASCC" >> $OUTPUT
echo "#define INSTRUCTIONSET_ASCC" >> $OUTPUT
echo "static const char* Instruction_ascii[sizeof_mips_op_t+1] = {" >> $OUTPUT

while read line 
	do
		if [ "$line" = "typedef enum $ENUM_DECL {" ]; then
			READING=2
		elif [ "$line" = "typedef enum $ENUM_DECL" ]; then
			READING=1
		elif [ $READING -eq 1 -a "$line" = "{" ]; then
			READING=2
 		elif [ "$line" = "} Instruction_e;" ]; then
			break;
		elif [ $READING -eq 2 ]; then
			CMT=""
			CMT=`echo "$line" | cut -s -d "/" --output-delimiter="" -f 2-`
			ENUM=`echo "$line" | cut -d "/" -f 1`

			if [ `echo "$ENUM" | grep -c "="` -ge 1 ]; then
				if [ `echo "$ENUM" | grep -c ","` -ge 1 ]; then
					ENUM=`echo "$ENUM" | cut -d "=" -f 1`","
				else
					ENUM=`echo "$ENUM" | cut -d "=" -f 1`
				fi
			fi

			if [ "$ENUM" != "," -a  -n "$ENUM" ]; then
				if [ `echo "$ENUM" | grep -c "ARM_"` -eq 1 ]; then
					ENUM=`echo "$ENUM" | sed -e 's/ARM_//' | tr A-Z a-z | sed -e 's/_lit//'`
				fi
				ENUM=`echo "$ENUM" | sed -e 's/[A-Za-z0-9_]*/\"&\"/'`		
			fi

			if [ -n "$CMT" ]; then
				echo "$ENUM // $CMT" >> $OUTPUT
			else
				echo "$ENUM"		>> $OUTPUT
			fi			
		fi
	done < $INPUT
echo "};" >> $OUTPUT

echo "#endif" >> $OUTPUT
