# This file is a makefile included from the top level makefile which
# defines the sources built for the target.

# Define the prefix to this directory.
# Note: The name must be unique within this build and should be
#       based on the root of the project
TARGET_NEW_HAL_MCU_SRC = $(PLATFORM_MCU_PATH)/src

CSRC += $(TARGET_NEW_HAL_MCU_SRC)/hw_config.c

# C++ source files included in this build.
CPPSRC +=

# ASM source files included in this build.
ASRC +=


