################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../CodeSegments.c \
../main.c \
../r4300.c \
../rom.c \
../tlb.c 

OBJS += \
./CodeSegments.o \
./main.o \
./r4300.o \
./rom.o \
./tlb.o 

C_DEPS += \
./CodeSegments.d \
./main.d \
./r4300.d \
./rom.d \
./tlb.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I.. -I"../InstructionSets" -I"../Translations" -I"../Debugger" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


