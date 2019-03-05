
MODULE=main
MAIN_MODULE_PATH=.

include ../build/platform-id.mk
include ../build/top-level-module.mk
include ../build/macros.mk

ifeq ($(PLATFORM_ID),0)
export COMPILE_LTO?=y
endif


ifeq ($(PLATFORM_ID),6)
MODULAR?=y
endif
ifeq ($(PLATFORM_ID),8)
MODULAR?=y
endif
ifeq ($(PLATFORM_ID),10)
MODULAR?=y
endif
ifeq ($(PLATFORM_ID),12)
MODULAR?=y
endif
ifeq ($(PLATFORM_ID),13)
MODULAR?=y
endif
ifeq ($(PLATFORM_ID),14)
MODULAR?=y
endif
ifeq ($(PLATFORM_ID),22)
MODULAR?=y
endif
ifeq ($(PLATFORM_ID),23)
MODULAR?=y
endif
ifeq ($(PLATFORM_ID),24)
MODULAR?=y
endif

ifeq ($(MODULAR),y)

PROJECT_ROOT = ..
COMMON_BUILD=../build
BUILD_PATH_BASE=$(COMMON_BUILD)/target
$(call check_modular)

# This is required to redefine the value of TARGET_DIR on the command line, since we are changing directories, so the relative path
# must also change
ifdef TARGET_DIR
ifneq ("$(TARGET_DIR)","$(abspath $(TARGET_DIR))")
override TARGET_DIR := $(abspath $(TARGET_DIR))
export TARGET_DIR
MAKEOVERRIDES := $(patsubst TARGET_FILE=%,,$(MAKE_OVERRIDES))
endif
endif

MAKE_DEPENDENCIES = modules/$(PLATFORM_DYNALIB_MODULES)/user-part
ifeq ("$(MAKECMDGOALS)","")
MAKECMDGOALS := all
endif

$(MAKECMDGOALS): make_deps

SUBDIR_GOALS := $(MAKECMDGOALS)
MAKECMDGOALS := all


include $(COMMON_BUILD)/common-tools.mk
include $(COMMON_BUILD)/recurse.mk
include $(COMMON_BUILD)/verbose.mk


.PNONY: st-flash program-dfu clean all

else

PLATFORM_DFU_LEAVE = y
BUILD_PATH_EXT = $(BUILD_TARGET_PLATFORM)$(USER_FLAVOR)

USE_PRINTF_FLOAT ?= y

# It's critical that user comes before wiring so that the setup()/loop() functions are linked in preference
# to the weak functions from wiring
MAKE_DEPENDENCIES = newlib_nano user wiring hal system services communication platform wiring_globals crypto
DEPENDENCIES = $(MAKE_DEPENDENCIES) dynalib

LIBS += $(MAKE_DEPENDENCIES)
LIB_DEPS += $(USER_LIB_DEP) $(WIRING_LIB_DEP) $(SYSTEM_LIB_DEP) $(SERVICES_LIB_DEP) $(COMMUNICATION_LIB_DEP) $(HAL_LIB_DEP) $(PLATFORM_LIB_DEP) $(WIRING_GLOBALS_LIB_DEP) $(CRYPTO_LIB_DEP)
LIB_DIRS += $(dir $(LIB_DEPS))

# Target this makefile is building.
ifeq ("$(ARCH)","gcc")
TARGET=exe
else
TARGET=elf bin lst hex size
endif

include ../build/arm-tlm.mk

endif
