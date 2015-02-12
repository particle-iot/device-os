

LINKER_FILE=$(USER_PART_MODULE_PATH)/linker.ld
LINKER_DEPS += $(LINKER_FILE)
LINKER_DEPS += $(SYSTEM_PART2_MODULE_PATH)/module_system_hal_export.ld 

LDFLAGS += --specs=nano.specs -lnosys
LDFLAGS += -L$(SYSTEM_PART2_MODULE_PATH)
LDFLAGS += -L$(USER_PART_MODULE_PATH)
LDFLAGS += -T$(LINKER_FILE)
LDFLAGS += -Wl,--defsym,__STACKSIZE__=1400
LDFLAGS += -Wl,-Map,$(TARGET_BASE).map

ifeq ("$(USE_PRINTF_FLOAT)","y")
LDFLAGS += -u _printf_float
endif

CPPSRC += $(call target_files,$(USER_PART_MODULE_PATH)/src/,*.cpp)
CSRC += $(call target_files,$(USER_PART_MODULE_PATH)/src/,*.c)