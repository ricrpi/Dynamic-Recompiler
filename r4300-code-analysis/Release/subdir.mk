################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../CodeSegments.c \
../Debugger.c \
../InstructionSet.c \
../InstructionSetARM6hf.c \
../InstructionSetMIPS4.c \
../Translate.c \
../main.c \
../rom.c 

OBJS += \
./CodeSegments.o \
./Debugger.o \
./InstructionSet.o \
./InstructionSetARM6hf.o \
./InstructionSetMIPS4.o \
./Translate.o \
./main.o \
./rom.o 

C_DEPS += \
./CodeSegments.d \
./Debugger.d \
./InstructionSet.d \
./InstructionSetARM6hf.d \
./InstructionSetMIPS4.d \
./Translate.d \
./main.d \
./rom.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/rjhender/git/R4300-code-analysis/r4300-code-analysis" -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


