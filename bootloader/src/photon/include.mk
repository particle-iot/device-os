INCLUDE_DIRS += $(BOOTLOADER_MODULE_PATH)/src/photon
INCLUDE_DIRS += $(BOOTLOADER_MODULE_PATH)/../hal/src/photon/wiced/platform/MCU
INCLUDE_DIRS += $(BOOTLOADER_MODULE_PATH)/../hal/src/photon/wiced/platform/MCU/STM32F2xx/WAF

CFLAGS += -fno-builtin-memcpy
LDFLAGS += -fno-builtin-memcpy

include $(BOOTLOADER_MODULE_PATH)/src/stm32f2xx/include.mk
