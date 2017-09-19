BOOTLOADER_SRC_COREV1_PATH = $(BOOTLOADER_MODULE_PATH)/src/core

CSRC += $(call target_files,$(BOOTLOADER_SRC_COREV1_PATH)/,*.c)

LDFLAGS += -T$(COMMON_BUILD)/arm/linker/linker_$(STM32_DEVICE_LC).ld
LINKER_DEPS += $(COMMON_BUILD)/arm/linker/linker_$(STM32_DEVICE_LC).ld

include $(BOOTLOADER_MODULE_PATH)/src/stm32/sources.mk
