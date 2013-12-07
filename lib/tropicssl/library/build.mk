# This file is a makefile included from the top level makefile which
# defines the sources built for the target.

# Define the prefix to this directory. 
# Note: The name must be unique within this build and should be
#       based on the root of the project
TARGET_LIB_TROPICSSL_PATH = lib/tropicssl
TARGET_LIB_TROPICSSL_LIB_PATH = $(TARGET_LIB_TROPICSSL_PATH)/library

# Add tropicssl include to all objects built for this target
INCLUDE_DIRS += $(TARGET_LIB_TROPICSSL_PATH)/include

# C source files included in this build.
CSRC += $(TARGET_LIB_TROPICSSL_LIB_PATH)/aes.c
CSRC += $(TARGET_LIB_TROPICSSL_LIB_PATH)/bignum.c
CSRC += $(TARGET_LIB_TROPICSSL_LIB_PATH)/padlock.c
CSRC += $(TARGET_LIB_TROPICSSL_LIB_PATH)/rsa.c
CSRC += $(TARGET_LIB_TROPICSSL_LIB_PATH)/sha1.c

# C++ source files included in this build.
CPPSRC += 

# ASM source files included in this build.
ASRC +=

