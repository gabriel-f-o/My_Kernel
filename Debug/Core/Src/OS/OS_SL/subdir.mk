################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/OS/OS_SL/OS_sl.c \
../Core/Src/OS/OS_SL/OS_sl_db.c 

OBJS += \
./Core/Src/OS/OS_SL/OS_sl.o \
./Core/Src/OS/OS_SL/OS_sl_db.o 

C_DEPS += \
./Core/Src/OS/OS_SL/OS_sl.d \
./Core/Src/OS/OS_SL/OS_sl_db.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/OS/OS_SL/OS_sl.o: ../Core/Src/OS/OS_SL/OS_sl.c
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DUSE_HAL_DRIVER -DDEBUG -DSTM32F407xx -c -I../Drivers/CMSIS/Include -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Core/Inc -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -Os -ffunction-sections -fdata-sections -Wall -Wswitch-default -Wswitch-enum -Wconversion -fstack-usage -MMD -MP -MF"Core/Src/OS/OS_SL/OS_sl.d" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Core/Src/OS/OS_SL/OS_sl_db.o: ../Core/Src/OS/OS_SL/OS_sl_db.c
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DUSE_HAL_DRIVER -DDEBUG -DSTM32F407xx -c -I../Drivers/CMSIS/Include -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Core/Inc -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -Os -ffunction-sections -fdata-sections -Wall -Wswitch-default -Wswitch-enum -Wconversion -fstack-usage -MMD -MP -MF"Core/Src/OS/OS_SL/OS_sl_db.d" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

