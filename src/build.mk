# This file is a makefile included from the top level makefile which
# defines the sources built for the target.

# Define the prefix to this directory. 
# Note: The name must be unique within this build and should be
#       based on the root of the project
TARGET_SRC_PATH = src

# Add tropicssl include to all objects built for this target
INCLUDE_DIRS += $(TARGET_SRC_PATH)

# C source files included in this build.
CSRC += 

# C++ source files included in this build.
CPPSRC += $(TARGET_SRC_PATH)/coap.cpp
CPPSRC += $(TARGET_SRC_PATH)/handshake.cpp
CPPSRC += $(TARGET_SRC_PATH)/spark_protocol.cpp

# ASM source files included in this build.
ASRC +=

