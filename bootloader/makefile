MODULE=bootloader
BOOTLOADER_MODULE_PATH=.
PLATFORM_DFU=0x8000000
BUILD_PATH_EXT = $(BOOTLOADER_BUILD_PATH_EXT)

ifeq ($(PLATFORM_MCU),rtl872x)
RTL_BINARY_FLASH_START = 0x4000
endif

SERVICES_NO_NANOPB_LIB := y

DEPENDENCIES = platform hal services system dynalib
MAKE_DEPENDENCIES = platform services
# dependent on HAL headers, but not the library
LIBS += $(MAKE_DEPENDENCIES)
LIB_DEPS += $(PLATFORM_LIB_DEP) $(SERVICES_LIB_DEP)
LIB_DIRS += $(dir $(LIB_DEPS))

BOOTLOADER_USE_LTO?=y

export COMPILE_LTO=BOOTLOADER_USE_LTO

export BOOTLOADER_MODULE=1

include ../build/platform-id.mk

# Build bootloader as modular by default if the platform is modular
-include ../modules/$(PLATFORM_NAME)/modular.mk

# Target this makefile is building.
ifeq ("$(ARCH)","gcc")
TARGET=none
else
TARGET=elf bin hex size lst
endif


include ../build/arm-tlm.mk
