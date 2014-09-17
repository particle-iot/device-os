# This file is a makefile included from the top level makefile which
# defines the sources built for the target.

# Define the prefix to this directory. 
# Note: The name must be unique within this build and should be
#       based on the root of the project
TARGET_SPARK_SERVICES_PATH = SPARK_Services
TARGET_SPARK_SERVICES_SRC_PATH = $(TARGET_SPARK_SERVICES_PATH)/src

INCLUDE_DIRS += $(TARGET_SPARK_SERVICES_SRC_PATH)/inc

CSRC += $(TARGET_SPARK_SERVICES_SRC_PATH)/rgbled.c

# C++ source files included in this build.
CPPSRC +=

# ASM source files included in this build.
ASRC +=

