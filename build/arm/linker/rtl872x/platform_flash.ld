/*
Platform FLASH memory map for Tron.

Modular firmware:                          Monolithic firmware:
+--------------------------------+ 0x08800000 +----------------------------------+
|                                                                                |
| Reserved 3                                                                     |
|                                                                                |
+--------------------------------+ 0x08600000 +----------------------------------+
| User part RAM data             |            | OTA                              |
+~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+            +~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+
| User part PSRAM code           |            |                                  |
+~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+            | Monolithic firmware RAM data     |
| User part XIP code             |            |                                  |
+~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+            |                                  |
| User part dynalib table        |            |                                  |
+~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+            +~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+
| User part module info          |            |                                  |
+~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+            |                                  |
| OTA                            |            |                                  |
+~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+            |                                  |
| System part 1 RAM data         |            | Monolithic firmware PSRAM code   |
+~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+            |                                  |
| System part 1 PSRAM code       |            |                                  |
+~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+            |                                  |
| System part 1 XIP code         |            |                                  |
+~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+            +~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+
| System part 1 dynalib table    |            | Monolithic firmware XIP code     |
+--------------------------------+ 0x0801C018 +----------------------------------+
| System part 1 module info      |            | Monolithic firmware module info  |
+--------------------------------+ 0x0801C000 +----------------------------------+
|                                                                                |
| Reserved 2                                                                     |
|                                                                                |
+--------------------------------+ 0x08014000 +----------------------------------+
| Bootloader RAM data                                                            |
+~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+
| Bootloader RAM code                                                            |
+~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+
| Bootloader RAM header                                                          |
+~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+
| Bootloader XIP code                                                            |
+--------------------------------+ 0x08004058 +----------------------------------+
| Bootloader module info                                                         |
+--------------------------------+ 0x08004040 +----------------------------------+
| Bootloader XIP header                                                          |
+--------------------------------+ 0x08004000 +----------------------------------+
| System data                                                                    |
+--------------------------------+ 0x08003000 +----------------------------------+
| Backup                                                                         |
+--------------------------------+ 0x08002000 +----------------------------------+
|                                                                                |
| Reserved 1                                                                     |
|                                                                                |
+--------------------------------+ 0x08000000 +----------------------------------+

Note: Keep the diagrams up to date with the actual memory maps defined in this file.
*/

/* Platform interrupt vector table size. Please reference to the interrupt vector table in platform startup file. */
platform_vector_table_flash_size = 268;

/* Module info size */
platform_module_info_flash_size = 24;

/* Realtek specific image header */
platform_image_header_flash_size = 64;


/* Total amount of flash available on the MCU */
platform_flash_size  = 8M;
platform_flash_start = 0x08000000;
platform_flash_end   = platform_flash_start + platform_flash_size;


/* Reserved for KM0 bootloader, see platform_flash_m23.ld */
platform_reserved1_flash_size  = 8K;
platform_reserved1_flash_start = platform_flash_start;
platform_reserved1_flash_end   = platform_reserved1_flash_start + platform_reserved1_flash_size;


/* Realtek specific backup region */
platform_backup_flash_size  = 4K;
platform_backup_flash_start = platform_reserved1_flash_end;
platform_backup_flash_end   = platform_backup_flash_start + platform_backup_flash_size;


/* Realtek specific system data */
platform_system_data_flash_size  = 4K;
platform_system_data_flash_start = platform_backup_flash_end;
platform_system_data_flash_end   = platform_system_data_flash_start + platform_system_data_flash_size;


/* Bootloader */
platform_bootloader_flash_size  = 64K;
platform_bootloader_flash_start = platform_system_data_flash_end;
platform_bootloader_flash_end   = platform_bootloader_flash_start + platform_bootloader_flash_size;

/* Bootloader module info */
platform_bootloader_module_info_flash_size  = platform_module_info_flash_size;
platform_bootloader_module_info_flash_start = platform_bootloader_flash_start + platform_image_header_flash_size;
platform_bootloader_module_info_flash_end   = platform_bootloader_module_info_flash_start + platform_bootloader_module_info_flash_size;


/* Reserved for KM0 application, see platform_flash_m23.ld */
platform_reserved2_flash_size  = 32K;
platform_reserved2_flash_start = platform_bootloader_flash_end;
platform_reserved2_flash_end   = platform_reserved2_flash_start + platform_reserved2_flash_size;


/* System part 1 */
platform_system_part1_flash_size  = 2M;
platform_system_part1_flash_start = platform_reserved2_flash_end;
platform_system_part1_flash_end   = platform_system_part1_flash_start + platform_system_part1_flash_size;

/* System part 1 module info */
platform_system_part1_module_info_flash_size  = platform_module_info_flash_size;
platform_system_part1_module_info_flash_start = platform_system_part1_flash_start;
platform_system_part1_module_info_flash_end   = platform_system_part1_module_info_flash_start + platform_system_part1_module_info_flash_size;

/* System part 1 dynalib table */
platform_system_part1_dynalib_table_flash_start = platform_system_part1_module_info_flash_end;


/* Reserved for filesystem */
platform_reserved3_flash_size  = 2M;
platform_reserved3_flash_end   = platform_flash_end;
platform_reserved3_flash_start = platform_reserved3_flash_end - platform_reserved3_flash_size;


/* User part */
platform_user_part_flash_size  = 1M;
platform_user_part_flash_end   = platform_reserved3_flash_start;
platform_user_part_flash_start = platform_user_part_flash_end - platform_user_part_flash_size;

/* User part module info */
platform_user_part_module_info_flash_size  = platform_module_info_flash_size;
platform_user_part_module_info_flash_start = platform_user_part_flash_start;
platform_user_part_module_info_flash_end   = platform_user_part_module_info_flash_start + platform_user_part_module_info_flash_size;

/* User part dynalib table */
platform_user_part_dynalib_table_flash_start = platform_user_part_module_info_flash_end;


/* Monolithic firmware*/
platform_monolithic_firmware_flash_size  = 3M;
platform_monolithic_firmware_flash_start = platform_reserved2_flash_end;
platform_monolithic_firmware_flash_end   = platform_monolithic_firmware_flash_start + platform_monolithic_firmware_flash_size;

/* Monolithic firmware module info */
platform_monolithic_firmware_module_info_flash_size  = platform_module_info_flash_size;
platform_monolithic_firmware_module_info_flash_start = platform_monolithic_firmware_flash_start;
platform_monolithic_firmware_module_info_flash_end   = platform_monolithic_firmware_module_info_flash_start + platform_monolithic_firmware_module_info_flash_size;

