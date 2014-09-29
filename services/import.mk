SERVICES_MODULE_PATH ?= ../services
include $(call rwildcard,$(SERVICES_MODULE_PATH)/,include.mk)

LIB_DIRS += $(BUILD_PATH_BASE)/services/$(ARCH)
