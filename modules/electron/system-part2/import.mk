include ../../shared/system_module_version.mk
SYSTEM_PART1_MODULE_PATH ?= $(PROJECT_ROOT)/modules/electron/system-part2
include $(call rwildcard,$(SYSTEM_PART1_MODULE_PATH)/,include.mk)
include $(call rwildcard,$(SHARED_MODULAR)/,include.mk)



