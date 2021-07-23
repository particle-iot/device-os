WRITE_FILE_CREATE = $(shell echo "$(2)" > $(1))
WRITE_FILE_APPEND = $(shell echo "$(2)" >> $(1))

COMMA := ,

COMMON_BUILD=../../../build
include $(COMMON_BUILD)/arm-tools.mk

ifneq (,$(PREBUILD))
# Should declare enough RAM for inermediate linker script: 128K
USER_SRAM_LENGTH = 128K
USER_PSRAM_LENGTH = 1M
USER_FLASH_LENGTH = 2M
else
TEXT_SECTION_LEN  = $(shell $(SIZE) --format=berkeley $(INTERMEDIATE_ELF) | awk 'NR==2 {print $$1}')
DATA_SECTION_LEN  = $(shell $(SIZE) --format=berkeley $(INTERMEDIATE_ELF) | awk 'NR==2 {print $$2}')
BSS_SECTION_LEN   = $(shell $(SIZE) --format=berkeley $(INTERMEDIATE_ELF) | awk 'NR==2 {print $$3}')

PSRAM_TEXT_SECTION_LEN  = $(shell $(OBJDUMP) -h --section=.psram_text $(INTERMEDIATE_ELF) | grep -E '^\s*[0-9]+\s+\.psram_text\s+')
PSRAM_TEXT_SECTION_LEN := 0x$(word 3,$(PSRAM_TEXT_SECTION_LEN))
PSRAM_DATA_SECTION_LEN  = $(shell $(OBJDUMP) -h --section=.data_alt $(INTERMEDIATE_ELF) | grep -E '^\s*[0-9]+\s+\.data_alt\s+')
PSRAM_DATA_SECTION_LEN := 0x$(word 3,$(PSRAM_DATA_SECTION_LEN))
PSRAM_BSS_SECTION_LEN  = $(shell $(OBJDUMP) -h --section=.bss_alt $(INTERMEDIATE_ELF) | grep -E '^\s*[0-9]+\s+\.bss_alt\s+')
PSRAM_BSS_SECTION_LEN := 0x$(word 3,$(PSRAM_BSS_SECTION_LEN))
$(info $(TEXT_SECTION_LEN), $(DATA_SECTION_LEN))

# Note: reserving 16 bytes for alignment just in case
USER_SRAM_LENGTH = ( $(DATA_SECTION_LEN) + $(BSS_SECTION_LEN) + 16 )
USER_PSRAM_LENGTH = ( $(PSRAM_TEXT_SECTION_LEN) + $(PSRAM_DATA_SECTION_LEN) + $(PSRAM_BSS_SECTION_LEN) + 16)
USER_FLASH_LENGTH = ( $(TEXT_SECTION_LEN) + $(DATA_SECTION_LEN) + 16 )

$(call WRITE_FILE_APPEND, "$(MODULE_USER_MEMORY_FILE_GEN)",platform_user_part_trimmed = 1;)

all: $(INTERMEDIATE_ELF)
endif

all:
	@echo Creating $(MODULE_USER_MEMORY_FILE_GEN) ...
	$(call WRITE_FILE_APPEND, "$(MODULE_USER_MEMORY_FILE_GEN)",platform_user_part_static_ram_size = $(USER_SRAM_LENGTH);)
	$(call WRITE_FILE_APPEND, "$(MODULE_USER_MEMORY_FILE_GEN)",platform_user_part_psram_size = $(USER_PSRAM_LENGTH);)
	$(call WRITE_FILE_APPEND, "$(MODULE_USER_MEMORY_FILE_GEN)",platform_user_part_flash_size = $(USER_FLASH_LENGTH);)
