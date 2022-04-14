# This file is a makefile included from the top level makefile which
# defines the sources built for the target.

# Define the prefix to this directory.
# Note: The name must be unique within this build and should be
#       based on the root of the project
TARGET_SPARK_SRC_PATH = $(TARGET_SPARK_PATH)/src

# C source files included in this build.
CSRC += $(TARGET_SPARK_SRC_PATH)/system_stm32f2xx.c
CSRC += $(TARGET_SPARK_SRC_PATH)/hw_config.c
CSRC += $(TARGET_SPARK_SRC_PATH)/flash_mal.c
CSRC += $(TARGET_SPARK_SRC_PATH)/usb_bsp.c
CSRC += $(TARGET_SPARK_SRC_PATH)/usbd_usr.c
CSRC += $(TARGET_SPARK_SRC_PATH)/usbd_composite.c
CSRC += $(TARGET_SPARK_SRC_PATH)/system_flags_impl.c
# Moved to hal, since the same symbols are also defined in the bootloader
# CSRC += $(TARGET_SPARK_SRC_PATH)/usbd_desc.c
ifeq ("$(PLATFORM_ID)","5")
CSRC += $(TARGET_SPARK_SRC_PATH)/spi_flash.c
endif
ifeq ("$(PLATFORM_ID)","7")
CSRC += $(TARGET_SPARK_SRC_PATH)/spi_flash.c
endif
ifeq ("$(PLATFORM_ID)","8")
CSRC += $(TARGET_SPARK_SRC_PATH)/spi_flash.c
endif

# USB CDC
CSRC += $(TARGET_SPARK_SRC_PATH)/usbd_mcdc.c

# USB HID
CSRC += $(TARGET_SPARK_SRC_PATH)/usbd_mhid.c

# C++ source files included in this build.
CPPSRC +=

# ASM source files included in this build.
ASRC +=

# include common sources also
include $(call rwildcard,$(PLATFORM_MCU_SHARED_STM32_PATH)/,sources.mk)
