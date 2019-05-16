# This file is a makefile included from the top level makefile which
# defines the sources built for the target.

# Define the prefix to this directory.
# Note: The name must be unique within this build and should be
#       based on the root of the project
TARGET_SRC_PATH = src

# C source files included in this build.
CSRC += $(call target_files,src/,*.c)

# enumerate target cpp files
CPPSRC += $(call target_files,src/,*.cpp)

# ASM source files included in this build.
ASRC +=

ifeq ($(PLATFORM_ID),6)
CFLAGS += -DLOG_COMPILE_TIME_LEVEL=LOG_LEVEL_NONE
endif

ifeq ($(PLATFORM_ID),8)
CFLAGS += -DLOG_COMPILE_TIME_LEVEL=LOG_LEVEL_NONE
endif

INCLUDE_DIRS += $(TARGET_SRC_PATH)
INCLUDE_DIRS += $(TARGET_SRC_PATH)/control/proto

LOG_MODULE_CATEGORY = system

ifneq (,$(filter $(PLATFORM_ID),13 23))
ifneq ($(DEBUG_BUILD),y)
ifneq ($(HYBRID_BUILD),y)
CFLAGS += -DLOG_COMPILE_TIME_LEVEL=LOG_LEVEL_WARN
endif
endif
endif
