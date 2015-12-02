# This file is a makefile included from the top level makefile which
# defines the sources built for the target.

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

