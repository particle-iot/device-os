
WRITE_FILE_CREATE =$(file >$(1),$(2))
WRITE_FILE_APPEND =$(file >>$(1),$(2))

COMMA := ,

ifneq (,$(PREBUILD))
# Should declare enough RAM for inermediate linker script: 96K
USER_SRAM_LENGTH = 0x18000
else
DATA_SECTION_LEN  = $(shell arm-none-eabi-objdump -h --section=.data $(INTERMEDIATE_ELF) | grep .data)
DATA_SECTION_LEN := 0x$(word 3,$(DATA_SECTION_LEN))
BSS_SECTION_LEN   = $(shell arm-none-eabi-objdump -h --section=.bss $(INTERMEDIATE_ELF) | grep .bss)
BSS_SECTION_LEN  := 0x$(word 3,$(BSS_SECTION_LEN))

USER_SRAM_LENGTH = ( $(DATA_SECTION_LEN) + $(BSS_SECTION_LEN) )

all: $(INTERMEDIATE_ELF)
endif

# 8K is the backup ram length plusing the stack length
all:
	@echo Creating module_user_memory.ld ...
	$(call WRITE_FILE_APPEND, module_user_memory.ld,user_module_app_flash_origin = 0xD4000;)
	$(call WRITE_FILE_APPEND, module_user_memory.ld,user_module_app_flash_length = 128K;)
	$(call WRITE_FILE_APPEND, module_user_memory.ld,)
	$(call WRITE_FILE_APPEND, module_user_memory.ld,user_module_sram_origin = 0x20040000 - 8K - $(USER_SRAM_LENGTH);)
	$(call WRITE_FILE_APPEND, module_user_memory.ld,user_module_sram_length = 8K + $(USER_SRAM_LENGTH);)


