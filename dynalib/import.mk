DYNALIB_MODULE_PATH ?= $(PROJECT_ROOT)/dynalib
include $(call rwildcard,$(DYNALIB_MODULE_PATH)/,include.mk)

DYNALIB_LIB_DIR = $(BUILD_PATH_BASE)/dynalib/$(ARCH)
DYNALIB_LIB_DEP = $(DYNALIB_LIB_DIR)/libdynalib.a
