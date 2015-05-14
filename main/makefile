
MODULE=main
MAIN_MODULE_PATH=.

include ../build/platform-id.mk

ifeq ($(PLATFORM_ID),6)
MODULAR?=y
endif
ifeq ($(PLATFORM_ID),8)
MODULAR?=y
endif

ifeq ($(MODULAR),y)

PROJECT_ROOT = ..
COMMON_BUILD=../build
BUILD_PATH_BASE=$(COMMON_BUILD)/target
MAKE_DEPENDENCIES = modules/photon/user-part

SUBDIR_GOALS := $(MAKECMDGOALS)
MAKECMDGOALS := all

all program-dfu st-flash: make_deps

include $(COMMON_BUILD)/common-tools.mk
include $(COMMON_BUILD)/recurse.mk
include $(COMMON_BUILD)/verbose.mk

clean: clean_deps
	$(VERBOSE)$(RMDIR) $(BUILD_PATH_BASE)

.PNONY: st-flash program-dfu clean

else

PLATFORM_DFU_LEAVE = y
BUILD_PATH_EXT = $(BUILD_TARGET_PLATFORM)$(USER_FLAVOR)

USE_PRINTF_FLOAT ?= y

# It's critical that user comes before wiring so that the setup()/loop() functions are linked in preference
# to the weak functions from wiring
MAKE_DEPENDENCIES = user wiring hal system services communication platform
DEPENDENCIES = $(MAKE_DEPENDENCIES) dynalib

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

endif