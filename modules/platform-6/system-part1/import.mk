WIFI_SYSTEM_MODULE_PATH ?= $(PROJECT_ROOT)/modules/photon/system-part1
include $(call rwildcard,$(WIFI_SYSTEM_MODULE_PATH)/,include.mk)

