# This file is a makefile included from the top level makefile which
# defines the sources built for the target.

# Define the prefix to this directory. 
# Note: The name must be unique within this build and should be
#       based on the root of the project
HAL_SRC_COREV2_PATH = $(TARGET_HAL_PATH)/src/core-v2

# if we are being compiled with platform as a dependency, then also include
# implementation headers.
ifneq (,$(findstring platform,$(DEPENDENCIES)))
INCLUDE_DIRS += $(HAL_SRC_COREV2_PATH)
endif

ifneq (,$(findstring hal,$(MAKE_DEPENDENCIES)))
# if hal is used as a make dependency (linked) then add linker commands

HAL_LIB_COREV2 = $(HAL_SRC_COREV2_PATH)/lib
HAL_WICED_LIBS += $(sort $(call rwildcard,$(HAL_LIB_COREV2)/,*.a))

WICED_MCU = $(HAL_SRC_COREV2_PATH)/wiced/platform/MCU/STM32F2xx/GCC
LDFLAGS += -Wl,--start-group $(HAL_WICED_LIBS) -Wl,--end-group
LDFLAGS += -T$(WICED_MCU)/app_no_bootloader.ld
LDFLAGS += -L$(WICED_MCU)/STM32F2x5
LDFLAGS += -Wl,--defsym,__STACKSIZE__=1400
LDFLAGS += --specs=nano.specs -lc -lnosys
LDFLAGS += -u _printf_float

endif

# not using assembler startup script, but will use startup linked in with wiced


