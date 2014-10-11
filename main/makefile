MODULE=main
MAIN_MODULE_PATH=.

BUILD_PATH_EXT = prod-$(SPARK_PRODUCT_ID)$(MAIN_FLAVOR)

DEPENDENCIES = wiring services communication hal

include ../build/product-id.mk

ifeq ("","$(SPARK_NO_PLATFORM)")
DEPENDENCIES += platform
CFLAGS += -DSPARK_PLATFORM
endif

MAKE_DEPENDENCIES = $(DEPENDENCIES)
LIBS += $(DEPENDENCIES)


# Target this makefile is building.
ifeq ("$(ARCH)","gcc")
TARGET=exe
else
TARGET=elf bin hex size 
endif


include ../build/arm-tlm.mk


