MODULE=main
MAIN_MODULE_PATH=.
PLATFORM_DFU_LEAVE = y
BUILD_PATH_EXT = $(BUILD_TARGET_PLATFORM)$(USER_FLAVOR)

# It's critical that user comes before wiring so that the setup()/loop() functions are linked in preference
# to the weak functions from wiring
LIB_DEPENDENCIES = wiring hal system services communication platform
MAKE_DEPENDENCIES = user $(LIB_DEPENDENCIES)
DEPENDENCIES = $(MAKE_DEPENDENCIES) dynalib

include ../build/platform-id.mk


LIBS += $(LIB_DEPENDENCIES)
LIB_DEPS += $(WIRING_LIB_DEP) $(SYSTEM_LIB_DEP) $(SERVICES_LIB_DEP) $(COMMUNICATION_LIB_DEP) $(HAL_LIB_DEP) $(PLATFORM_LIB_DEP)
LIB_DIRS += $(dir $(LIB_DEPS))
LIBS_EXT += -Wl,--whole-archive $(USER_LIB_DEP) -Wl,--no-whole-archive 

# Target this makefile is building.
ifeq ("$(ARCH)","gcc")
TARGET=exe
else
TARGET=elf bin lst hex size 
endif


include ../build/arm-tlm.mk

