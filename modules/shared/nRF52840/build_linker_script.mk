WRITE_FILE_CREATE = $(shell echo "$(2)" > $(1))
WRITE_FILE_APPEND = $(shell echo "$(2)" >> $(1))

COMMA := ,

COMMON_BUILD=../../../build
include $(COMMON_BUILD)/common-tools.mk
include $(COMMON_BUILD)/arm-tools.mk
include $(COMMON_BUILD)/macros.mk

ifneq (,$(PREBUILD))
# Should declare enough RAM for inermediate linker script: 89K
USER_SRAM_LENGTH = 89K
else
DATA_SECTION_LEN  = $(call get_section_size_with_alignment,.data,$(INTERMEDIATE_ELF))
BSS_SECTION_LEN   = $(call get_section_size_with_alignment,.bss,$(INTERMEDIATE_ELF))

USER_SRAM_LENGTH = ( $(DATA_SECTION_LEN) + $(BSS_SECTION_LEN) )

all: $(INTERMEDIATE_ELF)
endif

all:
	@echo Creating $(MODULE_USER_MEMORY_FILE_GEN) ...
	$(call WRITE_FILE_APPEND, "$(MODULE_USER_MEMORY_FILE_GEN)",platform_user_part_static_ram_size = $(USER_SRAM_LENGTH);)
