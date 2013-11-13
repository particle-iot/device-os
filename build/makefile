
-include ../makefile.init

RM := rm -rf
CORE_FIRMWARE?=core-firmware
USER_DIR?=user

# All of the sources participating in the build are defined here
-include sources.mk
-include startup/subdir.mk
-include src/subdir.mk
-include ../$(USER_DIR)/subdir.mk
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
$(CORE_FIRMWARE).hex \

SECONDARY_LIST += \
$(CORE_FIRMWARE).lst \

SECONDARY_SIZE += \
$(CORE_FIRMWARE).siz \


# All Target
all: $(CORE_FIRMWARE).elf secondary-outputs

dependents:
	-cd ../../core-common-lib/build && $(MAKE) clean all
	-cd ../../core-communication-lib/build && $(MAKE) clean all

# Tool invocations
$(CORE_FIRMWARE).elf: $(OBJS) $(USER_OBJS) ../../core-common-lib/build/libcore-common-lib.a ../../core-communication-lib/build/libcore-communication-lib.a
	@echo 'Building target: $@'
	@echo 'Invoking: ARM GCC C++ Linker'
	arm-none-eabi-g++ -T"../linker/linker_stm32f10x_md_dfu.ld" -nostartfiles -Xlinker --gc-sections -o "$(CORE_FIRMWARE).elf" -L"../../core-common-lib/build" -L"../../core-communication-lib/build" -Wl,-Map,$(CORE_FIRMWARE).map -mcpu=cortex-m3 -mthumb -g3 -gdwarf-2 $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '
	$(MAKE) --no-print-directory post-build

$(CORE_FIRMWARE).hex: $(CORE_FIRMWARE).elf
	@echo 'Invoking: ARM GNU Create Flash Image'
	arm-none-eabi-objcopy -O ihex $(CORE_FIRMWARE).elf  "$(CORE_FIRMWARE).hex"
	@echo 'Finished building: $@'
	@echo ' '

$(CORE_FIRMWARE).lst: $(CORE_FIRMWARE).elf
	@echo 'Invoking: ARM GNU Create Listing'
	arm-none-eabi-objdump -h -S $(CORE_FIRMWARE).elf > "$(CORE_FIRMWARE).lst"
	@echo 'Finished building: $@'
	@echo ' '

$(CORE_FIRMWARE).siz: $(CORE_FIRMWARE).elf
	@echo 'Invoking: ARM GNU Print Size'
	arm-none-eabi-size  --format=berkeley $(CORE_FIRMWARE).elf
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(OBJS)$(C_DEPS)$(SECONDARY_FLASH)$(CXX_DEPS)$(S_UPPER_DEPS)$(SECONDARY_LIST)$(SECONDARY_SIZE)$(C++_DEPS)$(ASM_DEPS)$(CC_DEPS)$(CPP_DEPS)$(EXECUTABLES)$(C_UPPER_DEPS) $(CORE_FIRMWARE).elf $(CORE_FIRMWARE).map $(CORE_FIRMWARE).bin
	-@echo ' '

post-build:
	-@echo 'Create Binary'
	-arm-none-eabi-objcopy -S -O binary "$(CORE_FIRMWARE).elf" "$(CORE_FIRMWARE).bin"
	-@echo ' '

secondary-outputs: $(SECONDARY_FLASH) $(SECONDARY_LIST) $(SECONDARY_SIZE)

.PHONY: all clean dependents
.SECONDARY: post-build
../../core-common-lib/build/libcore-common-lib.a:
../../core-communication-lib/build/libcore-communication-lib.a:

-include ../makefile.targets