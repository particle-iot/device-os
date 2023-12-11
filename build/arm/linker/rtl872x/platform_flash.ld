/*
Platform FLASH memory map for Tron.

Modular firmware:                          Monolithic firmware:
+--------------------------------+ 0x08800000 +----------------------------------+
|                                                                                |
| Reserved (filesystem)                                                          |
|                                                                                |
+--------------------------------+ 0x08600000 +----------------------------------+
| User part module info suffix   |            |                                  |
+~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+            |                                  |
| User part RAM data             |            |                                  |
+~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+            |                                  |
| User part secure RAM code      |            |                                  |
+~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+            |                                  |
| User part PSRAM code           |            | Monolithic firmware RAM data     |
+~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+            |                                  |
| User part dynalib table        |            |                                  |
+~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+            |                                  |
| User part XIP code             |            |                                  |
+~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+            +~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+
| User part module info          |            |                                  |
+~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+            |                                  |
|                                |            |                                  |
|                                |            |                                  |
+~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+            |                                  |
| System part 1 RAM data         |            | Monolithic firmware PSRAM code   |
+~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+            |                                  |
| System part 1 secure RAM code  |            |                                  |
+~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+            |                                  |
| System part 1 PSRAM code       |            |                                  |
+~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+            +~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+
| System part 1 dynalib table    |            |                                  |
+~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+            | Monolithic firmware XIP code     |
| System part 1 XIP code         |            |                                  |
+--------------------------------+ 0x08060026 +----------------------------------+
| System part 1 entry            |            | Monolithic firmware entry        |
+--------------------------------+ 0x08060018 +----------------------------------+
| System part 1 module info      |            | Monolithic firmware module info  |
+--------------------------------+ 0x08060000 +----------------------------------+
| Boot info                                                                      |
+--------------------------------+ 0x0805F000 +----------------------------------+
| Backup RAM persistent backup                                                   |
+--------------------------------+ 0x0805E000 +----------------------------------+
|                                                                                |
|                                                                                |
| KM0 part1 RAM data                                                             |
+~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+
| KM0 part1 RAM code                                                             |
+~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+
| KM0 part1 XIP code                                                             |
+--------------------------------+ 0x08014018 +----------------------------------+
| KM0 part1 module info                                                          |
+--------------------------------+ 0x08014000 +----------------------------------+
|                                                                                |
|                                                                                |
| Bootloader secure RAM code                                                     |
+~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+
| Bootloader start table                                                         |
+~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+
| Bootloader RAM header                                                          |
+~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+
| Bootloader RAM data                                                            |
+~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+
| Bootloader RAM code                                                            |
+~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+
| Bootloader XIP code                                                            |
+--------------------------------+ 0x08004038 +----------------------------------+
| Bootloader module info                                                         |
+--------------------------------+ 0x08004020 +----------------------------------+
| Bootloader XIP header                                                          |
+--------------------------------+ 0x08004000 +----------------------------------+
|                                                                                |
|                                                                                |
| KM0 MBR RAM data                                                               |
+~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+
| KM0 MBR RAM code                                                               |
+~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+
| KM0 MBR start table                                                            |
+~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+
| KM0 MBR RAM header                                                             |
+~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+
| KM0 MBR XIP code                                                               |
+--------------------------------+ 0x08000040 +----------------------------------+
| KM0 MBR module info                                                            |
+--------------------------------+ 0x08000020 +----------------------------------+
| KM0 MBR XIP header                                                             |
+--------------------------------+ 0x08000000 +----------------------------------+

Note: Keep the diagrams up to date with the actual memory maps defined in this file.
*/

/* Platform interrupt vector table size. Please reference to the interrupt vector table in platform startup file. */
platform_vector_table_flash_size = 268;

/* Platform entry */
platform_entry_size = 8;

/* Module info size */
platform_module_info_flash_size = 24;

/* Realtek specific image header */
platform_image_header_flash_size = 32;


/* Total amount of flash available on the MCU */
platform_flash_size  = 8M;
platform_flash_start = 0x08000000;
platform_flash_end   = platform_flash_start + platform_flash_size;


/* KM0 bootloader */
platform_km0_mbr_flash_size  = 16K;
platform_km0_mbr_flash_start = platform_flash_start;
platform_km0_mbr_flash_end   = platform_km0_mbr_flash_start + platform_km0_mbr_flash_size;

/* KM0 bootloader module info */
platform_km0_mbr_module_info_flash_size  = platform_module_info_flash_size;
platform_km0_mbr_module_info_flash_start = platform_km0_mbr_flash_start + platform_image_header_flash_size;
platform_km0_mbr_module_info_flash_end   = platform_km0_mbr_module_info_flash_start + platform_km0_mbr_module_info_flash_size;


