/*
Platform FLASH memory map for Electron and E-Series.

Modular firmware:                          Monolithic firmware:

+-----------------------------+ 0x08100000 +----------------------------------+
| Decompress region           |            |                                  |
+-----------------------------+ 0x080E0000 |                                  |
| OTA backup                  |            |                                  |
+-----------------------------+ 0x080C0000 |                                  |
| Factory reset               |            |                                  |
+-----------------------------+ 0x080A0000 |                                  |
| User part                   |            |                                  |
+~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+            |                                  |
| User part dynalib table     |            |                                  |
+-----------------------------+ 0x08080018 |                                  |
| User part module info       |            |                                  |
+-----------------------------+ 0x08080000 |                                  |
| System part 3               |            |                                  |
+~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+            |                                  |
| System part 3 dynalib table |            | Monolithic firmware              |
+-----------------------------+ 0x08060018 |                                  |
| System part 3 module info   |            |                                  |
+-----------------------------+ 0x08060000 |                                  |
| System part 2               |            |                                  |
+~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+            |                                  |
| System part 2 dynalib table |            |                                  |
+-----------------------------+ 0x0804019C |                                  |
| System part 2 module info   |            |                                  |
+-----------------------------+ 0x08040184 |                                  |
| System part 2 vector table  |            |                                  |
+-----------------------------+ 0x08040000 |                                  |
| System part 1               |            |                                  |
+~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+            |                                  |
| System part 1 dynalib table |            |                                  |
+-----------------------------+ 0x0802019C +----------------------------------+
| System part 1 module info   |            | Monolithic firmware module info  |
+-----------------------------+ 0x08020184 +----------------------------------+
| System part 1 vector table  |            | Monolithic firmware vector table |
+-----------------------------+ 0x08020000 +----------------------------------+
| EEPROM 2                    |            | EEPROM 2                         |
+-----------------------------+ 0x08010000 +----------------------------------+
| EEPROM 1                    |            | EEPROM 1                         |
+-----------------------------+ 0x0800C000 +----------------------------------+
| DCT 2                       |            | DCT 2                            |
+-----------------------------+ 0x08008000 +----------------------------------+
| DCT 1                       |            | DCT 1                            |
+-----------------------------+ 0x08004000 +----------------------------------+
| Bootloader                  |            | Bootloader                       |
+-----------------------------+ 0x0800019C +----------------------------------+
| Bootloader module info      |            | Bootloader module info           |
+-----------------------------+ 0x08000184 +----------------------------------+
| Bootloader vector table     |            | Bootloader vector table          |
+-----------------------------+ 0x08000000 +----------------------------------+

Note: Keep the diagrams up to date with the actual memory maps defined in this file.
*/

/* Platform interrupt vector table size. Please reference to the interrupt vector table in platform startup file. */
platform_vector_table_flash_size = 388;

/* This is only referenced in this file */
platform_module_info_flash_size = 24;

/* Total amount of flash available on the MCU */
platform_flash_size  = 1M;
platform_flash_start = 0x08000000;
platform_flash_end   = platform_flash_start + platform_flash_size;


/* Decompress region */
platform_decompress_region_flash_size  = 128K;
platform_decompress_region_flash_end   = platform_flash_end;
platform_decompress_region_flash_start = platform_decompress_region_flash_end - platform_decompress_region_flash_size;


/* OTA backup */
platform_ota_backup_flash_size  = 128K;
platform_ota_backup_flash_end   = platform_decompress_region_flash_start;
platform_ota_backup_flash_start = platform_ota_backup_flash_end - platform_ota_backup_flash_size;


/* Factory backup */
platform_factory_reset_flash_size  = 128K;
platform_factory_reset_flash_end   = platform_ota_backup_flash_start;
platform_factory_reset_flash_start = platform_factory_reset_flash_end - platform_factory_reset_flash_size;


/* User part */
platform_user_part_flash_size  = 128K;
platform_user_part_flash_end   = platform_factory_reset_flash_start;
platform_user_part_flash_start = platform_user_part_flash_end - platform_user_part_flash_size;

