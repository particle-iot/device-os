PROTO_DEFS_MODULE_NAME = proto_defs
PROTO_DEFS_MODULE_PATH ?= $(PROJECT_ROOT)/$(PROTO_DEFS_MODULE_NAME)
include $(call rwildcard,$(PROTO_DEFS_MODULE_PATH)/,include.mk)

PROTO_DEFS_BUILD_PATH_EXT = $(BUILD_TARGET_PLATFORM)
PROTO_DEFS_LIB_DIR = $(BUILD_PATH_BASE)/$(PROTO_DEFS_MODULE_NAME)/$(PROTO_DEFS_BUILD_PATH_EXT)
PROTO_DEFS_LIB_DEP = $(PROTO_DEFS_LIB_DIR)/lib$(PROTO_DEFS_MODULE_NAME).a
