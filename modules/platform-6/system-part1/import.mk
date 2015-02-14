WIFI_SYSTEM_MODULE_PATH ?= ../system-part1
include $(call rwildcard,$(WIFI_SYSTEM_MODULE_PATH)/,include.mk)

