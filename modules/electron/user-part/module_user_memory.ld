/* Memory layout constants */

user_module_app_flash_origin = 0x08080000;
user_module_app_flash_length = 128K;

/* The SRAM Origin is system_part1_module_ram_end, and extends to system_static_ram_start */
user_module_sram_origin = 0x20000400;
user_module_sram_length = 0x20000 - 0x400 - 16K;
