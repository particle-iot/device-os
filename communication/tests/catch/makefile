
MODULE=communication_catch_tests
COMMUNICATION_CATCH_TESTS_MODULE_PATH=.
PROJECT_ROOT=../../..

export PLATFORM_ID=3
export ARCH=gcc
include $(PROJECT_ROOT)/build/top-level-module.mk
include $(PROJECT_ROOT)/build/macros.mk


# Target this makefile is building.
TARGET_TYPE = a

BUILD_PATH_EXT=$(COMMUNICATION_CATCH_TESTS_BUILD_PATH_EXT)

DEPENDENCIES = dynalib hal services wiring crypto communication

MAKE_DEPENDENCIES = services crypto communication

TARGET=exe

GLOBAL_DEFINES += MODULE_FUNCTION=$(MODULE_FUNCTION_MONO_FIRMWARE)
GLOBAL_DEFINES += MODULE_DEPENDENCY=0,0,0
GLOBAL_DEFINES += MODULE_DEPENDENCY2=0,0,0

LIBS += $(MAKE_DEPENDENCIES)
LIB_DEPS += $(SERVICES_LIB_DEP) $(CRYPTO_LIB_DEP) $(COMMUNICATION_LIB_DEP)
LIB_DIRS += $(dir $(LIB_DEPS))


include $(PROJECT_ROOT)/build/arm-tlm.mk
