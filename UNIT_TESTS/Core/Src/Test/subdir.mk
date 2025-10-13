################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/Test/command_handler.c \
../Core/Src/Test/test_collision.c \
../Core/Src/Test/test_obstacles.c \
../Core/Src/Test/test_sdcard.c 

OBJS += \
./Core/Src/Test/command_handler.o \
./Core/Src/Test/test_collision.o \
./Core/Src/Test/test_obstacles.o \
./Core/Src/Test/test_sdcard.o 

C_DEPS += \
./Core/Src/Test/command_handler.d \
./Core/Src/Test/test_collision.d \
./Core/Src/Test/test_obstacles.d \
./Core/Src/Test/test_sdcard.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/Test/%.o Core/Src/Test/%.su Core/Src/Test/%.cyclo: ../Core/Src/Test/%.c Core/Src/Test/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DRUN_UNIT_TESTS -DDEBUG -DSTM32U545xx -DUSE_NUCLEO_64 -DUSE_HAL_DRIVER -c -I../Core/Inc -I../Drivers/STM32U5xx_HAL_Driver/Inc -I../Drivers/STM32U5xx_HAL_Driver/Inc/Legacy -I../Drivers/BSP/STM32U5xx_Nucleo -I../Drivers/CMSIS/Device/ST/STM32U5xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src-2f-Test

clean-Core-2f-Src-2f-Test:
	-$(RM) ./Core/Src/Test/command_handler.cyclo ./Core/Src/Test/command_handler.d ./Core/Src/Test/command_handler.o ./Core/Src/Test/command_handler.su ./Core/Src/Test/test_collision.cyclo ./Core/Src/Test/test_collision.d ./Core/Src/Test/test_collision.o ./Core/Src/Test/test_collision.su ./Core/Src/Test/test_obstacles.cyclo ./Core/Src/Test/test_obstacles.d ./Core/Src/Test/test_obstacles.o ./Core/Src/Test/test_obstacles.su ./Core/Src/Test/test_sdcard.cyclo ./Core/Src/Test/test_sdcard.d ./Core/Src/Test/test_sdcard.o ./Core/Src/Test/test_sdcard.su

.PHONY: clean-Core-2f-Src-2f-Test

