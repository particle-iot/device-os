include ../../shared/system_module_version.mk
SYSTEM_PART1_MODULE_PATH ?= $(PROJECT_ROOT)/modules/argon/system-part1

ifeq ($(MINIMAL),y)
GLOBAL_DEFINES += SYSTEM_MINIMAL
endif

include $(call rwildcard,$(SYSTEM_PART1_MODULE_PATH)/,include.mk)
include $(call rwildcard,$(SHARED_MODULAR)/,include.mk)
