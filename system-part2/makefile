MODULE=system-part2
SYSTEM_PART2_MODULE_PATH=.

BUILD_PATH_EXT = $(BUILD_TARGET_PLATFORM)
HAL_LINK := 
	
DEPENDENCIES = user-part dynalib services hal platform system wiring communication-dynalib rt-dynalib system-part1
MAKE_DEPENDENCIES = services hal platform system wiring communication-dynalib

include ../build/platform-id.mk

LIBS += $(MAKE_DEPENDENCIES)
LIB_DEPS += $(SERVICES_LIB_DEP) $(PLATFORM_LIB_DEP) $(HAL_LIB_DEP) $(SYSTEM_LIB_DEP) $(COMMUNICATION_DYNALIB_LIB_DEP) $(WIRING_LIB_DEP)
LIB_DIRS += $(dir $(LIB_DEPS))

TARGET=elf bin lst hex size 


include ../build/arm-tlm.mk


