include ../../shared/system_module_version.mk
SYSTEM_PART2_MODULE_PATH ?= $(PROJECT_ROOT)/modules/electron/system-part3
include $(call rwildcard,$(SYSTEM_PART2_MODULE_PATH)/,include.mk)
