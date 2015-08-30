BOOTLOADER_SRC_COREV2_PATH = $(BOOTLOADER_MODULE_PATH)/src/electron
include $(BOOTLOADER_MODULE_PATH)/src/stm32f2xx/sources.mk

CSRC += $(call target_files,$(BOOTLOADER_MODULE_PATH)/../hal/src/electron/,dct_hal.c)
CSRC += $(call target_files,$(BOOTLOADER_MODULE_PATH)/../hal/src/electron/,watchdog_hal.c)
$(info $(CSRC))
