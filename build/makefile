
-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include startup/subdir.mk
-include src/subdir.mk
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
usb-bootloader.hex \

SECONDARY_LIST += \
usb-bootloader.lst \

SECONDARY_SIZE += \
usb-bootloader.siz \


# All Target
all: usb-bootloader.elf secondary-outputs

# Tool invocations
usb-bootloader.elf: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: ARM Sourcery Windows GCC C Linker'
	arm-none-eabi-gcc -T"../linker/linker_stm32f10x_md.ld" -nostartfiles -Xlinker --gc-sections -L"../../core-common-lib/build" -Wl,-Map,usb-bootloader.map -mcpu=cortex-m3 -mthumb -g3 -gdwarf-2 -o "usb-bootloader.elf" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '
	$(MAKE) --no-print-directory post-build

usb-bootloader.hex: usb-bootloader.elf
	@echo 'Invoking: ARM Sourcery Windows GNU Create Flash Image'
	arm-none-eabi-objcopy -O ihex usb-bootloader.elf  "usb-bootloader.hex"
	@echo 'Finished building: $@'
	@echo ' '

usb-bootloader.lst: usb-bootloader.elf
	@echo 'Invoking: ARM Sourcery Windows GNU Create Listing'
	arm-none-eabi-objdump -h -S usb-bootloader.elf > "usb-bootloader.lst"
	@echo 'Finished building: $@'
	@echo ' '

usb-bootloader.siz: usb-bootloader.elf
	@echo 'Invoking: ARM Sourcery Windows GNU Print Size'
	arm-none-eabi-size  --format=berkeley usb-bootloader.elf
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(SECONDARY_SIZE)$(OBJS)$(C_DEPS)$(ASM_DEPS)$(SECONDARY_FLASH)$(EXECUTABLES)$(SECONDARY_LIST)$(S_UPPER_DEPS) usb-bootloader.elf usb-bootloader.map usb-bootloader.bin
	-@echo ' '

post-build:
	-@echo 'Create Binary'
	-arm-none-eabi-objcopy -S -O binary "usb-bootloader.elf" "usb-bootloader.bin"
	-@echo ' '

secondary-outputs: $(SECONDARY_FLASH) $(SECONDARY_LIST) $(SECONDARY_SIZE)

.PHONY: all clean dependents
.SECONDARY: post-build

-include ../makefile.targets
