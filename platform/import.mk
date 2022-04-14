# this imports the various paths provided by this module into another module
# note that MODULE is not set to this module but the importing module

PLATFORM_MODULE_NAME = platform
PLATFORM_MODULE_PATH ?= $(PROJECT_ROOT)/$(PLATFORM_MODULE_NAME)
PLATFORM_BUILD_PATH_EXT = $(BUILD_TARGET_PLATFORM)

PLATFORM_LIB_DIR = $(BUILD_PATH_BASE)/$(PLATFORM_MODULE_NAME)/$(PLATFORM_BUILD_PATH_EXT)
PLATFORM_LIB_DEP = $(PLATFORM_LIB_DIR)/lib$(PLATFORM_MODULE_NAME).a


# Target specific defines
CFLAGS += -DUSE_STDPERIPH_DRIVER
CFLAGS += -DDFU_BUILD_ENABLE

ifeq ("$(USE_SWD_JTAG)","y")
CFLAGS += -DUSE_SWD_JTAG
endif

ifeq ("$(USE_SWD)","y")
CFLAGS += -DUSE_SWD
endif

# pull in the includes/sources corresponding to the target platform

INCLUDE_DIRS += $(PLATFORM_MODULE_PATH)/shared/inc
PLATFORM_MCU_PATH=$(PLATFORM_MODULE_PATH)/MCU/$(PLATFORM_MCU)
PLATFORM_NET_PATH=$(PLATFORM_MODULE_PATH)/NET/$(PLATFORM_NET)
include $(call rwildcard,$(PLATFORM_MCU_PATH)/,include.mk)
include $(call rwildcard,$(PLATFORM_NET_PATH)/,include.mk)
