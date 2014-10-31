SERVICES_MODULE_PATH ?= ../services
include $(call rwildcard,$(SERVICES_MODULE_PATH)/,include.mk)


SERVICES_LIB_DIR = $(BUILD_PATH_BASE)/services/$(ARCH)
SERVICES_LIB_DEP = $(SERVICES_LIB_DIR)/libservices.a
