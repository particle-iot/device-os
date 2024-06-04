MAIN_STACK_SIZE = 4096

# # FIXME
# ifeq ("$(MODULE)","prebootloader-part1")
# EXTRA_DEPS = third_party/freertos
# EXTRA_DEPS_INCLUDE_SCRIPTS =$(foreach module,$(EXTRA_DEPS),$(PROJECT_ROOT)/$(module)/import.mk)
# include $(EXTRA_DEPS_INCLUDE_SCRIPTS)

# EXTRA_LIB_DEP += $(FREERTOS_LIB_DEP)
# LIBS += $(notdir $(EXTRA_DEPS))
# LIB_DIRS += $(FREERTOS_LIB_DIR)
# endif

ASFLAGS += -D__STACKSIZE__=$(MAIN_STACK_SIZE) -D__STACK_SIZE=$(MAIN_STACK_SIZE)

LDFLAGS += -L$(COMMON_BUILD)/arm/linker/rtl872x
LDFLAGS += -L$(BOOTLOADER_SHARED_MODULAR)
LDFLAGS += -Wl,--defsym,__STACKSIZE__=$(MAIN_STACK_SIZE)
LDFLAGS += -Wl,--defsym,__STACK_SIZE=$(MAIN_STACK_SIZE)
LDFLAGS += -u uxTopUsedPriority
