# This file is a makefile included from the top level makefile which
# defines the sources built for the target.

# Define the prefix to this directory. 
# Note: The name must be unique within this build and should be
#       based on the root of the project
TARGET_CC3000_PATH = CC3000_Host_Driver

# Add tropicssl include to all objects built for this target
INCLUDE_DIRS += $(TARGET_CC3000_PATH)

# C source files included in this build.
CSRC += $(TARGET_CC3000_PATH)/cc3000_common.c
CSRC += $(TARGET_CC3000_PATH)/evnt_handler.c
CSRC += $(TARGET_CC3000_PATH)/hci.c
CSRC += $(TARGET_CC3000_PATH)/netapp.c
CSRC += $(TARGET_CC3000_PATH)/nvmem.c
CSRC += $(TARGET_CC3000_PATH)/security.c
CSRC += $(TARGET_CC3000_PATH)/socket.c
CSRC += $(TARGET_CC3000_PATH)/wlan.c

# C++ source files included in this build.
CPPSRC +=

# ASM source files included in this build.
ASRC +=

