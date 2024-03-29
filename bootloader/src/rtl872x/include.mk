INCLUDE_DIRS += $(BOOTLOADER_MODULE_PATH)/src/rtl872x

MAIN_STACK_SIZE = 8192

ASFLAGS += -D__STACKSIZE__=$(MAIN_STACK_SIZE) -D__STACK_SIZE=$(MAIN_STACK_SIZE)

LDFLAGS += -L$(COMMON_BUILD)/arm/linker/rtl872x
LDFLAGS += -Wl,--defsym,__STACKSIZE__=$(MAIN_STACK_SIZE)
LDFLAGS += -Wl,--defsym,__STACK_SIZE=$(MAIN_STACK_SIZE)
