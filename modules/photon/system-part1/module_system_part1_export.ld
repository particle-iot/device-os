
system_part1_module_table = 0x8060000;

/**
 * The end of the statically allocated region.
 * This is much more than we need but ensures the RAM vtor table in part2 is
 * 512-byte aligned.
 */
system_part1_module_ram_end = 0x20000200;

PROVIDE ( dynalib_location_communication = system_part1_module_table + 0 );
PROVIDE ( dynalib_location_wifi_resource = system_part1_module_table + 4 );
PROVIDE ( dynalib_location_system_module_part1 = system_part1_module_table + 8 );

