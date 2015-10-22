#!/bin/sh

#  Dynamic-Recompiler - Turns MIPS code into ARM code
#  Original source: http://github.com/ricrpi/Dynamic-Recompiler
#  Copyright (C) 2015  Richard Hender
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.

READING=0

FOLDER="InstructionSets"
OUTPUT="InstructionSet_ascii.h"
INPUT="InstructionSet.h"
ENUM_DECL="_Instruction_e"

ARCH_I="MIPS"
ARCH_O="ARM"

#set -x
set -e

cd $FOLDER

echo "#ifndef INSTRUCTIONSET_ASCC" > $OUTPUT
echo "#define INSTRUCTIONSET_ASCC" >> $OUTPUT
echo "" >> $OUTPUT
echo "#include \"InstructionSet.h\"" >> $OUTPUT
echo "" >> $OUTPUT
echo "static const char* Instruction_ascii[sizeof_mips_op_t+1] = {" >> $OUTPUT


# Using a simple state machine where the state is stored in $READING as:
#	State	Description
#	1	The specified typedef enum has been found but no '{' on line
#	2	The specified typedef enum has been found and '{' found 
#	3	Finished reading the typedef enum

while read line 
	do
		if [ "$line" = "typedef enum $ENUM_DECL {" ]; then
			READING=2
		elif [ "$line" = "typedef enum $ENUM_DECL" ]; then
			READING=1
		elif [ $READING -eq 1 -a "$line" = "{" ]; then
			READING=2
 		elif [ "$line" = "} Instruction_e;" ]; then
			READING=3
			break;
		elif [ $READING -eq 2 ]; then
			CMT=""
			# find any comments on the current line
			CMT=`echo "$line" | cut -s -d "/" --output-delimiter="" -f 2-`

			# find any code on the current line, excluding comments
			ENUM=`echo "$line" | cut -d "/" -f 1`

			# if the enumeration sets a value then drop the value part
			# add a comma to the end if it existed before
			if [ `echo "$ENUM" | grep -c "="` -ge 1 ]; then
				if [ `echo "$ENUM" | grep -c ","` -ge 1 ]; then
					ENUM=`echo "$ENUM" | cut -d "=" -f 1`","
				else
					ENUM=`echo "$ENUM" | cut -d "=" -f 1`
				fi
			fi

			#if the current line is not just a comma and has a length
			if [ "$ENUM" != "," -a  -n "$ENUM" ]; then

				# strip the machine ARCH and make the output lower case
				if [ `echo "$ENUM" | grep -c "${ARCH_O}_"` -eq 1 ]; then
					ENUM=`echo "$ENUM" | sed -e "s/${ARCH_O}_//" | tr A-Z a-z | sed -e 's/_lit//'`
				elif [ `echo "$ENUM" | grep -c "${ARCH_I}_"` -eq 1 ]; then
					ENUM=`echo "$ENUM" | sed -e "s/${ARCH_I}_//" | sed -e 's/_lit//'`			
				#elif [ `echo "$ENUM" | grep -c "DR_"` -eq 1 ]; then
				#	ENUM=`echo "$ENUM" | sed -e 's/DR_//' | sed -e 's/_lit//'`			
				fi

				ENUM=`echo "$ENUM" | sed -e 's/[A-Za-z0-9_]*/\"&\"/'`		
			fi

			# now write out the 'ascii' value
			if [ -n "$CMT" ]; then
				echo "\t$ENUM\t // $CMT" >> $OUTPUT
			else
				echo "\t$ENUM"		>> $OUTPUT
			fi			
		fi
	done < $INPUT
echo "};" >> $OUTPUT

echo "#endif" >> $OUTPUT
