# This file is a makefile included from the top level makefile which
# defines the sources built for the target.

# Define the prefix to this directory.
# Note: The name must be unique within this build and should be
#       based on the root of the project
TARGET_SRC_PATH = src

# C source files included in this build.
CSRC +=

# enumerate target cpp files
CPPSRC += $(call target_files,src/,*.cpp)

# ASM source files included in this build.
ASRC +=

BUILTINS_EXCLUDE = malloc free realloc
CFLAGS += $(addprefix -fno-builtin-,$(BUILTINS_EXCLUDE))

LOG_MODULE_CATEGORY = wiring
