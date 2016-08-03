include ../../shared/system_module_version.mk
SYSTEM_PART3_MODULE_PATH ?= $(PROJECT_ROOT)/modules/electron/system-part1
include $(call rwildcard,$(SYSTEM_PART3_MODULE_PATH)/,include.mk)
include $(call rwildcard,$(SHARED_MODULAR)/,include.mk)

MODULE_HAS_SYSTEM_PART3=1



