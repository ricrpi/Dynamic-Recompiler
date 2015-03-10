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
