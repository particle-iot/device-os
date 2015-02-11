MODULE=module-part1
WIFI_SYSTEM_MODULE_PATH=.

BUILD_PATH_EXT = $(BUILD_TARGET_PLATFORM)
# don't link from hal - we link here
HAL_LINK := 
# HAL Dependency only for the WICED wifi resources	
MAKE_DEPENDENCIES = communication
DEPENDENCIES = $(MAKE_DEPENDENCIES) hal dynalib rt-dynalib system-part2

include ../build/platform-id.mk

LIBS += $(MAKE_DEPENDENCIES)
LIB_DEPS += $(COMMUNICATION_LIB_DEP)
LIB_DIRS += $(dir $(LIB_DEPS))

TARGET=elf bin lst hex size 


include ../build/arm-tlm.mk