/* User part module info */
platform_user_part_module_info_flash_size  = platform_module_info_flash_size;
platform_user_part_module_info_flash_start = platform_user_part_flash_start;
platform_user_part_module_info_flash_end   = platform_user_part_module_info_flash_start + platform_user_part_module_info_flash_size;

/* User part dynalib table */
platform_user_part_dynalib_table_flash_start = platform_user_part_module_info_flash_end;


/* System part 3, the linked image name is system-part1.bin */
platform_system_part3_flash_size  = 128K;
platform_system_part3_flash_end   = platform_user_part_flash_start;
platform_system_part3_flash_start = platform_system_part3_flash_end - platform_system_part3_flash_size;

/* System part 3 module info */
platform_system_part3_module_info_flash_size  = platform_module_info_flash_size;
platform_system_part3_module_info_flash_start = platform_system_part3_flash_start;
platform_system_part3_module_info_flash_end   = platform_system_part3_module_info_flash_start + platform_system_part3_module_info_flash_size;

/* System part 3 dynalib table */
platform_system_part3_dynalib_table_flash_start = platform_system_part3_module_info_flash_end;


/* System part 2, the linked image is system-part3.bin */
platform_system_part2_flash_size  = 128K;
platform_system_part2_flash_end   = platform_system_part3_flash_start;
platform_system_part2_flash_start = platform_system_part2_flash_end - platform_system_part2_flash_size;

/* System part 2 vector table */
platform_system_part2_vector_table_flash_size  = platform_vector_table_flash_size;
platform_system_part2_vector_table_flash_start = platform_system_part2_flash_start;
platform_system_part2_vector_table_flash_end   = platform_system_part2_vector_table_flash_start + platform_system_part2_vector_table_flash_size;

/* System part 2 module info */
platform_system_part2_module_info_flash_size  = platform_module_info_flash_size;
platform_system_part2_module_info_flash_start = platform_system_part2_vector_table_flash_end;
platform_system_part2_module_info_flash_end   = platform_system_part2_module_info_flash_start + platform_system_part2_module_info_flash_size;

/* System part 2 dynalib table */
platform_system_part2_dynalib_table_flash_start = platform_system_part2_module_info_flash_end;


/* System part 1, the linked image is system-part2.bin */
platform_system_part1_flash_size  = 128K;
platform_system_part1_flash_end   = platform_system_part2_flash_start;
platform_system_part1_flash_start = platform_system_part1_flash_end - platform_system_part1_flash_size;

/* System part 1 vector table */
platform_system_part1_vector_table_flash_size  = platform_vector_table_flash_size;
platform_system_part1_vector_table_flash_start = platform_system_part1_flash_start;
platform_system_part1_vector_table_flash_end   = platform_system_part1_vector_table_flash_start + platform_system_part1_vector_table_flash_size;

/* System part 1 module info */
platform_system_part1_module_info_flash_size  = platform_module_info_flash_size;
platform_system_part1_module_info_flash_start = platform_system_part1_vector_table_flash_end;
platform_system_part1_module_info_flash_end   = platform_system_part1_module_info_flash_start + platform_system_part1_module_info_flash_size;

/* System part 1 dynalib table */
platform_system_part1_dynalib_table_flash_start = platform_system_part1_module_info_flash_end;


/* Monolithic firmware*/
platform_monolithic_firmware_flash_size  = 896K;
platform_monolithic_firmware_flash_end   = platform_flash_end;
platform_monolithic_firmware_flash_start = platform_monolithic_firmware_flash_end - platform_monolithic_firmware_flash_size;

/* Monolithic firmware vector table */
platform_monolithic_firmware_vector_table_flash_size  = platform_vector_table_flash_size;
platform_monolithic_firmware_vector_table_flash_start = platform_monolithic_firmware_flash_start;
platform_monolithic_firmware_vector_table_flash_end   = platform_monolithic_firmware_vector_table_flash_start + platform_monolithic_firmware_vector_table_flash_size;

