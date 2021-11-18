/* Dynalib table locations */
PROVIDE (link_part1_flash_start                 = platform_km0_part1_flash_start);
PROVIDE (link_part1_flash_end                   = platform_km0_part1_flash_end);
PROVIDE (link_part1_module_info_flash_start     = platform_km0_part1_module_info_flash_start);
PROVIDE (link_part1_dynalib_table_flash_start   = platform_km0_part1_dynalib_table_flash_start);
PROVIDE (link_part1_dynalib_table_ram_start     = platform_km0_part1_dynalib_table_ram_start);

PROVIDE (link_dynalib_location_offset_part1_preinit = 0);
PROVIDE (link_dynalib_location_offset_part1 = 4);
PROVIDE (link_dynalib_location_offset_ipc = 8);
