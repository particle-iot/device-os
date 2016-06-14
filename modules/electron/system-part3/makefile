MODULE=system-part3
WIFI_SYSTEM_MODULE_PATH=.
PROJECT_ROOT=../../..
BUILD_PATH_EXT = $(BUILD_TARGET_PLATFORM)
HAL_LINK :=
PLATFORM_DFU = 0x8060000

MAKE_DEPENDENCIES = 

DEPENDENCIES = $(MAKE_DEPENDENCIES) dynalib rt-dynalib services

# rebuild if the linker specs change for other modules
DEPENDENCIES += modules/electron/user-part modules/electron/system-part1 modules/electron/system-part2
 
include ../modular.mk
include $(PROJECT_ROOT)/build/platform-id.mk
LIBS += $(MAKE_DEPENDENCIES)
LIB_DEPS += $(HAL_DYNALIB_LIB_DEP)
LIB_DIRS += $(dir $(LIB_DEPS))


TARGET=elf bin lst hex size

include $(PROJECT_ROOT)/build/arm-tlm.mk

$(call check_modular)


