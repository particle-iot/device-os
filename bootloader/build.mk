CFLAGS += -DUSE_STDPERIPH_DRIVER 
CFLAGS += -DSTM32F10X_MD 

ASRC += $(COMMON_BUILD)/arm/startup/startup_stm32f10x_md.S 

# Linker flags
LDFLAGS += -T$(COMMON_BUILD)/arm/linker/linker_stm32f10x_md.ld
LDFLAGS += -Wl,-Map,$(TARGET_BASE).map




