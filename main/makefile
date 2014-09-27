MODULE=main

# Target this makefile is building.
TARGET=elf bin hex size 
BUILD_PATH_EXT = prod-$(SPARK_PRODUCT_ID)

DEPENDENCIES = wiring communication hal services platform
MAKE_DEPENDENCIES = $(DEPENDENCIES)

include ../build/arm-tlm.mk
