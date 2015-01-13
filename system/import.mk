SYSTEM_MODULE_PATH ?= ../system
include $(call rwildcard,$(SYSTEM_MODULE_PATH)/,include.mk)

SYSTEM_LIB_DIR = $(BUILD_PATH_BASE)/system/$(ARCH)
SYSTEM_LIB_DEP = $(SYSTEM_LIB_DIR)/libsystem.a
