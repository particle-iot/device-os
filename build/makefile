########################################
#   @author  Spark Application Team    #
#   @version V1.0.0                    #
#   @date    20-June-2013              #
########################################

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
usb-bootloader.hex \

SECONDARY_LIST += \
usb-bootloader.lst \

SECONDARY_SIZE += \
usb-bootloader.siz \


# All Target
all: usb-bootloader.elf secondary-outputs

# Tool invocations
usb-bootloader.elf: $(OBJS) $(USER_OBJS)
	@echo 'Building target..'
	arm-none-eabi-gcc -T"../linker/linker_stm32f10x_md.ld" -nostartfiles -Xlinker --gc-sections -Wl,-Map,usb-bootloader.map -mcpu=cortex-m3 -mthumb -g3 -gdwarf-2 -o "usb-bootloader.elf" $(OBJS) $(USER_OBJS) $(LIBS)
	$(MAKE) --no-print-directory post-build

usb-bootloader.hex: usb-bootloader.elf
	arm-none-eabi-objcopy -O ihex usb-bootloader.elf  "usb-bootloader.hex"

usb-bootloader.lst: usb-bootloader.elf
	arm-none-eabi-objdump -h -S usb-bootloader.elf > "usb-bootloader.lst"

usb-bootloader.siz: usb-bootloader.elf
	arm-none-eabi-size  --format=berkeley usb-bootloader.elf
	@echo 'Finished building target'

# Other Targets
clean:
	-$(RM) $(OBJS)$(C_DEPS)$(ASM_DEPS)$(S_UPPER_DEPS)$(EXECUTABLES)$(SECONDARY_FLASH)$(SECONDARY_LIST)$(SECONDARY_SIZE) usb-bootloader.map usb-bootloader.elf usb-bootloader.bin

post-build:
	-arm-none-eabi-objcopy -S -O binary "usb-bootloader.elf" "usb-bootloader.bin"

secondary-outputs: $(SECONDARY_FLASH) $(SECONDARY_LIST) $(SECONDARY_SIZE)

.PHONY: all clean dependents
.SECONDARY: post-build

-include ../makefile.targets
