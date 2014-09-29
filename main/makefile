MODULE=main
MAIN_MODULE_PATH=.

BUILD_PATH_EXT = prod-$(SPARK_PRODUCT_ID)

DEPENDENCIES = wiring services platform communication hal
MAKE_DEPENDENCIES = $(DEPENDENCIES)
LIBS += $(DEPENDENCIES)

include ../build/product-id.mk

# Target this makefile is building.
ifeq ("$(ARCH)","gcc")
TARGET=exe
else
TARGET=elf bin hex size 
endif


include ../build/arm-tlm.mk


