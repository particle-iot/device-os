

LINKER_FILE=$(SYSTEM_PART2_MODULE_PATH)/linker.ld
LINKER_DEPS += $(LINKER_FILE) $(HAL_WICED_LIB_FILES) 

LINKER_DEPS += $(SYSTEM_PART2_MODULE_PATH)/module_system_hal_export.ld 
LINKER_DEPS += $(WIFI_SYSTEM_MODULE_PATH)/module_system_wifi_export.ld

LDFLAGS += --specs=nano.specs -lnosys
LDFLAGS += -Wl,--whole-archive $(HAL_WICED_LIB_FILES) -Wl,--no-whole-archive
LDFLAGS += -L$(WIFI_SYSTEM_MODULE_PATH)
LDFLAGS += -T$(LINKER_FILE)
LDFLAGS += -Wl,-Map,$(TARGET_BASE).map

ifeq ("$(USE_PRINTF_FLOAT)","y")
LDFLAGS += -u _printf_float
endif
LDFLAGS += -Wl,-Map,$(TARGET_BASE).map


SYSTEM_PART2_SRC_PATH = $(SYSTEM_PART2_MODULE_PATH)/src

CPPSRC += $(call target_files,$(SYSTEM_PART2_SRC_PATH),*.cpp)
CSRC += $(call target_files,$(SYSTEM_PART2_SRC_PATH),*.c)    

