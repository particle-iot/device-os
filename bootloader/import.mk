BOOTLOADER_MODULE_PATH ?= ../bootloader

# bring in the include folders from inc and src/<platform> is includes
include $(call rwildcard,$(BOOTLOADER_MODULE_PATH)/inc/,include.mk)
include $(call rwildcard,$(BOOTLOADER_MODULE_PATH)/src/$(PLATFORM_NAME)/,include.mk)
	