include ../../shared/system_module_version.mk
SYSTEM_PART2_MODULE_PATH ?= $(PROJECT_ROOT)/modules/photon/system-part2

ifeq ($(MINIMAL),y)
GLOBAL_DEFINES += SYSTEM_MINIMAL
endif

include $(call rwildcard,$(SYSTEM_PART2_MODULE_PATH)/,include.mk)
