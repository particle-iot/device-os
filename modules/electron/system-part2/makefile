MODULE=system-part2
SYSTEM_PART1_MODULE_PATH=.
PROJECT_ROOT=../../..
BUILD_PATH_EXT = $(BUILD_TARGET_PLATFORM)
HAL_LINK :=
PLATFORM_DFU = 0x8020000

# Platform dependency is for the LED implementation used by rgbled.c in services.
MAKE_DEPENDENCIES = communication hal-dynalib services platform crypto
DEPENDENCIES = $(MAKE_DEPENDENCIES) dynalib rt-dynalib modules/electron/system-part1  modules/electron/system-part3
include ../modular.mk
include $(PROJECT_ROOT)/build/platform-id.mk

LIBS += $(MAKE_DEPENDENCIES)
LIB_DEPS += $(COMMUNICATION_LIB_DEP) $(HAL_DYNALIB_LIB_DEP) $(SERVICES_LIB_DEP) $(PLATFORM_LIB_DEP) $(CRYPTO_LIB_DEP)
LIB_DIRS += $(dir $(LIB_DEPS))

TARGET=elf bin lst hex size

SERVICES_NO_NANOPB_LIB := y

include $(PROJECT_ROOT)/build/arm-tlm.mk

$(call check_modular)


