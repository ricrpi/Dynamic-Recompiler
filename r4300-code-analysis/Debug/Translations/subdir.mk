################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Translations/ALU.c \
../Translations/Branching.c \
../Translations/C_Interface.c \
../Translations/Constants.c \
../Translations/CountReg.c \
../Translations/Debug.c \
../Translations/DelaySlot.c \
../Translations/FPU.c \
../Translations/Literals.c \
../Translations/Memory.c \
../Translations/MipsRegisters.c \
../Translations/OptimizeARM.c \
../Translations/Translate.c \
../Translations/Trap.c 

OBJS += \
./Translations/ALU.o \
./Translations/Branching.o \
./Translations/C_Interface.o \
./Translations/Constants.o \
./Translations/CountReg.o \
./Translations/Debug.o \
./Translations/DelaySlot.o \
./Translations/FPU.o \
./Translations/Literals.o \
./Translations/Memory.o \
./Translations/MipsRegisters.o \
./Translations/OptimizeARM.o \
./Translations/Translate.o \
./Translations/Trap.o 

C_DEPS += \
./Translations/ALU.d \
./Translations/Branching.d \
./Translations/C_Interface.d \
./Translations/Constants.d \
./Translations/CountReg.d \
./Translations/Debug.d \
./Translations/DelaySlot.d \
./Translations/FPU.d \
./Translations/Literals.d \
./Translations/Memory.d \
./Translations/MipsRegisters.d \
./Translations/OptimizeARM.d \
./Translations/Translate.d \
./Translations/Trap.d 


# Each subdirectory must supply rules for building sources it contributes
Translations/%.o: ../Translations/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I.. -I"../InstructionSets" -I"../Translations" -I"../Debugger" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


