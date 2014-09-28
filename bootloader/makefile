MODULE=bootloader

BUILD_PATH_EXT = prod-$(SPARK_PRODUCT_ID)

DEPENDENCIES = platform hal services
MAKE_DEPENDENCIES = platform services

include ../build/product-id.mk

# Target this makefile is building.
ifeq ("$(ARCH)","gcc")
TARGET=none
else
TARGET=elf bin hex size 
endif


include ../build/arm-tlm.mk

