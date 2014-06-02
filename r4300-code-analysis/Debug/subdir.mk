################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../CodeSegments.c \
../Debugger.c \
../InstructionSetARM6hf.c \
../InstructionSetMIPS4.c \
../Optimize.c \
../main.c \
../rom.c 

OBJS += \
./CodeSegments.o \
./Debugger.o \
./InstructionSetARM6hf.o \
./InstructionSetMIPS4.o \
./Optimize.o \
./main.o \
./rom.o 

C_DEPS += \
./CodeSegments.d \
./Debugger.d \
./InstructionSetARM6hf.d \
./InstructionSetMIPS4.d \
./Optimize.d \
./main.d \
./rom.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


