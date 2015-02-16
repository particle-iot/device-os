MODULE=system-part2
SYSTEM_PART2_MODULE_PATH=.
PROJECT_ROOT = ../../..
BUILD_PATH_EXT = $(BUILD_TARGET_PLATFORM)
HAL_LINK := 
PLATFORM_DFU = 0x8020000
DEPENDENCIES = modules/photon/user-part dynalib services hal platform system wiring communication-dynalib rt-dynalib modules/photon/system-part1
MAKE_DEPENDENCIES = services hal platform system wiring communication-dynalib

include ../modular.mk
include $(PROJECT_ROOT)/build/platform-id.mk

LIBS += $(MAKE_DEPENDENCIES)
LIB_DEPS += $(SERVICES_LIB_DEP) $(PLATFORM_LIB_DEP) $(HAL_LIB_DEP) $(SYSTEM_LIB_DEP) $(COMMUNICATION_DYNALIB_LIB_DEP) $(WIRING_LIB_DEP)
LIB_DIRS += $(dir $(LIB_DEPS))

TARGET=elf bin lst hex size 


include $(PROJECT_ROOT)/build/arm-tlm.mk


