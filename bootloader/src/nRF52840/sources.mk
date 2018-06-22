BOOTLOADER_SRC_PATH = $(BOOTLOADER_MODULE_PATH)/src/nRF52840

CSRC += $(call target_files,$(BOOTLOADER_SRC_PATH)/,*.c)
CPPSRC += $(call target_files,$(BOOTLOADER_SRC_PATH)/,*.cpp)

CSRC += $(call target_files,$(BOOTLOADER_MODULE_PATH)/../hal/src/nRF52840/,flash_hal.c)
CPPSRC += $(call target_files,$(BOOTLOADER_MODULE_PATH)/../hal/src/nRF52840/,flash_common.cpp)
CSRC += $(call target_files,$(BOOTLOADER_MODULE_PATH)/../hal/src/nRF52840/,exflash_hal.c)
CSRC += $(call target_files,$(BOOTLOADER_MODULE_PATH)/../hal/src/nRF52840/,button_hal.c)
CSRC += $(call target_files,$(BOOTLOADER_MODULE_PATH)/../hal/src/nRF52840/,rgbled_hal.c)
CSRC += $(call target_files,$(BOOTLOADER_MODULE_PATH)/../hal/src/nRF52840/,watchdog_hal.c)
CPPSRC += $(call target_files,$(BOOTLOADER_MODULE_PATH)/../hal/src/nRF52840/,dct_hal.cpp)
# FIXME
CPPSRC += $(call target_files,$(BOOTLOADER_MODULE_PATH)/../hal/src/xenon/littlefs/,*.cpp)
CSRC += $(call target_files,$(BOOTLOADER_MODULE_PATH)/../hal/src/xenon/littlefs/,*.c)

LDFLAGS += -T$(COMMON_BUILD)/arm/linker/linker_nrf52840_bootloader.ld
LINKER_DEPS += $(COMMON_BUILD)/arm/linker/linker_nrf52840_bootloader.ld
