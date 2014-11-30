# This file is a makefile included from the top level makefile which
# defines the sources built for the target.

# Define the prefix to this directory. 
# Note: The name must be unique within this build and should be
#       based on the root of the project
HAL_SRC_COREV1_PATH = $(TARGET_HAL_PATH)/src/core-v1

# if we are being compiled with platform as a dependency, then also include
# implementation headers.
ifneq (,$(findstring platform,$(DEPENDENCIES)))
INCLUDE_DIRS += $(HAL_SRC_COREV1_PATH)
endif


ifneq (,$(findstring hal,$(MAKE_DEPENDENCIES)))

LDFLAGS += -T$(COMMON_BUILD)/arm/linker/linker_$(STM32_DEVICE_LC)_dfu.ld
LDFLAGS += --specs=nano.specs -lc -lnosys
LDFLAGS += -u _printf_float
LDFLAGS += -Wl,-Map,$(TARGET_BASE).map

ASRC += $(COMMON_BUILD)/arm/startup/startup_$(STM32_DEVICE_LC).S 
ASFLAGS += -I$(COMMON_BUILD)/arm/startup
ASFLAGS +=  -Wa,--defsym -Wa,SPARK_INIT_STARTUP=1

endif
