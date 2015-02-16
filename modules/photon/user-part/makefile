MODULE=user-part
USER_PART_MODULE_PATH=.
PROJECT_ROOT=../../..
BUILD_PATH_EXT = $(BUILD_TARGET_PLATFORM)
HAL_LINK := 
PLATFORM_DFU = 0x$(USER_FIRMWARE_IMAGE_LOCATION)
PLATFORM_DFU_LEAVE = y
# communication used only for header declarations	
DEPENDENCIES = user dynalib services services-dynalib hal hal-dynalib system system-dynalib rt-dynalib wiring communication modules/photon/system-part2 
MAKE_DEPENDENCIES = user hal-dynalib services-dynalib system-dynalib rt-dynalib wiring
include ../modular.mk
include $(PROJECT_ROOT)/build/platform-id.mk

LIBS += $(MAKE_DEPENDENCIES)
LIB_DEPS += $(USER_LIB_DEP) $(SERVICES_DYNALIB_LIB_DEP) $(HAL_DYNALIB_LIB_DEP) $(SYSTEM_DYNALIB_LIB_DEP) $(RT_DYNALIB_LIB_DEP) $(WIRING_LIB_DEP)
LIB_DIRS += $(dir $(LIB_DEPS))

TARGET=elf bin lst hex size 


include $(PROJECT_ROOT)/build/arm-tlm.mk


