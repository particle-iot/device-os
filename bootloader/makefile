MODULE=bootloader
BOOTLOADER_MODULE_PATH=.
PLATFORM_DFU=0x8000000
BUILD_PATH_EXT = $(BUILD_TARGET_PLATFORM)


DEPENDENCIES = platform hal services
MAKE_DEPENDENCIES = platform services
# dependent on HAL headers, but not the library
LIBS += $(MAKE_DEPENDENCIES)
LIB_DEPS += $(PLATFORM_LIB_DEP) $(SERVICES_LIB_DEP)
LIB_DIRS += $(PLATFORM_LIB_DIR) $(SERVICES_LIB_DIR)

include ../build/platform-id.mk

# Target this makefile is building.
ifeq ("$(ARCH)","gcc")
TARGET=none
else
TARGET=elf bin hex size 
endif


include ../build/arm-tlm.mk

