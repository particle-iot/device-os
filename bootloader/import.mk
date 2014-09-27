BOOTLOADER_MODULE_PATH ?= ../bootloader
include $(call rwildcard,$(BOOTLOADER_MODULE_PATH)/,include.mk)
	