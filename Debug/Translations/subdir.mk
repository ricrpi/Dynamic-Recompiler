################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Translations/ALU.c \
../Translations/Branching.c \
../Translations/C_Interface.c \
../Translations/CleanUp.c \
../Translations/Constants.c \
../Translations/CountReg.c \
../Translations/Debug.c \
../Translations/FPU.c \
../Translations/Init.c \
../Translations/Literals.c \
../Translations/Memory.c \
../Translations/Misc.c \
../Translations/OptimizeARM.c \
../Translations/Registers.c \
../Translations/TLB.c \
../Translations/Translate.c 

OBJS += \
./Translations/ALU.o \
./Translations/Branching.o \
./Translations/C_Interface.o \
./Translations/CleanUp.o \
./Translations/Constants.o \
./Translations/CountReg.o \
./Translations/Debug.o \
./Translations/FPU.o \
./Translations/Init.o \
./Translations/Literals.o \
./Translations/Memory.o \
./Translations/Misc.o \
./Translations/OptimizeARM.o \
./Translations/Registers.o \
./Translations/TLB.o \
./Translations/Translate.o 

C_DEPS += \
./Translations/ALU.d \
./Translations/Branching.d \
./Translations/C_Interface.d \
./Translations/CleanUp.d \
./Translations/Constants.d \
./Translations/CountReg.d \
./Translations/Debug.d \
./Translations/FPU.d \
./Translations/Init.d \
./Translations/Literals.d \
./Translations/Memory.d \
./Translations/Misc.d \
./Translations/OptimizeARM.d \
./Translations/Registers.d \
./Translations/TLB.d \
./Translations/Translate.d 


# Each subdirectory must supply rules for building sources it contributes
Translations/%.o: ../Translations/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/richard/git/Dynamic-Recompiler" -I"/home/richard/git/Dynamic-Recompiler/InstructionSets" -I"/home/richard/git/Dynamic-Recompiler/Recompiler" -I"/home/richard/git/Dynamic-Recompiler/Recompiler/Debugger" -I"/home/richard/git/Dynamic-Recompiler/Translations" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


