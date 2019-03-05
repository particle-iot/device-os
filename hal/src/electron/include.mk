
# Define the prefix to this directory.
# Note: The name must be unique within this build and should be
#       based on the root of the project
HAL_SRC_ELECTRON_INCL_PATH = $(TARGET_HAL_PATH)/src/electron
HAL_INCL_STM32F2XX_PATH = $(TARGET_HAL_PATH)/src/stm32f2xx
HAL_INCL_STM32_PATH = $(TARGET_HAL_PATH)/src/stm32

# Keeps phone numbers private
# Simply add UBLOX_PHONE_NUM=2223334444 on the command line
ifdef UBLOX_PHONE_NUM
CFLAGS += -DUBLOX_PHONE_NUM='"$(UBLOX_PHONE_NUM)"'
endif

INCLUDE_DIRS += $(HAL_SRC_ELECTRON_INCL_PATH)
INCLUDE_DIRS += $(HAL_INCL_STM32F2XX_PATH)
INCLUDE_DIRS += $(HAL_INCL_STM32_PATH)

HAL_LINK ?= $(findstring hal,$(MAKE_DEPENDENCIES))

HAL_DEPS = third_party/freertos
HAL_DEPS_INCLUDE_SCRIPTS =$(foreach module,$(HAL_DEPS),$(PROJECT_ROOT)/$(module)/import.mk)
include $(HAL_DEPS_INCLUDE_SCRIPTS)

ifneq ($(filter hal,$(LIBS)),)
HAL_LIB_DEP += $(FREERTOS_LIB_DEP)
LIBS += $(notdir $(HAL_DEPS))
endif

# if hal is used as a make dependency (linked) then add linker commands
ifneq (,$(HAL_LINK))
LINKER_FILE=$(HAL_SRC_ELECTRON_INCL_PATH)/app_no_bootloader.ld
LINKER_DEPS=$(LINKER_FILE)

LDFLAGS += -L$(COMMON_BUILD)/arm/linker/stm32f2xx
LINKER_DEPS += $(NEWLIB_TWEAK_SPECS)
LDFLAGS += --specs=nano.specs --specs=$(NEWLIB_TWEAK_SPECS)
LDFLAGS += -T$(LINKER_FILE)
LDFLAGS += -Wl,--defsym,__STACKSIZE__=1400

# support for external linker file

# todo - factor out common code with photon include.mk
LDFLAGS += -L$(HAL_SRC_ELECTRON_INCL_PATH)
USE_PRINTF_FLOAT ?= n
ifeq ("$(USE_PRINTF_FLOAT)","y")
LDFLAGS += -u _printf_float
endif
LDFLAGS += -Wl,-Map,$(TARGET_BASE).map
LDFLAGS += -u uxTopUsedPriority
#
# assembler startup script
ASRC += $(COMMON_BUILD)/arm/startup/startup_$(STM32_DEVICE_LC)_electron.S
ASFLAGS += -I$(COMMON_BUILD)/arm/startup
ASFLAGS +=  -Wa,--defsym -Wa,SPARK_INIT_STARTUP=1
#
endif