MODULE=bootloader

# Target this makefile is building.
TARGET=elf bin hex size lst
BUILD_PATH_EXT = prod-$(SPARK_PRODUCT_ID)


DEPENDENCIES = platform hal services
MAKE_DEPENDENCIES = platform services

include ../build/arm-tlm.mk
