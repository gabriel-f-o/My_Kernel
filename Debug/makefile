################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include Drivers/STM32F4xx_HAL_Driver/Src/subdir.mk
-include Core/Startup/subdir.mk
-include Core/Src/OS/OS_Core/subdir.mk
-include Core/Src/OS/OS_CLI/subdir.mk
-include Core/Src/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
EXECUTABLES += \
My_Kernel.elf \

SIZE_OUTPUT += \
default.size.stdout \

OBJDUMP_LIST += \
My_Kernel.list \

OBJCOPY_BIN += \
My_Kernel.bin \


# All Target
all: My_Kernel.elf secondary-outputs

# Tool invocations
My_Kernel.elf: $(OBJS) $(USER_OBJS) C:\Users\Gabriel\Documents\Embedded\ C\PROJECTS\My_Kernel\STM32F407VGTX_FLASH.ld
	arm-none-eabi-gcc -o "My_Kernel.elf" @"objects.list" $(USER_OBJS) $(LIBS) -mcpu=cortex-m4 -T"C:\Users\Gabriel\Documents\Embedded C\PROJECTS\My_Kernel\STM32F407VGTX_FLASH.ld" --specs=nosys.specs -Wl,-Map="My_Kernel.map" -Wl,--gc-sections -static --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -Wl,--start-group -lc -lm -Wl,--end-group
	@echo 'Finished building target: $@'
	@echo ' '

default.size.stdout: $(EXECUTABLES)
	arm-none-eabi-size  $(EXECUTABLES)
	@echo 'Finished building: $@'
	@echo ' '

My_Kernel.list: $(EXECUTABLES)
	arm-none-eabi-objdump -h -S $(EXECUTABLES) > "My_Kernel.list"
	@echo 'Finished building: $@'
	@echo ' '

My_Kernel.bin: $(EXECUTABLES)
	arm-none-eabi-objcopy  -O binary $(EXECUTABLES) "My_Kernel.bin"
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) *
	-@echo ' '

secondary-outputs: $(SIZE_OUTPUT) $(OBJDUMP_LIST) $(OBJCOPY_BIN)

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets
