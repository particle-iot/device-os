# This file is a makefile included from the top level makefile which
# defines the sources built for the target.

# Define the prefix to this directory. 
# Note: The name must be unique within this build and should be
#       based on the root of the project
HAL_SRC_GCC_PATH = $(TARGET_HAL_PATH)/src/gcc

# C source files included in this build.
CSRC += $(call target_files,$(HAL_SRC_GCC_PATH)/,*.c)


# C++ source files included in this build.
CPPSRC += $(call target_files,$(HAL_SRC_GCC_PATH)/,*.cpp)

# WIP - CPPSRC += $(HAL_SRC_TEMPLATE_PATH)/memory_hal.cpp

# ASM source files included in this build.
ASRC +=

