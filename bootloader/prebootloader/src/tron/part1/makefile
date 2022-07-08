MODULE=prebootloader-part1
PREBOOTLOADER_PART1_MODULE_PATH=.
PROJECT_ROOT ?= ../../../../..
BOOTLOADER_SHARED_MODULAR = ../shared
BUILD_PATH_EXT = $(PREBOOTLOADER_PART1_BUILD_PATH_EXT)
RTL_BINARY_FLASH_START = 0x14000

DEPENDENCIES = hal platform dynalib services wiring
MAKE_DEPENDENCIES = $(PLATFORM_DEPS)
# dependent on headers, but not the libraries
# LIBS +=
LIB_DEPS += $(PLATFORM_LIB_DEPS)
LIB_DIRS += $(dir $(LIB_DEPS))

BOOTLOADER_USE_LTO?=y
export COMPILE_LTO=BOOTLOADER_USE_LTO
export BOOTLOADER_MODULE=1

include $(PROJECT_ROOT)/build/platform-id.mk
# Include top level makefile which can override certain things
-include $(PREBOOTLOADER_PART1_MODULE_PATH)/tlm.mk
# Reinitialize platform definitions if requested by tlm.mk (included_productid_mk=0)
include $(PROJECT_ROOT)/build/platform-id.mk

# Build bootloader as modular by default if the platform is modular
-include $(PROJECT_ROOT)/modules/$(PLATFORM_NAME)/modular.mk

# Target this makefile is building.
ifeq ("$(ARCH)","gcc")
TARGET=none
else
TARGET=elf bin hex size lst
endif


include $(PROJECT_ROOT)/build/arm-tlm.mk
