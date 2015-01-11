

LINKER_FILE=$(SYSTEM_PART2_MODULE_PATH)/linker.ld
LINKER_DEPS=$(LINKER_FILE)

LDFLAGS += --specs=nano.specs -lnosys
LDFLAGS += -T$(LINKER_FILE)
LDFLAGS += -Wl,--defsym,__STACKSIZE__=1400
LDFLAGS += -Wl,-Map,$(TARGET_BASE).map


SYSTEM_PART2_SRC_PATH = $(SYSTEM_PART2_MODULE_PATH)/src

CPPSRC += $(call target_files,$(SYSTEM_PART2_SRC_PATH),*.cpp)
CSRC += $(call target_files,$(SYSTEM_PART2_SRC_PATH),*.c)    

