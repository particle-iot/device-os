INCLUDE_DIRS += $(BOOTLOADER_MODULE_PATH)/src/electron
include $(BOOTLOADER_MODULE_PATH)/src/stm32f2xx/include.mk

CFLAGS += -fno-builtin-memcpy -fno-builtin-memcmp -fno-builtin-memset
LDFLAGS += -fno-builtin-memcpy -fno-builtin-memcmp -fno-builtin-memset
