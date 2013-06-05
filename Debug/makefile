################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include startup/subdir.mk
-include src/subdir.mk
-include libraries/STM32_USB-FS-Device_Driver/src/subdir.mk
-include libraries/STM32F10x_StdPeriph_Driver/src/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
ifneq ($(strip $(ASM_DEPS)),)
-include $(ASM_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
SECONDARY_FLASH += \
usb-dfu.hex \

SECONDARY_LIST += \
usb-dfu.lst \

SECONDARY_SIZE += \
usb-dfu.siz \


# All Target
all: usb-dfu.elf secondary-outputs

# Tool invocations
usb-dfu.elf: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: ARM Sourcery Mac OS X GCC C Linker'
	arm-none-eabi-gcc -T"/Users/zac/code/spark/cc3000-patch-programmer/linker/linker_stm32f10x_md.ld" -nostartfiles -Xlinker --gc-sections -Wl,-Map,usb-dfu.map -mcpu=cortex-m3 -mthumb -g3 -gdwarf-2 -o "usb-dfu.elf" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

usb-dfu.hex: usb-dfu.elf
	@echo 'Invoking: ARM Sourcery Mac OS X GNU Create Flash Image'
	arm-none-eabi-objcopy -O binary usb-dfu.elf  usb-dfu.bin
	@echo 'Finished building: $@'
	@echo ' '

usb-dfu.lst: usb-dfu.elf
	@echo 'Invoking: ARM Sourcery Mac OS X GNU Create Listing'
	arm-none-eabi-objdump -h -S usb-dfu.elf > "usb-dfu.lst"
	@echo 'Finished building: $@'
	@echo ' '

usb-dfu.siz: usb-dfu.elf
	@echo 'Invoking: ARM Sourcery Mac OS X GNU Print Size'
	arm-none-eabi-size  --format=berkeley usb-dfu.elf
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(SECONDARY_SIZE)$(OBJS)$(C_DEPS)$(ASM_DEPS)$(SECONDARY_FLASH)$(EXECUTABLES)$(SECONDARY_LIST)$(S_UPPER_DEPS) usb-dfu.elf
	-@echo ' '

secondary-outputs: $(SECONDARY_FLASH) $(SECONDARY_LIST) $(SECONDARY_SIZE)

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets
