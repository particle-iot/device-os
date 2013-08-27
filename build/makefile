
-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include startup/subdir.mk
-include src/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C++_DEPS)),)
-include $(C++_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
ifneq ($(strip $(ASM_DEPS)),)
-include $(ASM_DEPS)
endif
ifneq ($(strip $(CC_DEPS)),)
-include $(CC_DEPS)
endif
ifneq ($(strip $(CPP_DEPS)),)
-include $(CPP_DEPS)
endif
ifneq ($(strip $(CXX_DEPS)),)
-include $(CXX_DEPS)
endif
ifneq ($(strip $(C_UPPER_DEPS)),)
-include $(C_UPPER_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
SECONDARY_FLASH += \
core-firmware.hex \

SECONDARY_LIST += \
core-firmware.lst \

SECONDARY_SIZE += \
core-firmware.siz \


# All Target
all: core-firmware.elf secondary-outputs

dependents:
	-cd ../../core-common-lib/build && $(MAKE) clean all

# Tool invocations
core-firmware.elf: $(OBJS) $(USER_OBJS) ../../core-common-lib/build/libcore-common-lib.a
	@echo 'Building target: $@'
	@echo 'Invoking: ARM Sourcery Windows GCC C++ Linker'
	arm-none-eabi-g++ -T"../linker/linker_stm32f10x_md_dfu.ld" -nostartfiles -Xlinker --gc-sections -L"../../core-common-lib/build" -Wl,-Map,core-firmware.map -mcpu=cortex-m3 -mthumb -g3 -gdwarf-2 -o "core-firmware.elf" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '
	$(MAKE) --no-print-directory post-build

core-firmware.hex: core-firmware.elf
	@echo 'Invoking: ARM Sourcery Windows GNU Create Flash Image'
	arm-none-eabi-objcopy -O ihex core-firmware.elf  "core-firmware.hex"
	@echo 'Finished building: $@'
	@echo ' '

core-firmware.lst: core-firmware.elf
	@echo 'Invoking: ARM Sourcery Windows GNU Create Listing'
	arm-none-eabi-objdump -h -S core-firmware.elf > "core-firmware.lst"
	@echo 'Finished building: $@'
	@echo ' '

core-firmware.siz: core-firmware.elf
	@echo 'Invoking: ARM Sourcery Windows GNU Print Size'
	arm-none-eabi-size  --format=berkeley core-firmware.elf
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(OBJS)$(C_DEPS)$(SECONDARY_FLASH)$(CXX_DEPS)$(S_UPPER_DEPS)$(SECONDARY_LIST)$(SECONDARY_SIZE)$(C++_DEPS)$(ASM_DEPS)$(CC_DEPS)$(CPP_DEPS)$(EXECUTABLES)$(C_UPPER_DEPS) core-firmware.elf core-firmware.map core-firmware.bin
	-@echo ' '

post-build:
	-@echo 'Create Binary'
	-arm-none-eabi-objcopy -S -O binary "core-firmware.elf" "core-firmware.bin"
	-@echo ' '

secondary-outputs: $(SECONDARY_FLASH) $(SECONDARY_LIST) $(SECONDARY_SIZE)

.PHONY: all clean dependents
.SECONDARY: post-build
../../core-common-lib/build/libcore-common-lib.a:

-include ../makefile.targets
