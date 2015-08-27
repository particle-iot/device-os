# This file is a makefile included from the top level makefile which
# defines the sources built for the target.

# Define the prefix to this directory.
# Note: The name must be unique within this build and should be
#       based on the root of the project
TARGET_SPARK_SRC_PATH = $(TARGET_SPARK_PATH)/src

INCLUDE_DIRS += SPARK_Services/inc

# C source files included in this build.
CSRC += $(TARGET_SPARK_SRC_PATH)/cc3000_spi.c
CSRC += $(TARGET_SPARK_SRC_PATH)/hw_config.c
CSRC += $(TARGET_SPARK_SRC_PATH)/sst25vf_spi.c
CSRC += $(TARGET_SPARK_SRC_PATH)/system_stm32f10x.c
CSRC += $(TARGET_SPARK_SRC_PATH)/spi_bus.c
CSRC += $(TARGET_SPARK_SRC_PATH)/usb_endp.c
CSRC += $(TARGET_SPARK_SRC_PATH)/usb_pwr.c

# C++ source files included in this build.
CPPSRC +=

# ASM source files included in this build.
ASRC +=


# include common sources also
include $(call rwildcard,$(PLATFORM_MCU_SHARED_STM32_PATH)/,sources.mk)

