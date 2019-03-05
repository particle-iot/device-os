
# Define the prefix to this directory.
# Note: The name must be unique within this build and should be
#       based on the root of the project

HAL_INCL_ARMV7_PATH = $(TARGET_HAL_PATH)/src/armv7
HAL_INCL_NRF52840_PATH = $(TARGET_HAL_PATH)/src/nRF52840
HAL_SRC_INCL_PATH = $(TARGET_HAL_PATH)/src/nRF52840

INCLUDE_DIRS += $(HAL_INCL_NRF52840_PATH)
INCLUDE_DIRS += $(HAL_INCL_ARMV7_PATH)

ifneq (,$(findstring platform,$(DEPENDENCIES)))
INCLUDE_DIRS += $(HAL_INCL_NRF52840_PATH)
INCLUDE_DIRS += $(HAL_INCL_NRF52840_PATH)/lwip
INCLUDE_DIRS += $(HAL_INCL_NRF52840_PATH)/freertos
INCLUDE_DIRS += $(HAL_INCL_NRF52840_PATH)/openthread
INCLUDE_DIRS += $(HAL_INCL_NRF52840_PATH)/mbedtls
INCLUDE_DIRS += $(HAL_INCL_NRF52840_PATH)/littlefs
INCLUDE_DIRS += $(HAL_MODULE_PATH)/network/api
INCLUDE_DIRS += $(HAL_MODULE_PATH)/network/lwip
INCLUDE_DIRS += $(HAL_MODULE_PATH)/network/lwip/posix
INCLUDE_DIRS += $(HAL_MODULE_PATH)/network/openthread
INCLUDE_DIRS += $(HAL_MODULE_PATH)/network/lwip/wiznet
INCLUDE_DIRS += $(HAL_MODULE_PATH)/network/ncp
INCLUDE_DIRS += $(HAL_MODULE_PATH)/network/ncp/at_parser
endif

HAL_LINK ?= $(findstring hal,$(MAKE_DEPENDENCIES))

HAL_DEPS = third_party/lwip third_party/freertos third_party/openthread third_party/wiznet_driver gsm0710muxer
HAL_DEPS_INCLUDE_SCRIPTS =$(foreach module,$(HAL_DEPS),$(PROJECT_ROOT)/$(module)/import.mk)
include $(HAL_DEPS_INCLUDE_SCRIPTS)

ifneq ($(filter hal,$(LIBS)),)
HAL_LIB_DEP += $(FREERTOS_LIB_DEP) $(LWIP_LIB_DEP) $(OPENTHREAD_LIB_DEP) $(WIZNET_DRIVER_LIB_DEP) $(GSM0710MUXER_LIB_DEP)
LIBS += $(notdir $(HAL_DEPS))
endif

# if hal is used as a make dependency (linked) then add linker commands
ifneq (,$(HAL_LINK))

LINKER_FILE=$(HAL_SRC_INCL_PATH)/app_no_bootloader.ld
LINKER_DEPS=$(LINKER_FILE)

LDFLAGS += -L$(COMMON_BUILD)/arm/linker/$(STM32_DEVICE_LC)
LINKER_DEPS += $(NEWLIB_TWEAK_SPECS)
LDFLAGS += --specs=nano.specs --specs=$(NEWLIB_TWEAK_SPECS)
LDFLAGS += -T$(LINKER_FILE)
# Minimum main stack size with S140 softdevice is 1536 bytes
MAIN_STACK_SIZE = 2048
LDFLAGS += -Wl,--defsym,__STACKSIZE__=$(MAIN_STACK_SIZE)
LDFLAGS += -Wl,--defsym,__STACK_SIZE=$(MAIN_STACK_SIZE)

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
ASFLAGS += -D__STACKSIZE__=$(MAIN_STACK_SIZE) -D__STACK_SIZE=$(MAIN_STACK_SIZE)
#
endif
