MODULE=system-part2
SYSTEM_PART2_MODULE_PATH=.
PROJECT_ROOT = ../../..
BUILD_PATH_EXT = $(BUILD_TARGET_PLATFORM)
HAL_LINK := 
PLATFORM_DFU = 0x8060000
DEPENDENCIES = modules/photon/user-part dynalib services services-dynalib hal platform system wiring communication-dynalib rt-dynalib modules/photon/system-part1
MAKE_DEPENDENCIES = dynalib services-dynalib system wiring communication-dynalib hal platform
include ../modular.mk
include $(PROJECT_ROOT)/build/platform-id.mk

LIBS += $(MAKE_DEPENDENCIES)
LIB_DEPS += $(SERVICES_DYNALIB_LIB_DEP) $(PLATFORM_LIB_DEP) $(HAL_LIB_DEP) $(SYSTEM_LIB_DEP) $(COMMUNICATION_DYNALIB_LIB_DEP) $(WIRING_LIB_DEP) $(DYNALIB_LIB_DEP)
LIB_DIRS += $(dir $(LIB_DEPS))
# This is needed for the module_info to be correctly linked.
LIBS_EXT += -Wl,--whole-archive -ldynalib -Wl,--no-whole-archive

TARGET=elf bin lst hex size 


include $(PROJECT_ROOT)/build/arm-tlm.mk


