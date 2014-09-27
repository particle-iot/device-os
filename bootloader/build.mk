CFLAGS += -DUSE_STDPERIPH_DRIVER 
CFLAGS += -DSTM32F10X_MD 

# Linker flags
LDFLAGS += -T$(MODULE_PATH)/linker/linker_stm32f10x_md.ld
LDFLAGS += -Wl,-Map,$(TARGET_BASE).map




