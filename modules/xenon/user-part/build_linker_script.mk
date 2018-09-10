
WRITE_FILE_CREATE =$(file >$(1),$(2))
WRITE_FILE_APPEND =$(file >>$(1),$(2))

DATA_SECTION_LEN = $(shell arm-none-eabi-objdump -h --section=.data $(TARGET_ELF) | grep .data)
DATA_SECTION_LEN := $(word 3,$(DATA_SECTION_LEN))
BSS_SECTION_LEN  = $(shell arm-none-eabi-objdump -h --section=.bss $(TARGET_ELF) | grep .bss)
BSS_SECTION_LEN  := $(word 3,$(BSS_SECTION_LEN))

COMMA := ,

all:
	@echo Generating final linker script ...
	$(call WRITE_FILE_CREATE, module_user_memory.ld,/* Memory layout constants */)
	$(call WRITE_FILE_APPEND, module_user_memory.ld,user_module_app_flash_origin = 0xD4000;)
	$(call WRITE_FILE_APPEND, module_user_memory.ld,user_module_app_flash_length = 128K;)
	$(call WRITE_FILE_APPEND, module_user_memory.ld,)
	$(call WRITE_FILE_APPEND, module_user_memory.ld,/* The SRAM Origin is system_part1_module_ram_end$(COMMA) and extends to system_static_ram_start */)
	$(call WRITE_FILE_APPEND, module_user_memory.ld,user_module_sram_origin = 0x20040000 - 0x$(DATA_SECTION_LEN) - 0x$(BSS_SECTION_LEN);)
	$(call WRITE_FILE_APPEND, module_user_memory.ld,user_module_sram_length = 0x$(DATA_SECTION_LEN) + 0x$(BSS_SECTION_LEN);)