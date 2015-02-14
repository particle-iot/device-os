MODULE=main
USER_MODULE_PATH=.
PLATFORM_DFU_LEAVE = y
BUILD_PATH_EXT = $(BUILD_TARGET_PLATFORM)$(USER_FLAVOR)

DEPENDENCIES = user wiring system services communication hal platform

include ../build/platform-id.mk

MAKE_DEPENDENCIES = $(DEPENDENCIES)
LIBS += $(MAKE_DEPENDENCIES)
LIB_DEPS += $(USER_LIB_DEP) $(WIRING_LIB_DEP) $(SYSTEM_LIB_DEP) $(SERVICES_LIB_DEP) $(COMMUNICATION_LIB_DEP) $(HAL_LIB_DEP) $(PLATFORM_LIB_DEP)
LIB_DIRS += $(dir $(LIB_DEPS))

# Target this makefile is building.
ifeq ("$(ARCH)","gcc")
TARGET=exe
else
TARGET=elf bin lst hex size 
endif


include ../build/arm-tlm.mk

