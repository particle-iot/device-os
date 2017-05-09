BOOTLOADER_SRC_ELECTRON_PATH = $(BOOTLOADER_MODULE_PATH)/src/electron
include $(BOOTLOADER_MODULE_PATH)/src/stm32f2xx/sources.mk

CSRC += $(call target_files,$(BOOTLOADER_SRC_ELECTRON_PATH)/,*.c)

CSRC += $(call target_files,$(BOOTLOADER_MODULE_PATH)/../hal/src/electron/,watchdog_hal.c)
CPPSRC += $(call target_files,$(BOOTLOADER_MODULE_PATH)/../hal/src/electron/,dct_hal.cpp)
CPPSRC += $(call target_files,$(BOOTLOADER_MODULE_PATH)/../hal/src/stm32/,newlib.cpp)

LDFLAGS += -L$(PROJECT_ROOT)/modules/electron/user-part
LINKER_DEPS += $(PROJECT_ROOT)/modules/electron/user-part/module_user_memory.ld
