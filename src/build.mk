# This file is a makefile included from the top level makefile which
# defines the sources built for the target.

# Define the prefix to this directory. 
# Note: The name must be unique within this build and should be
#       based on the root of the project
TARGET_SRC_PATH = src

# Add include to all objects built for this target
INCLUDE_DIRS += inc

# C source files included in this build.
CSRC +=

# enumerate target cpp files
CPPSRC += $(call target_files,src/,*.cpp)

# ignore application.cpp if requested
ifdef NO_APPLICATION_CPP
CPPSRC := $(filter-out $(TARGET_SRC_PATH)/application.cpp,$(CPPSRC))
endif

# ASM source files included in this build.
ASRC +=

