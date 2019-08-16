MODULE=bootloader
BOOTLOADER_MODULE_PATH=.
PLATFORM_DFU=0x8000000
BUILD_PATH_EXT = $(BOOTLOADER_BUILD_PATH_EXT)

DEPENDENCIES = platform hal services system dynalib
MAKE_DEPENDENCIES = platform services
# dependent on HAL headers, but not the library
LIBS += $(MAKE_DEPENDENCIES)
LIB_DEPS += $(PLATFORM_LIB_DEP) $(SERVICES_LIB_DEP)
LIB_DIRS += $(dir $(LIB_DEPS))

export COMPILE_LTO=y

export BOOTLOADER_MODULE=1

include ../build/platform-id.mk

ifeq ($(MONO_MFG_FIRMWARE_AT_USER_PART),y)
include ../modules/$(PLATFORM_NAME)/modular.mk
endif

# Target this makefile is building.
ifeq ("$(ARCH)","gcc")
TARGET=none
else
TARGET=elf bin hex size lst
endif


include ../build/arm-tlm.mk

