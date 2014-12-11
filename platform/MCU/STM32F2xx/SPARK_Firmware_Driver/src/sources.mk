# This file is a makefile included from the top level makefile which
# defines the sources built for the target.

# Define the prefix to this directory. 
# Note: The name must be unique within this build and should be
#       based on the root of the project
TARGET_SPARK_SRC_PATH = $(TARGET_SPARK_PATH)/src

INCLUDE_DIRS += SPARK_Services/inc

# C source files included in this build.
CSRC += $(TARGET_SPARK_SRC_PATH)/system_stm32f2xx.c
CSRC += $(TARGET_SPARK_SRC_PATH)/hw_config.c
CSRC += $(TARGET_SPARK_SRC_PATH)/usb_bsp.c
CSRC += $(TARGET_SPARK_SRC_PATH)/usbd_usr.c
CSRC += $(TARGET_SPARK_SRC_PATH)/usbd_desc.c
ifeq ("$(SPARK_PRODUCT_ID)","5")
CSRC += $(TARGET_SPARK_SRC_PATH)/spi_flash.c
endif

# C++ source files included in this build.
CPPSRC +=

# ASM source files included in this build.
ASRC +=


