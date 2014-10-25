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
	gcc -I.. -I"../InstructionSets" -I"../Translations" -I"../Debugger" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