/* Bootloader */
platform_bootloader_flash_size  = 64K;
platform_bootloader_flash_start = platform_km0_mbr_flash_end;
platform_bootloader_flash_end   = platform_bootloader_flash_start + platform_bootloader_flash_size;

/* Bootloader module info */
platform_bootloader_module_info_flash_size  = platform_module_info_flash_size;
platform_bootloader_module_info_flash_start = platform_bootloader_flash_start + platform_image_header_flash_size;
platform_bootloader_module_info_flash_end   = platform_bootloader_module_info_flash_start + platform_bootloader_module_info_flash_size;


/* KM0 part1 */
platform_km0_part1_flash_size  = 296K;
platform_km0_part1_flash_start = platform_bootloader_flash_end;
platform_km0_part1_flash_end   = platform_km0_part1_flash_start + platform_km0_part1_flash_size;

/* KM0 part1 module info */
platform_km0_part1_module_info_flash_size  = platform_module_info_flash_size;
platform_km0_part1_module_info_flash_start = platform_km0_part1_flash_start;
platform_km0_part1_module_info_flash_end   = platform_km0_part1_module_info_flash_start + platform_km0_part1_module_info_flash_size;

/* KM0 part1 dynalib location */
platform_km0_part1_dynalib_table_flash_start = platform_km0_part1_module_info_flash_end;

platform_backup_ram_persisted_flash_size = 4K;
platform_backup_ram_persisted_flash_start = platform_km0_part1_flash_end;
platform_backup_ram_persisted_flash_end = platform_backup_ram_persisted_flash_start + platform_backup_ram_persisted_flash_size;

/* KM0 image update info sector */
platform_km0_image_update_size  = 4K;
platform_km0_image_update_start = platform_backup_ram_persisted_flash_end;
platform_km0_image_update_end = platform_km0_image_update_start + platform_km0_image_update_size;

/* Reserved for filesystem */
platform_reserved_flash_size  = 2M;
platform_reserved_flash_end   = platform_flash_end;
platform_reserved_flash_start = platform_reserved_flash_end - platform_reserved_flash_size;


/* User part */
platform_user_part_flash_size  = DEFINED(platform_user_part_flash_size) ? platform_user_part_flash_size : 1536K;
platform_user_part_flash_end   = platform_reserved_flash_start;
platform_user_part_flash_start = platform_user_part_flash_end - platform_user_part_flash_size;

/* User part module info */
platform_user_part_module_info_flash_size  = platform_module_info_flash_size;
platform_user_part_module_info_flash_start = platform_user_part_flash_start;
platform_user_part_module_info_flash_end   = platform_user_part_module_info_flash_start + platform_user_part_module_info_flash_size;


/* System part 1 */
platform_system_part1_flash_size  = 1536K;
platform_system_part1_flash_start = platform_km0_image_update_end;
platform_system_part1_flash_end   = platform_system_part1_flash_start + platform_system_part1_flash_size;

/* System part 1 module info */
platform_system_part1_module_info_flash_size  = platform_module_info_flash_size;
platform_system_part1_module_info_flash_start = platform_system_part1_flash_start;
platform_system_part1_module_info_flash_end   = platform_system_part1_module_info_flash_start + platform_system_part1_module_info_flash_size;

/* System part 1 entry */
platform_system_part1_entry_flash_size  = platform_entry_size;
platform_system_part1_entry_flash_start = platform_system_part1_module_info_flash_end;
platform_system_part1_entry_flash_end   = platform_system_part1_entry_flash_start + platform_system_part1_entry_flash_size;


/* Monolithic firmware*/
platform_monolithic_firmware_flash_start = platform_km0_image_update_end;
platform_monolithic_firmware_flash_end   = platform_reserved_flash_start;
platform_monolithic_firmware_flash_size  = platform_monolithic_firmware_flash_end - platform_monolithic_firmware_flash_start;

/* Monolithic firmware module info */
platform_monolithic_firmware_module_info_flash_size  = platform_module_info_flash_size;
platform_monolithic_firmware_module_info_flash_start = platform_monolithic_firmware_flash_start;
platform_monolithic_firmware_module_info_flash_end   = platform_monolithic_firmware_module_info_flash_start + platform_monolithic_firmware_module_info_flash_size;

/* Monolithic firmware entry */
platform_monolithic_firmware_entry_flash_size  = platform_entry_size;
platform_monolithic_firmware_entry_flash_start = platform_monolithic_firmware_module_info_flash_end;
platform_monolithic_firmware_entry_flash_end   = platform_monolithic_firmware_entry_flash_start + platform_monolithic_firmware_entry_flash_size;
