/* System part1 static RAM */
system_part1_static_ram_start = 0x20000000 + APP_RAM_BASE;
system_part1_static_ram_size = 104K;
system_part1_static_ram_end = system_part1_static_ram_start + system_part1_static_ram_size;


/* System part1 flash */
system_part1_start = 0x30000;
system_part1_vtor_table_size = 0x200;
system_part1_module_info_size = 24;
system_part1_module_info_start = system_part1_start + system_part1_vtor_table_size;
system_part1_module_info_end = system_part1_module_info_start + system_part1_module_info_size;

/**
 * The location of the module export table.
 */
system_part1_module_table = system_part1_module_info_end;


PROVIDE ( dynalib_location_hal = system_part1_module_table + 0 );
PROVIDE ( dynalib_location_rt = system_part1_module_table + 4 );
PROVIDE ( dynalib_location_system = system_part1_module_table + 8 );
PROVIDE ( dynalib_location_hal_peripherals = system_part1_module_table + 12 );
PROVIDE ( dynalib_location_hal_i2c = system_part1_module_table + 16 );
PROVIDE ( dynalib_location_hal_gpio = system_part1_module_table + 20 );
PROVIDE ( dynalib_location_hal_spi = system_part1_module_table + 24 );
PROVIDE ( dynalib_location_hal_core = system_part1_module_table + 28 );
PROVIDE ( dynalib_location_hal_socket = system_part1_module_table + 32 );
PROVIDE ( dynalib_location_hal_usart = system_part1_module_table + 36 );
PROVIDE ( dynalib_location_system_net = system_part1_module_table + 40 );
PROVIDE ( dynalib_location_system_cloud = system_part1_module_table + 44 );
PROVIDE ( dynalib_location_hal_concurrent = system_part1_module_table + 48 );
PROVIDE ( dynalib_location_hal_usb = system_part1_module_table + 52 );
PROVIDE ( dynalib_location_hal_rgbled = system_part1_module_table + 56 );
PROVIDE ( dynalib_location_hal_bootloader = system_part1_module_table + 60 );
PROVIDE ( dynalib_location_hal_dct = system_part1_module_table + 64 );
PROVIDE ( dynalib_location_communication = system_part1_module_table + 68 );
PROVIDE ( dynalib_location_services = system_part1_module_table + 72 );
PROVIDE ( dynalib_location_hal_inet = system_part1_module_table + 76 );
PROVIDE ( dynalib_location_hal_netdb = system_part1_module_table + 80 );
PROVIDE ( dynalib_location_hal_ifapi = system_part1_module_table + 84 );
PROVIDE ( dynalib_location_hal_resolvapi = system_part1_module_table + 88 );
