# This file is a makefile included from the top level makefile which
# defines the sources built for the target.

# Define the prefix to this directory.
# Note: The name must be unique within this build and should be
#       based on the root of the project
TARGET_USB_FS_SRC_PATH = $(TARGET_USB_FS_PATH)/src

# C source files included in this build.
CSRC += $(TARGET_USB_FS_SRC_PATH)/usb_core.c
CSRC += $(TARGET_USB_FS_SRC_PATH)/usb_init.c
CSRC += $(TARGET_USB_FS_SRC_PATH)/usb_int.c
CSRC += $(TARGET_USB_FS_SRC_PATH)/usb_mem.c
CSRC += $(TARGET_USB_FS_SRC_PATH)/usb_regs.c
CSRC += $(TARGET_USB_FS_SRC_PATH)/usb_sil.c

# C++ source files included in this build.
CPPSRC +=

# ASM source files included in this build.
ASRC +=

