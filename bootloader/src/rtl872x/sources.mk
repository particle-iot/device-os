BOOTLOADER_SRC_PATH = $(BOOTLOADER_MODULE_PATH)/src/rtl872x

CSRC += $(call target_files,$(BOOTLOADER_SRC_PATH)/,*.c)
CPPSRC += $(call target_files,$(BOOTLOADER_SRC_PATH)/,*.cpp)

# CSRC += $(call target_files,$(BOOTLOADER_MODULE_PATH)/../hal/src/nRF52840/,flash_hal.c)
# CPPSRC += $(call target_files,$(BOOTLOADER_MODULE_PATH)/../hal/src/nRF52840/,flash_common.cpp)
# CSRC += $(call target_files,$(BOOTLOADER_MODULE_PATH)/../hal/src/nRF52840/,exflash_hal.c)
CPPSRC += $(call target_files,$(BOOTLOADER_MODULE_PATH)/../hal/src/rtl872x/,rgbled_hal.cpp)
# CSRC += $(call target_files,$(BOOTLOADER_MODULE_PATH)/../hal/src/nRF52840/,watchdog_hal.c)
CPPSRC += $(call target_files,$(BOOTLOADER_MODULE_PATH)/../hal/src/rtl872x/,button_hal.cpp)
CPPSRC += $(call target_files,$(BOOTLOADER_MODULE_PATH)/../hal/src/$(PLATFORM_NAME)/,pinmap_defines.cpp)
CPPSRC += $(call target_files,$(BOOTLOADER_MODULE_PATH)/../hal/src/rtl872x/,pinmap_hal.cpp)
CPPSRC += $(call target_files,$(BOOTLOADER_MODULE_PATH)/../hal/src/rtl872x/,gpio_hal.cpp)
CPPSRC += $(call target_files,$(BOOTLOADER_MODULE_PATH)/../hal/src/rtl872x/,pwm_hal.cpp)
CPPSRC += $(call target_files,$(BOOTLOADER_MODULE_PATH)/../hal/src/rtl872x/,interrupts_hal.cpp)
# CPPSRC += $(call target_files,$(BOOTLOADER_MODULE_PATH)/../hal/src/nRF52840/,dct_hal.cpp)
# # FIXME
# CPPSRC += $(call target_files,$(BOOTLOADER_MODULE_PATH)/../hal/src/nRF52840/,inflate.cpp)
# CPPSRC += $(call target_files,$(BOOTLOADER_MODULE_PATH)/../hal/src/nRF52840/littlefs/,*.cpp)
# CSRC += $(call target_files,$(BOOTLOADER_MODULE_PATH)/../hal/src/nRF52840/littlefs/,*.c)

LDFLAGS += -T$(COMMON_BUILD)/arm/linker/linker_rtl872x_bootloader.ld
LINKER_DEPS += $(COMMON_BUILD)/arm/linker/linker_rtl872x_bootloader.ld
