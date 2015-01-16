
MODULE=hal
HAL_MODULE_PATH=.

# Target this makefile is building.
TARGET_TYPE = a
BUILD_PATH_EXT = $(BUILD_TARGET_PLATFORM)$(HAL_TEST_FLAVOR)

DEPENDENCIES = platform services

# ensure PLATFORM_NAME is defined
include ../build/platform-id.mk
-include $(HAL_MODULE_PATH)/src/$(PLATFORM_NAME)/tlm.mk


include ../build/arm-tlm.mk

