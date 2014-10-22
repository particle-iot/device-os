CFLAGS += -DUSE_STDPERIPH_DRIVER 

ASRC += $(COMMON_BUILD)/arm/startup/startup_$(STM32_DEVICE_LC).S 
ASFLAGS += -I$(COMMON_BUILD)/arm/startup

# Linker flags
LDFLAGS += -T$(COMMON_BUILD)/arm/linker/linker_$(STM32_DEVICE_LC).ld
LDFLAGS += -Wl,-Map,$(TARGET_BASE).map

# select sources from platform

#un comment if the C code under bootloader/src is also needed
# include $(BOOTLOADER_MODULE_PATH)/src/sources.mk

# import the sources from the platform
include $(call rwildcard,$(BOOTLOADER_MODULE_PATH)/src/$(PLATFORM_NAME)/,sources.mk)





