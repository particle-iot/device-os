
system_part3_start = 0x8060000;

system_part3_ram_end = 0x20020000 - 12K;
system_part3_ram_start = 0x20020000 - 16K;

system_part3_module_info_size = 24;

system_part3_module_info_start = system_part3_start;

system_part3_module_info_end = system_part3_module_info_start + system_part3_module_info_size;

/* static ram for system modules starts with part3, then part2 (and above that the stack) */
system_static_ram_start = system_part3_ram_start;

/**
 * The location of the module export table.
 */
system_part3_module_table = system_part3_module_info_end;


/**
 * The dyanmic libraries exported by this module.
 */
PROVIDE ( dynalib_location_system_module_part3 = system_part3_module_table + 0 );
PROVIDE ( dynalib_location_hal_usb = system_part3_module_table + 4 );
PROVIDE ( dynalib_location_hal_cellular = system_part3_module_table + 8 );
PROVIDE ( dynalib_location_hal_socket = system_part3_module_table + 12 );
PROVIDE ( dynalib_location_hal_bootloader = system_part3_module_table + 16 );
PROVIDE ( dynalib_location_services2 = system_part3_module_table + 20 );
