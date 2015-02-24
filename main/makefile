MODULE=main
USER_MODULE_PATH=.
PLATFORM_DFU_LEAVE = y
BUILD_PATH_EXT = $(BUILD_TARGET_PLATFORM)$(USER_FLAVOR)

MODULE_VERSION ?= 0
GLOBAL_DEFINES += MODULE_VERSION=$(MODULE_VERSION)
GLOBAL_DEFINES += MODULE_FUNCTION=$(MODULE_FUNCTION_MONO_FIRMWARE)

# It's critical that user comes before wiring so that the setup()/loop() functions are linked in preference
# to the weak functions from wiring
MAKE_DEPENDENCIES = user wiring hal system services communication platform
DEPENDENCIES = $(MAKE_DEPENDENCIES) dynalib

include ../build/platform-id.mk


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

