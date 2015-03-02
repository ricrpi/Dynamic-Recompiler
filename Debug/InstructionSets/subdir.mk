################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../InstructionSets/InstructionSet.c \
../InstructionSets/InstructionSetARM6hf.c \
../InstructionSets/InstructionSetMIPS4.c 

OBJS += \
./InstructionSets/InstructionSet.o \
./InstructionSets/InstructionSetARM6hf.o \
./InstructionSets/InstructionSetMIPS4.o 

C_DEPS += \
./InstructionSets/InstructionSet.d \
./InstructionSets/InstructionSetARM6hf.d \
./InstructionSets/InstructionSetMIPS4.d 


# Each subdirectory must supply rules for building sources it contributes
InstructionSets/%.o: ../InstructionSets/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/rjhender/git/R4300-code-analysis" -I"/home/rjhender/git/R4300-code-analysis/InstructionSets" -I"/home/rjhender/git/R4300-code-analysis/Recompiler" -I"/home/rjhender/git/R4300-code-analysis/Recompiler/Debugger" -I"/home/rjhender/git/R4300-code-analysis/Translations" -I"/home/rjhender/git/R4300-code-analysis/InstructionSets" -I/home/rjhender/rpi/tools/arm-bcm2708/arm-bcm2708hardfp-linux-gnueabi/arm-bcm2708hardfp-linux-gnueabi/sysroot/usr/include -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


