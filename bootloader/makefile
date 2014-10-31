MODULE=bootloader
BOOTLOADER_MODULE_PATH=.

BUILD_PATH_EXT = prod-$(SPARK_PRODUCT_ID)

DEPENDENCIES = platform hal services
MAKE_DEPENDENCIES = platform services
# dependent on HAL headers, but not the library
LIBS += $(MAKE_DEPENDENCIES)
LIB_DEPS += $(PLATFORM_LIB_DEP) $(SERVICES_LIB_DEP)
LIB_DIRS += $(PLATFORM_LIB_DIR) $(SERVICES_LIB_DIR)

include ../build/product-id.mk

# Target this makefile is building.
ifeq ("$(ARCH)","gcc")
TARGET=none
else
TARGET=elf bin hex size 
endif


include ../build/arm-tlm.mk

