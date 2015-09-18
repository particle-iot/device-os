SYSTEM_PART1_MODULE_VERSION ?= 1
SYSTEM_PART1_MODULE_PATH ?= $(PROJECT_ROOT)/modules/electron/system-part1
include $(call rwildcard,$(SYSTEM_PART1_MODULE_PATH)/,include.mk)
include $(call rwildcard,$(SHARED_MODULAR)/,include.mk)



