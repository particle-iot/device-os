CFLAGS += -DUSE_STDPERIPH_DRIVER 

ASRC += $(COMMON_BUILD)/arm/startup/startup_$(STM32_DEVICE_LC).S 
ASFLAGS += -I$(COMMON_BUILD)/arm/startup

# Linker flags
LDFLAGS += -T$(COMMON_BUILD)/arm/linker/linker_$(STM32_DEVICE_LC).ld
LDFLAGS += -Wl,-Map,$(TARGET_BASE).map 

# select sources from platform

# import common main.c under bootloader/src
include $(BOOTLOADER_MODULE_PATH)/src/sources.mk

# import the sources from the platform
include $(call rwildcard,$(BOOTLOADER_MODULE_PATH)/src/$(PLATFORM_NAME)/,sources.mk)





