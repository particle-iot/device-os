
MODULE=platform
PLATFORM_MODULE_PATH=.

# Target this makefile is building.
TARGET_TYPE = a
BUILD_PATH_EXT = $(PLATFORM_BUILD_PATH_EXT)

DEPENDENCIES = services hal dynalib

# ensure PLATFORM_MCU is defined
include ../build/platform-id.mk

PLATFORM_MCU_PATH ?= $(PLATFORM_MODULE_PATH)/MCU/$(PLATFORM_MCU)
-include $(PLATFORM_MCU_PATH)/tlm.mk

include ../build/arm-tlm.mk
