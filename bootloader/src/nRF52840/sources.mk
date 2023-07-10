BOOTLOADER_SRC_PATH = $(BOOTLOADER_MODULE_PATH)/src/nRF52840

CSRC += $(call target_files,$(BOOTLOADER_SRC_PATH)/,*.c)
CPPSRC += $(call target_files,$(BOOTLOADER_SRC_PATH)/,*.cpp)

CSRC += $(call target_files,$(BOOTLOADER_MODULE_PATH)/../hal/src/nRF52840/,flash_hal.c)
CPPSRC += $(call target_files,$(BOOTLOADER_MODULE_PATH)/../hal/shared/,flash_common.cpp)
CPPSRC += $(call target_files,$(BOOTLOADER_MODULE_PATH)/../hal/src/nRF52840/,exflash_hal.cpp)
CPPSRC += $(call target_files,$(BOOTLOADER_MODULE_PATH)/../hal/src/nRF52840/,exflash_hal_lock.cpp)
CPPSRC += $(call target_files,$(BOOTLOADER_MODULE_PATH)/../hal/src/nRF52840/,exflash_hal_params.cpp)
CSRC += $(call target_files,$(BOOTLOADER_MODULE_PATH)/../hal/src/nRF52840/,rgbled_hal.c)
CSRC += $(call target_files,$(BOOTLOADER_MODULE_PATH)/../hal/src/nRF52840/,watchdog_hal.c)
CSRC += $(call target_files,$(BOOTLOADER_MODULE_PATH)/../hal/src/nRF52840/,pinmap_hal.c)
CSRC += $(call target_files,$(BOOTLOADER_MODULE_PATH)/../hal/src/nRF52840/,button_hal.c)
CSRC += $(call target_files,$(BOOTLOADER_MODULE_PATH)/../hal/src/$(PLATFORM_NAME)/,pinmap_hal.c)
CPPSRC += $(call target_files,$(BOOTLOADER_MODULE_PATH)/../hal/src/nRF52840/,gpio_hal.cpp)
CPPSRC += $(call target_files,$(BOOTLOADER_MODULE_PATH)/../hal/src/nRF52840/,pwm_hal.cpp)
CPPSRC += $(call target_files,$(BOOTLOADER_MODULE_PATH)/../hal/src/nRF52840/,interrupts_hal.cpp)
CPPSRC += $(call target_files,$(BOOTLOADER_MODULE_PATH)/../hal/src/nRF52840/,dct_hal.cpp)
CPPSRC += $(call target_files,$(BOOTLOADER_MODULE_PATH)/../hal/src/nRF52840/,nrf_system_error.cpp)
# FIXME
CPPSRC += $(call target_files,$(BOOTLOADER_MODULE_PATH)/../hal/shared/,inflate.cpp)
CPPSRC += $(call target_files,$(BOOTLOADER_MODULE_PATH)/../hal/src/nRF52840/littlefs/,*.cpp)
CPPSRC += $(call target_files,$(BOOTLOADER_MODULE_PATH)/../hal/shared/,filesystem.cpp)
CSRC += $(call target_files,$(BOOTLOADER_MODULE_PATH)/../hal/src/nRF52840/littlefs/,*.c)

LDFLAGS += -T$(BOOTLOADER_SRC_PATH)/linker.ld
LINKER_DEPS += $(BOOTLOADER_SRC_PATH)/linker.ld
