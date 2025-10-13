################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/SDCard/game_storage.c \
../Core/Src/SDCard/sd_card.c 

OBJS += \
./Core/Src/SDCard/game_storage.o \
./Core/Src/SDCard/sd_card.o 

C_DEPS += \
./Core/Src/SDCard/game_storage.d \
./Core/Src/SDCard/sd_card.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/SDCard/%.o Core/Src/SDCard/%.su Core/Src/SDCard/%.cyclo: ../Core/Src/SDCard/%.c Core/Src/SDCard/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DRUN_UNIT_TESTS -DDEBUG -DSTM32U545xx -DUSE_NUCLEO_64 -DUSE_HAL_DRIVER -c -I../Core/Inc -I../Drivers/STM32U5xx_HAL_Driver/Inc -I../Drivers/STM32U5xx_HAL_Driver/Inc/Legacy -I../Drivers/BSP/STM32U5xx_Nucleo -I../Drivers/CMSIS/Device/ST/STM32U5xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src-2f-SDCard

clean-Core-2f-Src-2f-SDCard:
	-$(RM) ./Core/Src/SDCard/game_storage.cyclo ./Core/Src/SDCard/game_storage.d ./Core/Src/SDCard/game_storage.o ./Core/Src/SDCard/game_storage.su ./Core/Src/SDCard/sd_card.cyclo ./Core/Src/SDCard/sd_card.d ./Core/Src/SDCard/sd_card.o ./Core/Src/SDCard/sd_card.su

.PHONY: clean-Core-2f-Src-2f-SDCard

