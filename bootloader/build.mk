CFLAGS += -DUSE_STDPERIPH_DRIVER

ASRC += $(COMMON_BUILD)/arm/startup/startup_$(STM32_DEVICE_LC).S
ASFLAGS += -I$(COMMON_BUILD)/arm/startup

# Linker flags
LDFLAGS += -Wl,-Map,$(TARGET_BASE).map

LINKER_DEPS += $(COMMON_BUILD)/arm/linker/module_start.ld
LINKER_DEPS += $(COMMON_BUILD)/arm/linker/module_end.ld
LINKER_DEPS += $(COMMON_BUILD)/arm/linker/module_info.ld

GLOBAL_DEFINES += BOOTLOADER_VERSION=$(BOOTLOADER_VERSION)
GLOBAL_DEFINES += MODULE_VERSION=$(BOOTLOADER_VERSION)
GLOBAL_DEFINES += MODULE_FUNCTION=$(MODULE_FUNCTION_BOOTLOADER)
GLOBAL_DEFINES += MODULE_DEPENDENCY=0,0,0
GLOBAL_DEFINES += MODULE_DEPENDENCY2=0,0,0

# select sources from platform

# import common main.c under bootloader/src
include $(BOOTLOADER_MODULE_PATH)/src/sources.mk

# import the sources from the platform
include $(call rwildcard,$(BOOTLOADER_MODULE_PATH)/src/$(PLATFORM_NAME)/,sources.mk)





