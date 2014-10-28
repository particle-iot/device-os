
# ensure defined
USRSRC += ""

# determine where user sources are, relative to project root
ifdef APP
USRSRC = applications/$(APP)
ifndef TARGET_FILE
TARGET_FILE ?= $(notdir $(APP))
TARGET_DIR ?= $(USRSRC)
endif
endif

ifdef APPDIR
USRSRC = $(APPDIR)
ifndef TARGET_FILE
TARGET_FILE ?= $(notdir $(APPDIR))
TARGET_DIR ?= $(USRSRC)
endif
endif


ifdef TEST
USRSRC = tests/$(TEST)
INCLUDE_PLATFORM?=1
ifndef TARGET_FILE
TARGET_FILE ?= $(notdir $(TEST))
TARGET_DIR ?= $(USRSRC)
endif
include $(MODULE_PATH)/tests/tests.mk
-include $(MODULE_PATH)/$(USRSRC)/test.mk
endif

# user sources specified, so override application.cpp and pull in files from 
# the user source folder
ifneq ($(USRSRC),"")
    NO_APPLICATION_CPP=1
    INCLUDE_DIRS += $(USRSRC)  # add user sources to include path
    # add C and CPP files 
    CPPSRC += $(call target_files,$(USRSRC)/,*.cpp)
    CSRC += $(call target_files,$(USRSRC)/,*.c)    
endif

TARGET_FILE ?= core-firmware

INCLUDE_DIRS += $(MODULE_PATH)/libraries

CFLAGS += -DSPARK_PLATFORM_NET=$(PLATFORM_NET)

# Linker flags
LDFLAGS += -Wl,-Map,$(TARGET_BASE).map
ifeq ("$(ARCH)","arm")

LDFLAGS += -T$(COMMON_BUILD)/arm/linker/linker_$(STM32_DEVICE_LC)_dfu.ld
LDFLAGS += --specs=nano.specs -lc -lnosys
LDFLAGS += -u _printf_float

ASRC += $(COMMON_BUILD)/arm/startup/startup_$(STM32_DEVICE_LC).S 
ASFLAGS += -I$(COMMON_BUILD)/arm/startup
ASFLAGS +=  -Wa,--defsym -Wa,SPARK_INIT_STARTUP=1

endif


