WRITE_FILE_CREATE = $(shell echo "$(2)" > $(1))
WRITE_FILE_APPEND = $(shell echo "$(2)" >> $(1))

GAWK_VERSION := $(shell gawk --version 2>/dev/null)
ifdef GAWK_VERSION
AWK = gawk
else
AWK = awk
endif

COMMA := ,

COMMON_BUILD=../../../build
include $(COMMON_BUILD)/arm-tools.mk

ifneq (,$(PREBUILD))
# Should declare enough RAM for inermediate linker script: 128K
# FIXME: flash definitions should rely on linker file?
USER_SRAM_LENGTH = 128K
USER_PSRAM_LENGTH = 1536K
USER_FLASH_LENGTH = 1536K
else
TEXT_SECTION_LEN  = $(shell $(SIZE) --format=berkeley $(INTERMEDIATE_ELF) | $(AWK) 'NR==2 {print $$1}')
DATA_SECTION_LEN  = $(shell $(SIZE) --format=berkeley $(INTERMEDIATE_ELF) | $(AWK) 'NR==2 {print $$2}')
BSS_SECTION_LEN   = $(shell $(SIZE) --format=berkeley $(INTERMEDIATE_ELF) | $(AWK) 'NR==2 {print $$3}')

PSRAM_TEXT_SECTION_LEN  = $(shell $(OBJDUMP) -h --section=.psram_text $(INTERMEDIATE_ELF) | grep -E '.psram_text')
PSRAM_TEXT_SECTION_LEN := 0x$(word 3,$(PSRAM_TEXT_SECTION_LEN))
PSRAM_DATA_SECTION_LEN  = $(shell $(OBJDUMP) -h --section=.data_alt $(INTERMEDIATE_ELF) | grep -E '.data_alt')
PSRAM_DATA_SECTION_LEN := 0x$(word 3,$(PSRAM_DATA_SECTION_LEN))
PSRAM_BSS_SECTION_LEN  = $(shell $(OBJDUMP) -h --section=.bss_alt $(INTERMEDIATE_ELF) | grep -E '.bss_alt')
PSRAM_BSS_SECTION_LEN := 0x$(word 3,$(PSRAM_BSS_SECTION_LEN))
PSRAM_DYNALIB_SECTION_LEN  = $(shell $(OBJDUMP) -h --section=.dynalib $(INTERMEDIATE_ELF) | grep -E '.dynalib')
PSRAM_DYNALIB_SECTION_LEN := 0x$(word 3,$(PSRAM_DYNALIB_SECTION_LEN))
USER_MODULE_END = 0x$(shell $(OBJDUMP) -t $(INTERMEDIATE_ELF) | grep link_module_info_crc_end | $(AWK) '{ print $$1 }')
USER_MODULE_START = 0x$(shell $(OBJDUMP) -t $(INTERMEDIATE_ELF) | grep link_module_start | $(AWK) '{ print $$1 }')
USER_MODULE_SUFFIX_START = 0x$(shell $(OBJDUMP) -t $(INTERMEDIATE_ELF) | grep link_module_info_static_start | $(AWK) '{ print $$1 }')
MODULE_INFO_SUFFIX_LEN := ( $(USER_MODULE_END) - $(USER_MODULE_SUFFIX_START) )

# Note: reserving 16 bytes for alignment just in case
USER_SRAM_LENGTH = ( $(DATA_SECTION_LEN) + $(BSS_SECTION_LEN) + 16 )
USER_PSRAM_LENGTH = ( $(PSRAM_TEXT_SECTION_LEN) + $(PSRAM_DATA_SECTION_LEN) + $(PSRAM_BSS_SECTION_LEN) + $(PSRAM_DYNALIB_SECTION_LEN) + 16)

USER_FLASH_LENGTH = $(shell let var=($(USER_MODULE_END) - $(USER_MODULE_START) + 16); echo $$var)
USER_FLASH_LENGTH := $(shell echo $$((($(USER_FLASH_LENGTH) + 4095) / 4096 * 4096)))

$(call WRITE_FILE_APPEND, "$(MODULE_USER_MEMORY_FILE_GEN)",platform_user_part_trimmed = 1;)
$(call WRITE_FILE_APPEND, "$(MODULE_USER_MEMORY_FILE_GEN)",platform_module_info_suffix_size = $(MODULE_INFO_SUFFIX_LEN);)

all: $(INTERMEDIATE_ELF)
endif

all:
	@echo Creating $(MODULE_USER_MEMORY_FILE_GEN) ...
	$(call WRITE_FILE_APPEND, "$(MODULE_USER_MEMORY_FILE_GEN)",platform_user_part_static_ram_size = $(USER_SRAM_LENGTH);)
	$(call WRITE_FILE_APPEND, "$(MODULE_USER_MEMORY_FILE_GEN)",platform_user_part_psram_size = $(USER_PSRAM_LENGTH);)
	$(call WRITE_FILE_APPEND, "$(MODULE_USER_MEMORY_FILE_GEN)",platform_user_part_flash_size = $(USER_FLASH_LENGTH);)
