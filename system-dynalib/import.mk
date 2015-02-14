SYSTEM_DYNALIB_MODULE_PATH ?= $(PROJECT_ROOT)/system-dynalib
include $(call rwildcard,$(SYSTEM_DYNALIB_MODULE_PATH)/,include.mk)

SYSTEM_DYNALIB_LIB_DIR = $(BUILD_PATH_BASE)/system-dynalib/$(ARCH)
SYSTEM_DYNALIB_LIB_DEP = $(SYSTEM_DYNALIB_LIB_DIR)/libsystem-dynalib.a
