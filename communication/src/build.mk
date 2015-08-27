# This file is a makefile included from the top level makefile which
# defines the sources built for the target.

# Define the prefix to this directory.
# Note: The name must be unique within this build and should be
#       based on the root of the project
TARGET_SRC_PATH = src


# C source files included in this build.
CSRC +=

# C++ source files included in this build.
CPPSRC += $(TARGET_SRC_PATH)/coap.cpp
CPPSRC += $(TARGET_SRC_PATH)/handshake.cpp
CPPSRC += $(TARGET_SRC_PATH)/spark_protocol.cpp
CPPSRC += $(TARGET_SRC_PATH)/events.cpp
CPPSRC += $(TARGET_SRC_PATH)/spark_protocol_functions.cpp
CPPSRC += $(TARGET_SRC_PATH)/communication_dynalib.cpp
CPPSRC += $(TARGET_SRC_PATH)/dsakeygen.cpp

# ASM source files included in this build.
ASRC +=

CPPFLAGS += -std=gnu++11

