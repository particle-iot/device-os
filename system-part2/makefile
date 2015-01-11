MODULE=system-part2
SYSTEM_PART2_MODULE_PATH=.

BUILD_PATH_EXT = prod-$(SPARK_PRODUCT_ID)

DEPENDENCIES = services

include ../build/product-id.mk

PLATFORM_LIB_DIR = $(BUILD_PATH_BASE)/platform/prod-$(SPARK_PRODUCT_ID)
PLATFORM_LIB_DEP = $(PLATFORM_LIB_DIR)/libplatform.a

HAL_LIB_DIR = $(BUILD_PATH_BASE)/hal/prod-$(SPARK_PRODUCT_ID)$(HAL_TEST_FLAVOR)
HAL_LIB_DEP = $(HAL_LIB_DIR)/libhal.a


MAKE_DEPENDENCIES = $(DEPENDENCIES) hal platform
LIBS += $(MAKE_DEPENDENCIES)
LIB_DEPS += $(SERVICES_LIB_DEP) $(PLATFORM_LIB_DEP) $(HAL_LIB_DEP)
LIB_DIRS += $(dir $(LIB_DEPS))

TARGET=elf bin lst hex size 


include ../build/arm-tlm.mk


