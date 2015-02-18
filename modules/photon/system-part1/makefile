MODULE=system-part1
WIFI_SYSTEM_MODULE_PATH=.

PROJECT_ROOT=../../..
PLATFORM_DFU = 0x8020000
BUILD_PATH_EXT = $(BUILD_TARGET_PLATFORM)
# don't link from hal - we link here
HAL_LINK := 
# HAL Dependency only for the WICED wifi nvmem resource
MAKE_DEPENDENCIES = dynalib communication hal services platform
DEPENDENCIES = $(MAKE_DEPENDENCIES) rt-dynalib modules/photon/system-part2
include ../modular.mk
include $(PROJECT_ROOT)/build/platform-id.mk

# dependent on hal lib only for the wwd_nvmem_resource
LIBS += $(MAKE_DEPENDENCIES)
LIB_DEPS += $(COMMUNICATION_LIB_DEP) $(HAL_LIB_DEP) $(SERVICES_LIB_DEP) $(PLATFORM_LIB_DEP) $(DYNALIB_LIB_DEP)
LIB_DIRS += $(dir $(LIB_DEPS))
# This is needed for the module_info to be correctly linked.
LIBS_EXT += -Wl,--whole-archive -ldynalib -Wl,--no-whole-archive

TARGET=elf bin lst hex size 

include $(PROJECT_ROOT)/build/arm-tlm.mk


