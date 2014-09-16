# This file is a makefile included from the top level makefile which
# defines the sources built for the target.

# Define the prefix to this directory. 
# Note: The name must be unique within this build and should be
#       based on the root of the project
TARGET_HAL_PATH = hal
TARGET_HAL_SRC_PATH = $(TARGET_HAL_PATH)/src

INCLUDE_DIRS += $(TARGET_SPARK_HAL_PATH)/inc

# C source files included in this build.
CSRC += $(TARGET_HAL_SRC_PATH)/pinmap_hal.c
CSRC += $(TARGET_HAL_SRC_PATH)/gpio_hal.c
CSRC += $(TARGET_HAL_SRC_PATH)/adc_hal.c
CSRC += $(TARGET_HAL_SRC_PATH)/pwm_hal.c
CSRC += $(TARGET_HAL_SRC_PATH)/timer_hal.c
CSRC += $(TARGET_HAL_SRC_PATH)/rtc_hal.c

# C++ source files included in this build.
CPPSRC +=

# ASM source files included in this build.
ASRC +=

