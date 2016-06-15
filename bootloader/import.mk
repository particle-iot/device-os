BOOTLOADER_MODULE_PATH ?= $(PROJECT_ROOT)/bootloader
BOOTLOADER_VERSION ?= 8
BOOTLOADER_BUILD_PATH_EXT = $(BUILD_TARGET_PLATFORM)

# bring in the include folders from inc and src/<platform> is includes
include $(call rwildcard,$(BOOTLOADER_MODULE_PATH)/inc/,include.mk)
include $(call rwildcard,$(BOOTLOADER_MODULE_PATH)/src/$(PLATFORM_NAME)/,include.mk)

