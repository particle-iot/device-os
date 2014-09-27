PLATFORM_MODULE_PATH ?= ../platform

LIB_DIRS += $(BUILD_PATH_BASE)/platform
LIBS += platform

# Target specific defines
CFLAGS += -DUSE_STDPERIPH_DRIVER
CFLAGS += -DSTM32F10X_MD
CFLAGS += -DDFU_BUILD_ENABLE

ifeq ("$(USE_SWD_JTAG)","y") 
CFLAGS += -DUSE_SWD_JTAG
endif

# todo - this needs to only include modules for the target MCU and net subsystem
include $(call rwildcard,$(PLATFORM_MODULE_PATH)/,include.mk)

