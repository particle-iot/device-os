MODULE=system-part1
WIFI_SYSTEM_MODULE_PATH=.

PROJECT_ROOT=../../..

BUILD_PATH_EXT = $(BUILD_TARGET_PLATFORM)
# don't link from hal - we link here
HAL_LINK := 
# HAL Dependency only for the WICED wifi resources	
MAKE_DEPENDENCIES = communication
DEPENDENCIES = $(MAKE_DEPENDENCIES) hal dynalib rt-dynalib modules/photon/system-part2

include $(PROJECT_ROOT)/build/platform-id.mk

LIBS += $(MAKE_DEPENDENCIES)
LIB_DEPS += $(COMMUNICATION_LIB_DEP)
LIB_DIRS += $(dir $(LIB_DEPS))

TARGET=elf bin lst hex size 


include $(PROJECT_ROOT)/build/arm-tlm.mk


