################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Debugger/Debugger.c 

OBJS += \
./Debugger/Debugger.o 

C_DEPS += \
./Debugger/Debugger.d 


# Each subdirectory must supply rules for building sources it contributes
Debugger/%.o: ../Debugger/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/rjhender/git/R4300-code-analysis" -I"/home/rjhender/git/R4300-code-analysis/Debugger" -I"/home/rjhender/git/R4300-code-analysis/InstructionSets" -I"/home/rjhender/git/R4300-code-analysis/Translations" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


