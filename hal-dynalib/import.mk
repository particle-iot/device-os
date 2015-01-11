HAL_DYNALIB_MODULE_PATH ?= ../hal-dynalib
include $(call rwildcard,$(HAL_DYNALIB_MODULE_PATH)/,include.mk)

HAL_DYNALIB_LIB_DIR = $(BUILD_PATH_BASE)/hal-dynalib/$(ARCH)
HAL_DYNALIB_LIB_DEP = $(SERVICES_LIB_DIR)/libhal-dynalib.a
