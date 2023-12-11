BOOTLOADER_SRC_PATH = $(BOOTLOADER_MODULE_PATH)/src/rtl872x

CSRC += $(call target_files,$(BOOTLOADER_SRC_PATH)/,*.c)
CPPSRC += $(call target_files,$(BOOTLOADER_SRC_PATH)/,*.cpp)

CSRC += $(call target_files,$(BOOTLOADER_MODULE_PATH)/../hal/src/rtl872x/,flash_hal.c)
CPPSRC += $(call target_files,$(BOOTLOADER_MODULE_PATH)/../hal/src/rtl872x/,flash_common.cpp)
CPPSRC += $(call target_files,$(BOOTLOADER_MODULE_PATH)/../hal/src/rtl872x/,exflash_hal.cpp)
CPPSRC += $(call target_files,$(BOOTLOADER_MODULE_PATH)/../hal/src/rtl872x/,rgbled_hal.cpp)
# CSRC += $(call target_files,$(BOOTLOADER_MODULE_PATH)/../hal/src/rtl872x/,watchdog_hal.c)
CPPSRC += $(call target_files,$(BOOTLOADER_MODULE_PATH)/../hal/src/rtl872x/,button_hal.cpp)
CPPSRC += $(call target_files,$(BOOTLOADER_MODULE_PATH)/../hal/src/$(PLATFORM_NAME)/,pinmap_defines.cpp)
CPPSRC += $(call target_files,$(BOOTLOADER_MODULE_PATH)/../hal/src/rtl872x/,pinmap_hal.cpp)
CPPSRC += $(call target_files,$(BOOTLOADER_MODULE_PATH)/../hal/src/rtl872x/,gpio_hal.cpp)
CPPSRC += $(call target_files,$(BOOTLOADER_MODULE_PATH)/../hal/src/rtl872x/,pwm_hal.cpp)
CPPSRC += $(call target_files,$(BOOTLOADER_MODULE_PATH)/../hal/src/rtl872x/,interrupts_hal.cpp)
CPPSRC += $(call target_files,$(BOOTLOADER_MODULE_PATH)/../hal/src/rtl872x/,dct_hal.cpp)
CPPSRC += $(call target_files,$(BOOTLOADER_MODULE_PATH)/../hal/shared/,inflate.cpp)
CPPSRC += $(call target_files,$(BOOTLOADER_MODULE_PATH)/../hal/src/rtl872x/,km0_km4_ipc.cpp)
CPPSRC += $(call target_files,$(BOOTLOADER_MODULE_PATH)/../hal/src/rtl872x/,rtl_sdk_support.cpp)
CPPSRC += $(call target_files,$(BOOTLOADER_MODULE_PATH)/../hal/src/rtl872x/,timer_hal.cpp)
CPPSRC += $(call target_files,$(BOOTLOADER_MODULE_PATH)/../hal/src/rtl872x/littlefs/,*.cpp)
CSRC += $(call target_files,$(BOOTLOADER_MODULE_PATH)/../hal/src/rtl872x/littlefs/,*.c)
CPPSRC += $(call target_files,$(BOOTLOADER_MODULE_PATH)/../hal/shared/,filesystem.cpp)
CPPSRC += $(call target_files,$(BOOTLOADER_MODULE_PATH)/../hal/src/rtl872x/,usbd_device.cpp)
CPPSRC += $(call target_files,$(BOOTLOADER_MODULE_PATH)/../hal/src/rtl872x/,usbd_driver.cpp)

LDFLAGS += -T$(BOOTLOADER_SRC_PATH)/linker.ld
LINKER_DEPS += $(BOOTLOADER_SRC_PATH)/linker.ld
