
/**
 * The load address of the part1 module image
 */
system_part1_start = 0x8020000;

/**
 * The end of the statically allocated region.
 * This is much more than we need but ensures the RAM vtor table in part2 is
 * 512-byte aligned.
 */
system_part1_module_ram_end = 0x20000300;

/**
 * A fake VTOR table so that the bootloader can jump to this module and set up the stack,
 * plus that the module_info resides at the correct offset.
*/
system_part1_boot_table_size = 0x184;

system_part1_module_info_size = 24;

system_part1_boot_table_start = system_part1_start;

system_part1_module_info_start = system_part1_start + system_part1_boot_table_size;

system_part1_module_info_end = system_part1_module_info_start + system_part1_module_info_size;

/**
 * The location of the module export table.
 */
system_part1_module_table = system_part1_module_info_end;


PROVIDE ( dynalib_location_communication = system_part1_module_table + 0 );
PROVIDE ( dynalib_location_wifi_resource = system_part1_module_table + 4 );
PROVIDE ( dynalib_location_system_module_part1 = system_part1_module_table + 8 );
PROVIDE ( dynalib_location_services = system_part1_module_table + 12 );

