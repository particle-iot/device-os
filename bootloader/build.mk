CFLAGS += -DUSE_STDPERIPH_DRIVER 

ASRC += $(COMMON_BUILD)/arm/startup/startup_$(STM32_DEVICE_LC).S 
ASFLAGS += -I$(COMMON_BUILD)/arm/startup

# Linker flags
LDFLAGS += -T$(COMMON_BUILD)/arm/linker/linker_$(STM32_DEVICE_LC).ld
LDFLAGS += -Wl,-Map,$(TARGET_BASE).map





