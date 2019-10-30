link_dynamic_reset_handler_location = platform_system_part2_vector_table_flash_start + 4;

/* Dynalib table locations */
PROVIDE (link_dynalib_location_hal                      = platform_system_part2_dynalib_table_flash_start + 0);
PROVIDE (link_dynalib_location_rt                       = platform_system_part2_dynalib_table_flash_start + 4);
PROVIDE (link_dynalib_location_system                   = platform_system_part2_dynalib_table_flash_start + 8);
PROVIDE (link_dynalib_location_hal_peripherals          = platform_system_part2_dynalib_table_flash_start + 12);
PROVIDE (link_dynalib_location_hal_i2c                  = platform_system_part2_dynalib_table_flash_start + 16);
PROVIDE (link_dynalib_location_hal_gpio                 = platform_system_part2_dynalib_table_flash_start + 20);
PROVIDE (link_dynalib_location_hal_spi                  = platform_system_part2_dynalib_table_flash_start + 24);
PROVIDE (link_dynalib_location_hal_core                 = platform_system_part2_dynalib_table_flash_start + 28);
PROVIDE (link_dynalib_location_hal_socket               = platform_system_part2_dynalib_table_flash_start + 32);
PROVIDE (link_dynalib_location_hal_wlan                 = platform_system_part2_dynalib_table_flash_start + 36);
PROVIDE (link_dynalib_location_hal_usart                = platform_system_part2_dynalib_table_flash_start + 40);
PROVIDE (link_dynalib_location_system_net               = platform_system_part2_dynalib_table_flash_start + 44);
PROVIDE (link_dynalib_location_system_cloud             = platform_system_part2_dynalib_table_flash_start + 48);
PROVIDE (link_dynalib_location_hal_concurrent           = platform_system_part2_dynalib_table_flash_start + 52);
PROVIDE (link_dynalib_location_hal_can                  = platform_system_part2_dynalib_table_flash_start + 56);
PROVIDE (link_dynalib_location_hal_usb                  = platform_system_part2_dynalib_table_flash_start + 60);
PROVIDE (link_dynalib_location_hal_rgbled               = platform_system_part2_dynalib_table_flash_start + 64);
PROVIDE (link_dynalib_location_hal_bootloader           = platform_system_part2_dynalib_table_flash_start + 68);
PROVIDE (link_dynalib_location_hal_dct                  = platform_system_part2_dynalib_table_flash_start + 72);
PROVIDE (link_dynalib_location_system_module_part2      = platform_system_part2_dynalib_table_flash_start + 76);
