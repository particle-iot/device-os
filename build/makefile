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
-include libraries/CC3000_Host_Driver/subdir.mk
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
marvin.hex \

SECONDARY_LIST += \
marvin.lst \

SECONDARY_SIZE += \
marvin.siz \


# All Target
all: marvin.elf secondary-outputs

# Tool invocations
marvin.elf: $(OBJS) $(USER_OBJS)
	arm-none-eabi-gcc -T"../linker/linker_stm32f10x_md_dfu.ld" -nostartfiles -Xlinker --gc-sections -Wl,-Map,marvin.map -mcpu=cortex-m3 -mthumb -g3 -gdwarf-2 -o "marvin.elf" $(OBJS) $(USER_OBJS) $(LIBS)
	$(MAKE) --no-print-directory post-build

marvin.hex: marvin.elf
	arm-none-eabi-objcopy -O ihex marvin.elf  "marvin.hex"

marvin.lst: marvin.elf
	arm-none-eabi-objdump -h -S marvin.elf > "marvin.lst"

marvin.siz: marvin.elf
	arm-none-eabi-size  --format=berkeley marvin.elf

# Other Targets
clean:
	-$(RM) $(OBJS)$(C_DEPS)$(ASM_DEPS)$(S_UPPER_DEPS)$(EXECUTABLES)$(SECONDARY_FLASH)$(SECONDARY_LIST)$(SECONDARY_SIZE) marvin.map marvin.elf marvin.bin

post-build:
	-arm-none-eabi-objcopy -S -O binary "marvin.elf" "marvin.bin"

secondary-outputs: $(SECONDARY_FLASH) $(SECONDARY_LIST) $(SECONDARY_SIZE)

.PHONY: all clean dependents
.SECONDARY: post-build

-include ../makefile.targets
