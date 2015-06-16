
# Define the prefix to this directory. 
# Note: The name must be unique within this build and should be
#       based on the root of the project
HAL_SRC_ELECTRON_INCL_PATH = $(TARGET_HAL_PATH)/src/electron

#---------------------------
# ADD FREERTOS BACK IN LATER
#---------------------------
#HAL_RTOS=FreeRTOS
#HAL_NETWORK=LwIP
#---------------------------

# if we are being compiled with platform as a dependency, then also include
# implementation headers.
ifneq (,$(findstring platform,$(DEPENDENCIES)))
INCLUDE_DIRS += $(HAL_SRC_ELECTRON_INCL_PATH)
endif

# implementation defined details for the platform that can vary
#---------------------------
# ADD FREERTOS BACK IN LATER
#---------------------------
#HAL_LIB_ELECTRON = $(HAL_SRC_ELECTRON_INCL_PATH)/lib
#HAL_LIB_RTOS = $(HAL_LIB_ELECTRON)/$(HAL_RTOS)
#HAL_RTOS_LIBS = FreeRTOS LwIP
#HAL_LIB_FILES = $(addprefix $(HAL_LIB_RTOS)/,$(addsuffix .a,$(HAL_RTOS_LIBS)))
#---------------------------

HAL_LINK ?= $(findstring hal,$(MAKE_DEPENDENCIES))

# if hal is used as a make dependency (linked) then add linker commands
ifneq (,$(HAL_LINK))
LINKER_FILE=$(HAL_SRC_ELECTRON_INCL_PATH)/app_no_bootloader.ld
LINKER_DEPS=$(LINKER_FILE)
#---------------------------
# ADD FREERTOS BACK IN LATER
#---------------------------
#LINKER_DEPS+=$(HAL_LIB_FILES)
#---------------------------
#
LDFLAGS += --specs=nano.specs -lc -lnosys
LDFLAGS += -T$(LINKER_FILE)
LDFLAGS += -Wl,--defsym,__STACKSIZE__=1400
# support for external linker file
LDFLAGS += -L$(HAL_SRC_ELECTRON_INCL_PATH)
USE_PRINTF_FLOAT ?= n
ifeq ("$(USE_PRINTF_FLOAT)","y")
LDFLAGS += -u _printf_float
endif
LDFLAGS += -Wl,-Map,$(TARGET_BASE).map
#
# assembler startup script
ASRC += $(COMMON_BUILD)/arm/startup/startup_$(STM32_DEVICE_LC).S 
ASFLAGS += -I$(COMMON_BUILD)/arm/startup
ASFLAGS +=  -Wa,--defsym -Wa,SPARK_INIT_STARTUP=1
#
endif