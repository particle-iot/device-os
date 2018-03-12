BOOTLOADER_SRC_STM32F2XX_PATH = $(BOOTLOADER_MODULE_PATH)/src/stm32f2xx

CSRC += $(call target_files,$(BOOTLOADER_SRC_STM32F2XX_PATH)/,*.c)

ifeq ("$(PLATFORM_ID)","88")
LDFLAGS += -T$(COMMON_BUILD)/arm/linker/linker_$(STM32_DEVICE_LC)_bootloader_ext.ld
LINKER_DEPS += $(COMMON_BUILD)/arm/linker/linker_$(STM32_DEVICE_LC)_bootloader_ext.ld
else
LDFLAGS += -T$(COMMON_BUILD)/arm/linker/linker_$(STM32_DEVICE_LC)_bootloader.ld
LINKER_DEPS += $(COMMON_BUILD)/arm/linker/linker_$(STM32_DEVICE_LC)_bootloader.ld
endif

include $(BOOTLOADER_MODULE_PATH)/src/stm32/sources.mk
