################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/OS/OS_CLI/submenus/cli_flash.c \
../Core/Src/OS/OS_CLI/submenus/cli_lfs.c \
../Core/Src/OS/OS_CLI/submenus/cli_system.c \
../Core/Src/OS/OS_CLI/submenus/cli_task.c 

OBJS += \
./Core/Src/OS/OS_CLI/submenus/cli_flash.o \
./Core/Src/OS/OS_CLI/submenus/cli_lfs.o \
./Core/Src/OS/OS_CLI/submenus/cli_system.o \
./Core/Src/OS/OS_CLI/submenus/cli_task.o 

C_DEPS += \
./Core/Src/OS/OS_CLI/submenus/cli_flash.d \
./Core/Src/OS/OS_CLI/submenus/cli_lfs.d \
./Core/Src/OS/OS_CLI/submenus/cli_system.d \
./Core/Src/OS/OS_CLI/submenus/cli_task.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/OS/OS_CLI/submenus/cli_flash.o: ../Core/Src/OS/OS_CLI/submenus/cli_flash.c
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DUSE_HAL_DRIVER -DDEBUG -DSTM32F407xx -c -I../Drivers/CMSIS/Include -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Core/Inc -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -O0 -ffunction-sections -fdata-sections -Wall -Wswitch-default -Wswitch-enum -Wconversion -fstack-usage -MMD -MP -MF"Core/Src/OS/OS_CLI/submenus/cli_flash.d" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Core/Src/OS/OS_CLI/submenus/cli_lfs.o: ../Core/Src/OS/OS_CLI/submenus/cli_lfs.c
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DUSE_HAL_DRIVER -DDEBUG -DSTM32F407xx -c -I../Drivers/CMSIS/Include -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Core/Inc -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -O0 -ffunction-sections -fdata-sections -Wall -Wswitch-default -Wswitch-enum -Wconversion -fstack-usage -MMD -MP -MF"Core/Src/OS/OS_CLI/submenus/cli_lfs.d" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Core/Src/OS/OS_CLI/submenus/cli_system.o: ../Core/Src/OS/OS_CLI/submenus/cli_system.c
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DUSE_HAL_DRIVER -DDEBUG -DSTM32F407xx -c -I../Drivers/CMSIS/Include -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Core/Inc -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -O0 -ffunction-sections -fdata-sections -Wall -Wswitch-default -Wswitch-enum -Wconversion -fstack-usage -MMD -MP -MF"Core/Src/OS/OS_CLI/submenus/cli_system.d" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Core/Src/OS/OS_CLI/submenus/cli_task.o: ../Core/Src/OS/OS_CLI/submenus/cli_task.c
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DUSE_HAL_DRIVER -DDEBUG -DSTM32F407xx -c -I../Drivers/CMSIS/Include -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Core/Inc -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -O0 -ffunction-sections -fdata-sections -Wall -Wswitch-default -Wswitch-enum -Wconversion -fstack-usage -MMD -MP -MF"Core/Src/OS/OS_CLI/submenus/cli_task.d" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

