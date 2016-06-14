
system_part3_start = 0x8060000;

system_part3_module_ram_end = 0x201FD800 /* 0x20200000-10K */;


system_part3_module_info_size = 24;

system_part3_module_info_start = system_part3_start;

system_part3_module_info_end = system_part3_module_info_start + system_part3_module_info_size;


/**
 * The location of the module export table.
 */
system_part3_module_table = system_part3_module_info_end;


/**
 * The dyanmic libraries exported by this module.
 */
PROVIDE ( dynalib_location_system_module_part3 = system_part3_module_table + 0 );
PROVIDE ( dynalib_location_hal_usb = system_part3_module_table + 4 );

