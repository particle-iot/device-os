
# Define the prefix to this directory.
# Note: The name must be unique within this build and should be
#       based on the root of the project
HAL_SRC_INCL_PATH = $(TARGET_HAL_PATH)/src/$(PLATFORM)
HAL_INCL_ARMV7_PATH = $(TARGET_HAL_PATH)/src/armv7
HAL_INCL_NRF52840_PATH = $(TARGET_HAL_PATH)/src/nRF52840

INCLUDE_DIRS += $(HAL_SRC_INCL_PATH)
INCLUDE_DIRS += $(HAL_INCL_NRF52840_PATH)
INCLUDE_DIRS += $(HAL_INCL_ARMV7_PATH)

ifneq (,$(findstring platform,$(DEPENDENCIES)))
INCLUDE_DIRS += $(HAL_INCL_NRF52840_PATH)
INCLUDE_DIRS += $(HAL_SRC_INCL_PATH)/lwip
INCLUDE_DIRS += $(HAL_SRC_INCL_PATH)/freertos
INCLUDE_DIRS += $(HAL_SRC_INCL_PATH)/openthread
INCLUDE_DIRS += $(HAL_SRC_INCL_PATH)/mbedtls
INCLUDE_DIRS += $(HAL_MODULE_PATH)/network/lwip
INCLUDE_DIRS += $(HAL_MODULE_PATH)/network/openthread
endif

HAL_LINK ?= $(findstring hal,$(MAKE_DEPENDENCIES))

HAL_DEPS = third_party/lwip third_party/freertos third_party/openthread third_party/nrf_802154
HAL_DEPS_INCLUDE_SCRIPTS =$(foreach module,$(HAL_DEPS),$(PROJECT_ROOT)/$(module)/import.mk)
include $(HAL_DEPS_INCLUDE_SCRIPTS)

# if hal is used as a make dependency (linked) then add linker commands
ifneq (,$(HAL_LINK))

HAL_LIB_DEP += $(FREERTOS_LIB_DEP) $(LWIP_LIB_DEP) $(OPENTHREAD_LIB_DEP) $(NRF_802154_LIB_DEP)
LIBS += $(notdir $(HAL_DEPS))

LINKER_FILE=$(HAL_SRC_INCL_PATH)/app_no_bootloader.ld
LINKER_DEPS=$(LINKER_FILE)

LDFLAGS += -L$(COMMON_BUILD)/arm/linker/$(STM32_DEVICE_LC)
LINKER_DEPS += $(NEWLIB_TWEAK_SPECS)
LDFLAGS += --specs=nano.specs --specs=$(NEWLIB_TWEAK_SPECS)
LDFLAGS += -T$(LINKER_FILE)
LDFLAGS += -Wl,--defsym,__STACKSIZE__=1400

# support for external linker file

# todo - factor out common code with photon include.mk
LDFLAGS += -L$(HAL_SRC_INCL_PATH)
USE_PRINTF_FLOAT ?= n
ifeq ("$(USE_PRINTF_FLOAT)","y")
LDFLAGS += -u _printf_float
endif
LDFLAGS += -Wl,-Map,$(TARGET_BASE).map
LDFLAGS += -u uxTopUsedPriority
#
# assembler startup script
ASRC += $(COMMON_BUILD)/arm/startup/startup_$(STM32_DEVICE_LC).S
ASFLAGS += -I$(COMMON_BUILD)/arm/startup
ASFLAGS +=  -Wa,--defsym -Wa,SPARK_INIT_STARTUP=0
#
endif
