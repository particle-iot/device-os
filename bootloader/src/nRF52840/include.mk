INCLUDE_DIRS += $(BOOTLOADER_MODULE_PATH)/src/nRF52840

CFLAGS += -fno-builtin-memcpy -fno-builtin-memcmp -fno-builtin-memset
LDFLAGS += -fno-builtin-memcpy -fno-builtin-memcmp -fno-builtin-memset

LDFLAGS += -L$(COMMON_BUILD)/arm/linker/nrf52840
