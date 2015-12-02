SYSTEM_PART1_MODULE_VERSION ?= 8
SYSTEM_PART1_MODULE_PATH ?= $(PROJECT_ROOT)/modules/photon/system-part1
include $(call rwildcard,$(SYSTEM_PART1_MODULE_PATH)/,include.mk)
include $(call rwildcard,$(SHARED_MODULAR)/,include.mk)



