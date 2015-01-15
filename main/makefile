MODULE=main
MAIN_MODULE_PATH=.

ifdef PRODUCT_ID
MAIN_PRODUCT = -prod-$(PRODUCT_ID)
endif

BUILD_PATH_EXT = $(BUILD_TARGET_PLATFORM)$(MAIN_FLAVOR)$(MAIN_PRODUCT)

DEPENDENCIES = wiring services communication hal

include ../build/platform-id.mk

ifeq ("","$(SPARK_NO_PLATFORM)")
DEPENDENCIES += platform
CFLAGS += -DSPARK_PLATFORM
endif

MAKE_DEPENDENCIES = $(DEPENDENCIES)
LIBS += $(MAKE_DEPENDENCIES)
LIB_DEPS += $(WIRING_LIB_DEP) $(SERVICES_LIB_DEP) $(COMMUNICATION_LIB_DEP) $(HAL_LIB_DEP) $(PLATFORM_LIB_DEP)
LIB_DIRS += $(dir $(LIB_DEPS))


# Target this makefile is building.
ifeq ("$(ARCH)","gcc")
TARGET=exe
else
TARGET=elf bin hex size 
endif


include ../build/arm-tlm.mk


