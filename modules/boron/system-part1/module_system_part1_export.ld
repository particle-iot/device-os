<<<<<<< HEAD
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
PROVIDE ( dynalib_location_hal_cellular = system_part1_module_table + 92 );
PROVIDE ( dynalib_location_hal_ble = system_part1_module_table + 96 );
PROVIDE ( dynalib_location_hal_nfc = system_part1_module_table + 100 );
PROVIDE ( dynalib_location_hal_mesh_deprecated = system_part1_module_table + 104 );
PROVIDE ( dynalib_location_hal_posix_syscall = system_part1_module_table + 108 );
PROVIDE ( dynalib_location_hal_storage = system_part1_module_table + 112 );
=======
/* Dynalib table locations */
PROVIDE (link_dynalib_location_hal                  = platform_system_part1_dynalib_table_flash_start + 0);
PROVIDE (link_dynalib_location_rt                   = platform_system_part1_dynalib_table_flash_start + 4);
PROVIDE (link_dynalib_location_system               = platform_system_part1_dynalib_table_flash_start + 8);
PROVIDE (link_dynalib_location_hal_peripherals      = platform_system_part1_dynalib_table_flash_start + 12);
PROVIDE (link_dynalib_location_hal_i2c              = platform_system_part1_dynalib_table_flash_start + 16);
PROVIDE (link_dynalib_location_hal_gpio             = platform_system_part1_dynalib_table_flash_start + 20);
PROVIDE (link_dynalib_location_hal_spi              = platform_system_part1_dynalib_table_flash_start + 24);
PROVIDE (link_dynalib_location_hal_core             = platform_system_part1_dynalib_table_flash_start + 28);
PROVIDE (link_dynalib_location_hal_socket           = platform_system_part1_dynalib_table_flash_start + 32);
PROVIDE (link_dynalib_location_hal_usart            = platform_system_part1_dynalib_table_flash_start + 36);
PROVIDE (link_dynalib_location_system_net           = platform_system_part1_dynalib_table_flash_start + 40);
PROVIDE (link_dynalib_location_system_cloud         = platform_system_part1_dynalib_table_flash_start + 44);
PROVIDE (link_dynalib_location_hal_concurrent       = platform_system_part1_dynalib_table_flash_start + 48);
PROVIDE (link_dynalib_location_hal_usb              = platform_system_part1_dynalib_table_flash_start + 52);
PROVIDE (link_dynalib_location_hal_rgbled           = platform_system_part1_dynalib_table_flash_start + 56);
PROVIDE (link_dynalib_location_hal_bootloader       = platform_system_part1_dynalib_table_flash_start + 60);
PROVIDE (link_dynalib_location_hal_dct              = platform_system_part1_dynalib_table_flash_start + 64);
PROVIDE (link_dynalib_location_communication        = platform_system_part1_dynalib_table_flash_start + 68);
PROVIDE (link_dynalib_location_services             = platform_system_part1_dynalib_table_flash_start + 72);
PROVIDE (link_dynalib_location_hal_inet             = platform_system_part1_dynalib_table_flash_start + 76);
PROVIDE (link_dynalib_location_hal_netdb            = platform_system_part1_dynalib_table_flash_start + 80);
PROVIDE (link_dynalib_location_hal_ifapi            = platform_system_part1_dynalib_table_flash_start + 84);
PROVIDE (link_dynalib_location_hal_resolvapi        = platform_system_part1_dynalib_table_flash_start + 88);
PROVIDE (link_dynalib_location_hal_cellular         = platform_system_part1_dynalib_table_flash_start + 92);
PROVIDE (link_dynalib_location_hal_ble              = platform_system_part1_dynalib_table_flash_start + 96);
PROVIDE (link_dynalib_location_hal_nfc              = platform_system_part1_dynalib_table_flash_start + 100);
PROVIDE (link_dynalib_location_hal_mesh_deprecated  = platform_system_part1_dynalib_table_flash_start + 104);
PROVIDE (link_dynalib_location_hal_posix_syscall    = platform_system_part1_dynalib_table_flash_start + 108);
>>>>>>> origin/tron-develop
