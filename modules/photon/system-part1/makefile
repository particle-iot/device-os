MODULE=system-part1
WIFI_SYSTEM_MODULE_PATH=.
PROJECT_ROOT=../../..
BUILD_PATH_EXT = $(BUILD_TARGET_PLATFORM)
HAL_LINK := 
PLATFORM_DFU = 0x8020000

MODULE_VERSION ?= 0
GLOBAL_DEFINES += MODULE_VERSION=$(MODULE_VERSION)
GLOBAL_DEFINES += MODULE_FUNCTION=$(MODULE_FUNCTION_SYSTEM_PART)
GLOBAL_DEFINES += MODULE_INDEX=1


# HAL Dependency only for the WICED wifi nvmem resource
MAKE_DEPENDENCIES = communication hal services platform
DEPENDENCIES = $(MAKE_DEPENDENCIES) dynalib rt-dynalib modules/photon/system-part2
include ../modular.mk
include $(PROJECT_ROOT)/build/platform-id.mk

# dependent on hal lib only for the wwd_nvmem_resource
LIBS += $(MAKE_DEPENDENCIES)
LIB_DEPS += $(COMMUNICATION_LIB_DEP) $(HAL_LIB_DEP) $(SERVICES_LIB_DEP) $(PLATFORM_LIB_DEP)
LIB_DIRS += $(dir $(LIB_DEPS))

TARGET=elf bin lst hex size 

include $(PROJECT_ROOT)/build/arm-tlm.mk


