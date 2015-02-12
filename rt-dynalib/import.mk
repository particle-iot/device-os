RT_DYNALIB_MODULE_PATH ?= ../rt-dynalib
include $(call rwildcard,$(RT_DYNALIB_MODULE_PATH)/,include.mk)

RT_DYNALIB_LIB_DIR = $(BUILD_PATH_BASE)/rt-dynalib/$(ARCH)
RT_DYNALIB_LIB_DEP = $(RT_DYNALIB_LIB_DIR)/librt-dynalib.a

INCLUDE_DIRS += $(RT_DYNALIB_MODULE_PATH)/inc

# have to disable malloc/free builtins
CFLAGS += -fno-builtin