/* Monolithic firmware module info */
platform_monolithic_firmware_module_info_flash_size  = platform_module_info_flash_size;
platform_monolithic_firmware_module_info_flash_start = platform_monolithic_firmware_vector_table_flash_end;
platform_monolithic_firmware_module_info_flash_end   = platform_monolithic_firmware_module_info_flash_start + platform_monolithic_firmware_module_info_flash_size;


/* EEPROM 2 */
platform_eeprom2_flash_size  = 64K;
platform_eeprom2_flash_end   = platform_system_part1_flash_start;
platform_eeprom2_flash_start = platform_eeprom2_flash_end - platform_eeprom2_flash_size;

/* EEPROM 1 */
platform_eeprom1_flash_size  = 16K;
platform_eeprom1_flash_end   = platform_eeprom2_flash_start;
platform_eeprom1_flash_start = platform_eeprom1_flash_end - platform_eeprom1_flash_size;


/* DCT 2 */
platform_dct2_flash_size  = 16K;
platform_dct2_flash_end   = platform_eeprom1_flash_start;
platform_dct2_flash_start = platform_dct2_flash_end - platform_dct2_flash_size;

/* DCT 1 */
platform_dct1_flash_size  = 16K;
platform_dct1_flash_end   = platform_dct2_flash_start;
platform_dct1_flash_start = platform_dct1_flash_end - platform_dct1_flash_size;


/* Bootloader */
platform_bootloader_flash_size  = 16K;
platform_bootloader_flash_start = platform_flash_start;
platform_bootloader_flash_end   = platform_bootloader_flash_start + platform_bootloader_flash_size;

/* Bootloader vector table */
platform_bootloader_vector_table_flash_size  = platform_vector_table_flash_size;
platform_bootloader_vector_table_flash_start = platform_bootloader_flash_start;
platform_bootloader_vector_table_flash_end   = platform_bootloader_vector_table_flash_start + platform_bootloader_vector_table_flash_size;

/* Bootloader module info */
platform_bootloader_module_info_flash_size  = platform_module_info_flash_size;
platform_bootloader_module_info_flash_start = platform_bootloader_vector_table_flash_end;
platform_bootloader_module_info_flash_end   = platform_bootloader_module_info_flash_start + platform_bootloader_module_info_flash_size;


