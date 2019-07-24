
include ../../shared/stm32f2xx/part2_build.mk

# add dependency and directories for part3
LINKER_DEPS += $(SYSTEM_PART3_MODULE_PATH)/module_system_part3_export.ld
LDFLAGS += -L$(SYSTEM_PART3_MODULE_PATH)


LDFLAGS += -Wl,--defsym,__STACKSIZE__=1400
ASRC += $(COMMON_BUILD)/arm/startup/startup_$(STM32_DEVICE_LC)_electron.S
ASFLAGS += -I$(COMMON_BUILD)/arm/startup
ASFLAGS +=  -Wa,--defsym -Wa,SPARK_INIT_STARTUP=1
