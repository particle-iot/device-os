SYSTEM_PART2_MODULE_VERSION ?= 2
SYSTEM_PART2_MODULE_PATH ?= $(PROJECT_ROOT)/modules/photon/system-part2
include $(call rwildcard,$(SYSTEM_PART2_MODULE_PATH)/,include.mk)