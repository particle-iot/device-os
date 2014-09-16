
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

ifeq ("$(USE_SWD_JTAG)","y") 
CFLAGS += -DUSE_SWD_JTAG
endif

ifeq ("$(DEBUG_BUILD)","y") 
CFLAGS += -DDEBUG_BUILD
else
CFLAGS += -DRELEASE_BUILD
endif


# Add inputs and outputs from these tool invocations to the build variables 
SECONDARY_FLASH += \
bootloader.hex \

SECONDARY_LIST += \
bootloader.lst \

SECONDARY_SIZE += \
bootloader.siz \


# All Target
all: bootloader.elf secondary-outputs

dependents:
	-cd ../../common/build && $(MAKE) clean all

# Tool invocations
bootloader.elf: $(OBJS) $(USER_OBJS) ../../common/build/libcore-common-lib.a
	@echo 'Building target: $@'
	@echo 'Invoking: ARM Sourcery Windows GCC C Linker'
	arm-none-eabi-gcc -T"../linker/linker_stm32f10x_md.ld" -nostartfiles -Xlinker --gc-sections -L"../../common/build" -Wl,-Map,bootloader.map -mcpu=cortex-m3 -mthumb -g3 -gdwarf-2 -o "bootloader.elf" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '
	$(MAKE) --no-print-directory post-build

bootloader.hex: bootloader.elf
	@echo 'Invoking: ARM Sourcery Windows GNU Create Flash Image'
	arm-none-eabi-objcopy -O ihex bootloader.elf  "bootloader.hex"
	@echo 'Finished building: $@'
	@echo ' '

bootloader.lst: bootloader.elf
	@echo 'Invoking: ARM Sourcery Windows GNU Create Listing'
	arm-none-eabi-objdump -h -S bootloader.elf > "bootloader.lst"
	@echo 'Finished building: $@'
	@echo ' '

bootloader.siz: bootloader.elf
	@echo 'Invoking: ARM Sourcery Windows GNU Print Size'
	arm-none-eabi-size  --format=berkeley bootloader.elf
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(SECONDARY_SIZE)$(OBJS)$(C_DEPS)$(ASM_DEPS)$(SECONDARY_FLASH)$(EXECUTABLES)$(SECONDARY_LIST)$(S_UPPER_DEPS) bootloader.elf bootloader.map bootloader.bin
	-@echo ' '

post-build:
	-@echo 'Create Binary'
	-arm-none-eabi-objcopy -S -O binary "bootloader.elf" "bootloader.bin"
	-@echo ' '

secondary-outputs: $(SECONDARY_FLASH) $(SECONDARY_LIST) $(SECONDARY_SIZE)

.PHONY: all clean dependents
.SECONDARY: post-build
../../common/build/libcore-common-lib.a:

-include ../makefile.targets
