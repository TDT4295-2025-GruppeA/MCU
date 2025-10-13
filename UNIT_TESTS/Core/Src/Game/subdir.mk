################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/Game/collision.c \
../Core/Src/Game/game.c \
../Core/Src/Game/input.c \
../Core/Src/Game/obstacles.c \
../Core/Src/Game/shapes.c \
../Core/Src/Game/spi_protocol.c 

OBJS += \
./Core/Src/Game/collision.o \
./Core/Src/Game/game.o \
./Core/Src/Game/input.o \
./Core/Src/Game/obstacles.o \
./Core/Src/Game/shapes.o \
./Core/Src/Game/spi_protocol.o 

C_DEPS += \
./Core/Src/Game/collision.d \
./Core/Src/Game/game.d \
./Core/Src/Game/input.d \
./Core/Src/Game/obstacles.d \
./Core/Src/Game/shapes.d \
./Core/Src/Game/spi_protocol.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/Game/%.o Core/Src/Game/%.su Core/Src/Game/%.cyclo: ../Core/Src/Game/%.c Core/Src/Game/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DRUN_UNIT_TESTS -DDEBUG -DSTM32U545xx -DUSE_NUCLEO_64 -DUSE_HAL_DRIVER -c -I../Core/Inc -I../Drivers/STM32U5xx_HAL_Driver/Inc -I../Drivers/STM32U5xx_HAL_Driver/Inc/Legacy -I../Drivers/BSP/STM32U5xx_Nucleo -I../Drivers/CMSIS/Device/ST/STM32U5xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src-2f-Game

clean-Core-2f-Src-2f-Game:
	-$(RM) ./Core/Src/Game/collision.cyclo ./Core/Src/Game/collision.d ./Core/Src/Game/collision.o ./Core/Src/Game/collision.su ./Core/Src/Game/game.cyclo ./Core/Src/Game/game.d ./Core/Src/Game/game.o ./Core/Src/Game/game.su ./Core/Src/Game/input.cyclo ./Core/Src/Game/input.d ./Core/Src/Game/input.o ./Core/Src/Game/input.su ./Core/Src/Game/obstacles.cyclo ./Core/Src/Game/obstacles.d ./Core/Src/Game/obstacles.o ./Core/Src/Game/obstacles.su ./Core/Src/Game/shapes.cyclo ./Core/Src/Game/shapes.d ./Core/Src/Game/shapes.o ./Core/Src/Game/shapes.su ./Core/Src/Game/spi_protocol.cyclo ./Core/Src/Game/spi_protocol.d ./Core/Src/Game/spi_protocol.o ./Core/Src/Game/spi_protocol.su

.PHONY: clean-Core-2f-Src-2f-Game

