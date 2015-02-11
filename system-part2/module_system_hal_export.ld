
system_part2_module_table = 0x8020190;

PROVIDE ( dynalib_location_services = system_part2_module_table + 0 );
PROVIDE ( dynalib_location_hal = system_part2_module_table + 4 );
PROVIDE ( dynalib_location_rt = system_part2_module_table + 8 );