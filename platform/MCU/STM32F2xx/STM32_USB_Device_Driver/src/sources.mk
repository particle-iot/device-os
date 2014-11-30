# This file is a makefile included from the top level makefile which
# defines the sources built for the target.

# Define the prefix to this directory. 
# Note: The name must be unique within this build and should be
#       based on the root of the project
TARGET_USB_FS_SRC_PATH = $(TARGET_USB_FS_PATH)/src

# C source files included in this build.
CSRC += $(TARGET_USB_FS_SRC_PATH)/usbd_core.c
CSRC += $(TARGET_USB_FS_SRC_PATH)/usbd_req.c
CSRC += $(TARGET_USB_FS_SRC_PATH)/usbd_ioreq.c
CSRC += $(TARGET_USB_FS_SRC_PATH)/usbd_dfu_core.c
CSRC += $(TARGET_USB_FS_SRC_PATH)/usbd_dfu_mal.c
CSRC += $(TARGET_USB_FS_SRC_PATH)/usbd_flash_if.c

# C++ source files included in this build.
CPPSRC +=

# ASM source files included in this build.
ASRC +=

