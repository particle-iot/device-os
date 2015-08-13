# This file is a makefile included from the top level makefile which
# defines the sources built for the target.

# Define the prefix to this directory.
# Note: The name must be unique within this build and should be
#       based on the root of the project
TARGET_STDPERIPH_SRC_PATH = $(TARGET_STDPERIPH_PATH)/src

# C source files included in this build.
CSRC += $(TARGET_STDPERIPH_SRC_PATH)/misc.c
CSRC += $(TARGET_STDPERIPH_SRC_PATH)/stm32f10x_adc.c
CSRC += $(TARGET_STDPERIPH_SRC_PATH)/stm32f10x_bkp.c
CSRC += $(TARGET_STDPERIPH_SRC_PATH)/stm32f10x_crc.c
CSRC += $(TARGET_STDPERIPH_SRC_PATH)/stm32f10x_dma.c
CSRC += $(TARGET_STDPERIPH_SRC_PATH)/stm32f10x_exti.c
CSRC += $(TARGET_STDPERIPH_SRC_PATH)/stm32f10x_flash.c
CSRC += $(TARGET_STDPERIPH_SRC_PATH)/stm32f10x_gpio.c
CSRC += $(TARGET_STDPERIPH_SRC_PATH)/stm32f10x_i2c.c
CSRC += $(TARGET_STDPERIPH_SRC_PATH)/stm32f10x_iwdg.c
CSRC += $(TARGET_STDPERIPH_SRC_PATH)/stm32f10x_pwr.c
CSRC += $(TARGET_STDPERIPH_SRC_PATH)/stm32f10x_rcc.c
CSRC += $(TARGET_STDPERIPH_SRC_PATH)/stm32f10x_rtc.c
CSRC += $(TARGET_STDPERIPH_SRC_PATH)/stm32f10x_spi.c
CSRC += $(TARGET_STDPERIPH_SRC_PATH)/stm32f10x_tim.c
CSRC += $(TARGET_STDPERIPH_SRC_PATH)/stm32f10x_usart.c

# C++ source files included in this build.
CPPSRC +=

# ASM source files included in this build.
ASRC +=

