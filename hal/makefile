
MODULE=hal
HAL_MODULE_PATH=.

# Target this makefile is building.
TARGET_TYPE = a
BUILD_PATH_EXT = $(HAL_BUILD_PATH_EXT)

DEPENDENCIES = newlib_nano platform services dynalib bootloader wiring crypto

# ensure PLATFORM_NAME is defined
include ../build/platform-id.mk
-include $(HAL_MODULE_PATH)/src/$(PLATFORM_NAME)/tlm.mk


include ../build/arm-tlm.mk

