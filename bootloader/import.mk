BOOTLOADER_MODULE_PATH ?= ../bootloader
include $(call rwildcard,$(BOOTLOADER_MODULE_PATH)/,include.mk)
	
STM32_DEVICE_SYMBOL  = $(shell echo $(STM32_DEVICE) | tr a-z A-Z)