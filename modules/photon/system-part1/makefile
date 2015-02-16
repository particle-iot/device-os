MODULE=system-part1
WIFI_SYSTEM_MODULE_PATH=.

PROJECT_ROOT=../../..
PLATFORM_DFU = 0x8060000
BUILD_PATH_EXT = $(BUILD_TARGET_PLATFORM)
# don't link from hal - we link here
HAL_LINK := 
# HAL Dependency only for the WICED wifi nvmem resource
MAKE_DEPENDENCIES = communication hal
DEPENDENCIES = $(MAKE_DEPENDENCIES) dynalib rt-dynalib modules/photon/system-part2
include ../modular.mk
include $(PROJECT_ROOT)/build/platform-id.mk

# dependent on hal lib only for the wwd_nvmem_resource
LIBS += $(MAKE_DEPENDENCIES)
LIB_DEPS += $(COMMUNICATION_LIB_DEP) $(HAL_LIB_DEP)
LIB_DIRS += $(dir $(LIB_DEPS))

TARGET=elf bin lst hex size 

include $(PROJECT_ROOT)/build/arm-tlm.mk


