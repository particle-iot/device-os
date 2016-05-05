# This file is a makefile included from the top level makefile which
# defines the sources built for the target.

# Define the prefix to this directory.
# Note: The name must be unique within this build and should be
#       based on the root of the project


TARGET_SHARED_SRC_PATH = $(PLATFORM_MCU_SHARED_STM32_PATH)/src

# C source files included in this directory.
CSRC += $(TARGET_SHARED_SRC_PATH)/hw_ticks.c
CSRC += $(TARGET_SHARED_SRC_PATH)/hw_system_flags.c


# C++ source files included in this build.
CPPSRC +=

# ASM source files included in this build.
ASRC +=
