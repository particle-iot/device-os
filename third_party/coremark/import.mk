COREMARK_MODULE_NAME = coremark
COREMARK_MODULE_PATH ?= $(PROJECT_ROOT)/third_party/$(COREMARK_MODULE_NAME)
include $(COREMARK_MODULE_PATH)/include.mk

COREMARK_BUILD_PATH_EXT = $(BUILD_TARGET_PLATFORM)
COREMARK_LIB_DIR = $(BUILD_PATH_BASE)/$(COREMARK_MODULE_NAME)/$(COREMARK_BUILD_PATH_EXT)
COREMARK_LIB_DEP = $(COREMARK_LIB_DIR)/lib$(COREMARK_MODULE_NAME).a
