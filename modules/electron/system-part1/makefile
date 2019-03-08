MODULE=system-part1
SYSTEM_PART3_MODULE_PATH=.
PROJECT_ROOT=../../..
BUILD_PATH_EXT = $(BUILD_TARGET_PLATFORM)
HAL_LINK :=
PLATFORM_DFU = 0x8060000

LIB_DEPENDENCIES = services-dynalib rt-dynalib platform third_party/nanopb
MAKE_DEPENDENCIES = $(LIB_DEPENDENCIES)

DEPENDENCIES = $(MAKE_DEPENDENCIES) dynalib services hal

# rebuild if the linker specs change for other modules
DEPENDENCIES += modules/electron/user-part modules/electron/system-part2 modules/electron/system-part3

include ../modular.mk
include $(PROJECT_ROOT)/build/platform-id.mk
LIBS += $(notdir $(LIB_DEPENDENCIES))
LIB_DEPS += $(SERVICES_DYNALIB_LIB_DEP) $(RT_DYNALIB_LIB_DEP) $(PLATFORM_LIB_DEP) $(NANOPB_LIB_DEP)
LIB_DIRS += $(dir $(LIB_DEPS))


TARGET=elf bin lst hex size

include $(PROJECT_ROOT)/build/arm-tlm.mk

$(call check_modular)


