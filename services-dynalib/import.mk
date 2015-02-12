SERVICES_DYNALIB_MODULE_PATH ?= ../services-dynalib
include $(call rwildcard,$(SERVICES_DYNALIB_MODULE_PATH)/,include.mk)


SERVICES_DYNALIB_LIB_DIR = $(BUILD_PATH_BASE)/services-dynalib/$(ARCH)
SERVICES_DYNALIB_LIB_DEP = $(SERVICES_DYNALIB_LIB_DIR)/libservices-dynalib.a
