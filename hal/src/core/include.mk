# This file is a makefile included from the top level makefile which
# defines the sources built for the target.

# Define the prefix to this directory.
# Note: The name must be unique within this build and should be
#       based on the root of the project
HAL_SRC_COREV1_PATH = $(TARGET_HAL_PATH)/src/core

INCLUDE_DIRS += $(HAL_SRC_COREV1_PATH)

INCLUDE_DIRS += $(TARGET_HAL_PATH)/src/stm32

ifneq (,$(findstring hal,$(MAKE_DEPENDENCIES)))

LDFLAGS += -Tlinker_$(STM32_DEVICE_LC)_dfu.ld
LDFLAGS += -L$(COMMON_BUILD)/arm/linker
LDFLAGS += --specs=nano.specs -lc -lnosys
USE_PRINTF_FLOAT ?= y
ifeq ("$(USE_PRINTF_FLOAT)","y")
LDFLAGS += -u _printf_float
endif
LDFLAGS += -Wl,-Map,$(TARGET_BASE).map

ASRC += $(COMMON_BUILD)/arm/startup/startup_$(STM32_DEVICE_LC).S
ASFLAGS += -I$(COMMON_BUILD)/arm/startup
ASFLAGS +=  -Wa,--defsym -Wa,SPARK_INIT_STARTUP=1

endif
