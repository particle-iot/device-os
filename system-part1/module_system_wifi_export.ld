
system_part1_module_table = 0x8040000;

/**
 * The end of the statically allocated region
 */
system_part1_module_ram_end = 0x20000100;

PROVIDE ( dynalib_location_communication = system_part1_module_table + 0 );
PROVIDE ( dynalib_location_wifi_resource = system_part1_module_table + 4 );
PROVIDE ( dynalib_location_system_module_part1 = system_part1_module_table + 8 );