/* Basic sanity checks */
ASSERT (
    (platform_flash_end - platform_flash_start == platform_flash_size && platform_flash_size > 0) &&
    (platform_bootloader_flash_size + platform_dct1_flash_size + platform_dct2_flash_size + platform_eeprom1_flash_size + platform_eeprom2_flash_size + platform_system_part1_flash_size + platform_system_part2_flash_size + platform_system_part3_flash_size + platform_user_part_flash_size + platform_ota_backup_flash_size + platform_factory_reset_flash_size + platform_decompress_region_flash_size == platform_flash_size) &&
    (platform_bootloader_flash_size + platform_dct1_flash_size + platform_dct2_flash_size + platform_eeprom1_flash_size + platform_eeprom2_flash_size + platform_monolithic_firmware_flash_size == platform_flash_size),
    "Invalid overall flash memory map!"
);
ASSERT (
    (platform_decompress_region_flash_end - platform_decompress_region_flash_start == platform_decompress_region_flash_size && platform_decompress_region_flash_size > 0),
    "Invalid decompress region flash memory map!"
);
ASSERT (
    (platform_ota_backup_flash_end - platform_ota_backup_flash_start == platform_ota_backup_flash_size && platform_ota_backup_flash_size > 0),
    "Invalid OTA backup flash memory map!"
);
ASSERT (
    (platform_factory_reset_flash_end - platform_factory_reset_flash_start == platform_factory_reset_flash_size && platform_factory_reset_flash_size > 0),
    "Invalid factory backup flash memory map!"
);
ASSERT (
    (platform_user_part_flash_start == 0x8080000) &&
    (platform_user_part_flash_end - platform_user_part_flash_start == platform_user_part_flash_size && platform_user_part_flash_size > 0) &&
    (platform_user_part_module_info_flash_end - platform_user_part_module_info_flash_start == platform_user_part_module_info_flash_size && platform_user_part_module_info_flash_size > 0),
    "Invalid user part flash memory map!"
);
ASSERT (
    (platform_system_part3_flash_start == 0x8060000) &&
    (platform_system_part3_flash_end - platform_system_part3_flash_start == platform_system_part3_flash_size && platform_system_part3_flash_size > 0) &&
    (platform_system_part3_module_info_flash_end - platform_system_part3_module_info_flash_start == platform_system_part3_module_info_flash_size && platform_system_part3_module_info_flash_size > 0),
    "Invalid system part 3 flash memory map!"
);
ASSERT (
    (platform_system_part2_flash_start == 0x8040000) &&
    (platform_system_part2_flash_end - platform_system_part2_flash_start == platform_system_part2_flash_size && platform_system_part2_flash_size > 0) &&
    (platform_system_part2_vector_table_flash_end - platform_system_part2_vector_table_flash_start == platform_system_part2_vector_table_flash_size && platform_system_part2_vector_table_flash_size > 0) &&
    (platform_system_part2_module_info_flash_end - platform_system_part2_module_info_flash_start == platform_system_part2_module_info_flash_size && platform_system_part2_module_info_flash_size > 0),
    "Invalid system part 2 flash memory map!"
);
ASSERT (
    (platform_system_part1_flash_start == 0x8020000) &&
    (platform_system_part1_flash_end - platform_system_part1_flash_start == platform_system_part1_flash_size && platform_system_part1_flash_size > 0) &&
    (platform_system_part1_vector_table_flash_end - platform_system_part1_vector_table_flash_start == platform_system_part1_vector_table_flash_size && platform_system_part1_vector_table_flash_size > 0) &&
    (platform_system_part1_module_info_flash_end - platform_system_part1_module_info_flash_start == platform_system_part1_module_info_flash_size && platform_system_part1_module_info_flash_size > 0),
    "Invalid system part 1 flash memory map!"
);
ASSERT (
    (platform_monolithic_firmware_flash_start == 0x8020000) &&
    (platform_monolithic_firmware_flash_end - platform_monolithic_firmware_flash_start == platform_monolithic_firmware_flash_size && platform_monolithic_firmware_flash_size > 0) &&
    (platform_monolithic_firmware_vector_table_flash_end - platform_monolithic_firmware_vector_table_flash_start == platform_monolithic_firmware_vector_table_flash_size && platform_monolithic_firmware_vector_table_flash_size > 0) &&
    (platform_monolithic_firmware_module_info_flash_end - platform_monolithic_firmware_module_info_flash_start == platform_monolithic_firmware_module_info_flash_size && platform_monolithic_firmware_module_info_flash_size > 0),
    "Invalid monolithic firmware flash memory map!"
);
ASSERT (
    (platform_eeprom2_flash_end - platform_eeprom2_flash_start == platform_eeprom2_flash_size && platform_eeprom2_flash_size > 0) &&
    (platform_eeprom1_flash_end - platform_eeprom1_flash_start == platform_eeprom1_flash_size && platform_eeprom1_flash_size > 0),
    "Invalid EEPROM flash memory map!"
);
ASSERT (
    (platform_dct2_flash_end - platform_dct2_flash_start == platform_dct2_flash_size && platform_dct2_flash_size > 0) &&
    (platform_dct1_flash_end - platform_dct1_flash_start == platform_dct1_flash_size && platform_dct1_flash_size > 0),
    "Invalid DCT flash memory map!"
);
ASSERT (
    (platform_bootloader_flash_start == 0x8000000) &&
    (platform_bootloader_flash_end - platform_bootloader_flash_start == platform_bootloader_flash_size && platform_bootloader_flash_size > 0) &&
    (platform_bootloader_vector_table_flash_end - platform_bootloader_vector_table_flash_start == platform_bootloader_vector_table_flash_size && platform_bootloader_vector_table_flash_size > 0) &&
    (platform_bootloader_module_info_flash_end - platform_bootloader_module_info_flash_start == platform_bootloader_module_info_flash_size && platform_bootloader_module_info_flash_size > 0),
    "Invalid bootloader flash memory map"
);
