DYNALIB_MODULE_PATH ?= ../dynalib
include $(call rwildcard,$(DYNALIB_MODULE_PATH)/,include.mk)

DYNALIB_LIB_DIR = $(BUILD_PATH_BASE)/dynalib/$(BUILD_TARGET_PLATFORM)
DYNALIB_LIB_DEP = $(DYNALIB_LIB_DIR)/libdynalib.a
