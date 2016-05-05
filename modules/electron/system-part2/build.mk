
include ../../shared/stm32f2xx/part2_build.mk

LDFLAGS += -Wl,--defsym,__STACKSIZE__=1400
ASRC += $(COMMON_BUILD)/arm/startup/startup_$(STM32_DEVICE_LC).S
ASFLAGS += -I$(COMMON_BUILD)/arm/startup
ASFLAGS +=  -Wa,--defsym -Wa,SPARK_INIT_STARTUP=1
