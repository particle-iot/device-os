# This file is a makefile included from the top level makefile which
# defines the sources built for the target.

# Define the prefix to this directory. 
# Note: The name must be unique within this build and should be
#       based on the root of the project
HAL_SRC_COREV1_PATH = $(TARGET_HAL_PATH)/src/core-v1

INCLUDE_DIRS += $(HAL_SRC_COREV1_PATH)

# C source files included in this build.
CSRC += $(HAL_SRC_COREV1_PATH)/adc_hal.c
CSRC += $(HAL_SRC_COREV1_PATH)/core_hal.c
CSRC += $(HAL_SRC_COREV1_PATH)/core_subsys_hal.c
CSRC += $(HAL_SRC_COREV1_PATH)/delay_hal.c
CSRC += $(HAL_SRC_COREV1_PATH)/eeprom_hal.c
CSRC += $(HAL_SRC_COREV1_PATH)/gpio_hal.c
CSRC += $(HAL_SRC_COREV1_PATH)/i2c_hal.c
CSRC += $(HAL_SRC_COREV1_PATH)/inet_hal.c
CSRC += $(HAL_SRC_COREV1_PATH)/interrupts_hal.c
CSRC += $(HAL_SRC_COREV1_PATH)/ota_flash_hal.c
CSRC += $(HAL_SRC_COREV1_PATH)/pinmap_hal.c
CSRC += $(HAL_SRC_COREV1_PATH)/pwm_hal.c
CSRC += $(HAL_SRC_COREV1_PATH)/rtc_hal.c
CSRC += $(HAL_SRC_COREV1_PATH)/servo_hal.c
CSRC += $(HAL_SRC_COREV1_PATH)/socket_hal.c
CSRC += $(HAL_SRC_COREV1_PATH)/spi_hal.c
CSRC += $(HAL_SRC_COREV1_PATH)/timer_hal.c
CSRC += $(HAL_SRC_COREV1_PATH)/tone_hal.c
CSRC += $(HAL_SRC_COREV1_PATH)/usart_hal.c
CSRC += $(HAL_SRC_COREV1_PATH)/usb_hal.c
CSRC += $(HAL_SRC_COREV1_PATH)/wlan_hal.c

# C++ source files included in this build.
CPPSRC += $(HAL_SRC_COREV1_PATH)/deviceid_hal.cpp
# WIP - CPPSRC += $(HAL_SRC_COREV1_PATH)/memory_hal.cpp

# ASM source files included in this build.
ASRC +=

