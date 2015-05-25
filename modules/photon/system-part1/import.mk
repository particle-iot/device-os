SYSTEM_PART1_MODULE_VERSION ?= 2
SYSTEM_PART1_MODULE_PATH ?= $(PROJECT_ROOT)/modules/photon/system-part1
include $(call rwildcard,$(SYSTEM_PART1_MODULE_PATH)/,include.mk)



