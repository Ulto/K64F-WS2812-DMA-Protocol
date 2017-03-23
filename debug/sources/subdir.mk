################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../sources/NEO.c \
../sources/board.c \
../sources/clock_config.c \
../sources/ftm_simple_pwm.c \
../sources/pin_mux.c 

OBJS += \
./sources/NEO.o \
./sources/board.o \
./sources/clock_config.o \
./sources/ftm_simple_pwm.o \
./sources/pin_mux.o 

C_DEPS += \
./sources/NEO.d \
./sources/board.d \
./sources/clock_config.d \
./sources/ftm_simple_pwm.d \
./sources/pin_mux.d 


# Each subdirectory must supply rules for building sources it contributes
sources/%.o: ../sources/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -ffreestanding -fno-builtin -Wall  -g -DDEBUG -DCPU_MK64FN1M0VMD12 -DFRDM_K64F -DFREEDOM -I../CMSIS/Include -I../devices/gcc -I../devices -I../drivers -I../sources -I../utilities -std=gnu99 -mapcs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


