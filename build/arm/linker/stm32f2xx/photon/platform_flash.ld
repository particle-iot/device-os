/*
Platform FLASH memory map for Photon and P1.

Modular firmware:                          Monolithic firmware:

+-----------------------------+ 0x08100000 +----------------------------------+
| Factory backup              |            |                                  |
+-----------------------------+ 0x080E0000 |                                  |
| OTA backup                  |            |                                  |
+-----------------------------+ 0x080C0000 |                                  |
| User part                   |            |                                  |
+~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+            |                                  |
| User part dynalib table     |            |                                  |
+-----------------------------+ 0x080A0018 |                                  |
| User part module info       |            |                                  |
+-----------------------------+ 0x080A0000 |                                  |
| System part 2               |            | Monolithic firmware              |
+~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+            |                                  |
| System part 2 dynalib table |            |                                  |
+-----------------------------+ 0x0806019C |                                  |
| System part 2 module info   |            |                                  |
+-----------------------------+ 0x08060184 |                                  |
| System part 2 vector table  |            |                                  |
+-----------------------------+ 0x08060000 |                                  |
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

/* This is only referenced in this file */
_platform_vector_table_flash_size = 388;

/* This is only referenced in this file */
_module_info_flash_size = 24;

/* Total amount of flash available on the MCU */
_flash_size  = 1M;
_flash_start = 0x08000000;
_flash_end   = _flash_start + _flash_size;

/* Factory backup */
_factory_backup_flash_size  = 128K;
_factory_backup_flash_end   = _flash_end;
_factory_backup_flash_start = _factory_backup_flash_end - _factory_backup_flash_size;

/* OTA backup */
_ota_backup_flash_size  = 128K;
_ota_backup_flash_end   = _factory_backup_flash_start;
_ota_backup_flash_start = _ota_backup_flash_end - _ota_backup_flash_size;

/* User part */
_user_part_flash_size  = 128K;
_user_part_flash_end   = _ota_backup_flash_start;
_user_part_flash_start = _user_part_flash_end - _user_part_flash_size;

/* User part module info */
_user_part_module_info_flash_size  = _module_info_flash_size;
_user_part_module_info_flash_start = _user_part_flash_start;
_user_part_module_info_flash_end   = _user_part_module_info_flash_start + _user_part_module_info_flash_size;

/* User part dynalib table */
_user_part_dynalib_table_flash_start = _user_part_module_info_flash_end;

/* System part 2 */
_system_part2_flash_size  = 256K;
_system_part2_flash_end   = _user_part_flash_start;
_system_part2_flash_start = _system_part2_flash_end - _system_part2_flash_size;

/* System part 2 vector table */
_system_part2_vector_table_flash_size  = _platform_vector_table_flash_size;
_system_part2_vector_table_flash_start = _system_part2_flash_start;
_system_part2_vector_table_flash_end   = _system_part2_vector_table_flash_start + _system_part2_vector_table_flash_size;

/* System part 2 module info */
_system_part2_module_info_flash_size  = _module_info_flash_size;
_system_part2_module_info_flash_start = _system_part2_vector_table_flash_end;
_system_part2_module_info_flash_end   = _system_part2_module_info_flash_start + _system_part2_module_info_flash_size;

/* System part 2 dynalib table */
_system_part2_dynalib_table_flash_start = _system_part2_module_info_flash_end;

/* System part 1 */
_system_part1_flash_size  = 256K;
_system_part1_flash_end   = _system_part2_flash_start;
_system_part1_flash_start = _system_part1_flash_end - _system_part1_flash_size;

/* System part 1 vector table */
_system_part1_vector_table_flash_size  = _platform_vector_table_flash_size;
_system_part1_vector_table_flash_start = _system_part1_flash_start;
_system_part1_vector_table_flash_end   = _system_part1_vector_table_flash_start + _system_part1_vector_table_flash_size;

/* System part 1 module info */
_system_part1_module_info_flash_size  = _module_info_flash_size;
_system_part1_module_info_flash_start = _system_part1_vector_table_flash_end;
_system_part1_module_info_flash_end   = _system_part1_module_info_flash_start + _system_part1_module_info_flash_size;

/* System part 1 dynalib table */
_system_part1_dynalib_table_flash_start = _system_part1_module_info_flash_end;

/* Monolithic firmware*/
_monolithic_firmware_flash_size  = 896K;
_monolithic_firmware_flash_end   = _flash_end;
_monolithic_firmware_flash_start = _monolithic_firmware_flash_end - _monolithic_firmware_flash_size;

/* Monolithic firmware vector table */
_monolithic_firmware_vector_table_flash_size  = _platform_vector_table_flash_size;
_monolithic_firmware_vector_table_flash_start = _monolithic_firmware_flash_start;
_monolithic_firmware_vector_table_flash_end   = _monolithic_firmware_vector_table_flash_start + _monolithic_firmware_vector_table_flash_size;

/* Monolithic firmware module info */
_monolithic_firmware_module_info_flash_size  = _module_info_flash_size;
_monolithic_firmware_module_info_flash_start = _monolithic_firmware_vector_table_flash_end;
_monolithic_firmware_module_info_flash_end   = _monolithic_firmware_module_info_flash_start + _monolithic_firmware_module_info_flash_size;

/* EEPROM 2 */
_eeprom2_flash_size  = 64K;
_eeprom2_flash_end   = _system_part1_flash_start;
_eeprom2_flash_start = _eeprom2_flash_end - _eeprom2_flash_size;

/* EEPROM 1 */
_eeprom1_flash_size  = 16K;
_eeprom1_flash_end   = _eeprom2_flash_start;
_eeprom1_flash_start = _eeprom1_flash_end - _eeprom1_flash_size;

/* DCT 2 */
_dct2_flash_size  = 16K;
_dct2_flash_end   = _eeprom1_flash_start;
_dct2_flash_start = _dct2_flash_end - _dct2_flash_size;

/* DCT 1 */
_dct1_flash_size  = 16K;
_dct1_flash_end   = _dct2_flash_start;
_dct1_flash_start = _dct1_flash_end - _dct1_flash_size;

/* Bootloader */
_bootloader_flash_size  = 16K;
_bootloader_flash_start = _flash_start;
_bootloader_flash_end   = _bootloader_flash_start + _bootloader_flash_size;

/* Bootloader vector table */
_bootloader_vector_table_flash_size  = _platform_vector_table_flash_size;
_bootloader_vector_table_flash_start = _bootloader_flash_start;
_bootloader_vector_table_flash_end   = _bootloader_vector_table_flash_start + _bootloader_vector_table_flash_size;

/* Bootloader module info */
_bootloader_module_info_flash_size  = _module_info_flash_size;
_bootloader_module_info_flash_start = _bootloader_vector_table_flash_end;
_bootloader_module_info_flash_end   = _bootloader_module_info_flash_start + _bootloader_module_info_flash_size;

/* Basic sanity checks */
ASSERT(
    (_flash_end - _flash_start == _flash_size && _flash_size > 0) &&
    (_bootloader_flash_size + _dct1_flash_size + _dct2_flash_size + _eeprom1_flash_size + _eeprom2_flash_size + _system_part1_flash_size + _system_part2_flash_size + _user_part_flash_size + _ota_backup_flash_size + _factory_backup_flash_size == _flash_size) &&
    (_bootloader_flash_size + _dct1_flash_size + _dct2_flash_size + _eeprom1_flash_size + _eeprom2_flash_size + _monolithic_firmware_flash_size == _flash_size),
    "Invalid overall flash memory map!"
);
ASSERT(
    (_factory_backup_flash_end - _factory_backup_flash_start == _factory_backup_flash_size && _factory_backup_flash_size > 0),
    "Invalid factory backup flash memory map!"
);
ASSERT(
    (_ota_backup_flash_end - _ota_backup_flash_start == _ota_backup_flash_size && _ota_backup_flash_size > 0),
    "Invalid OTA backup flash memory map!"
);
ASSERT(
    (_user_part_flash_start == 0x80A0000) &&
    (_user_part_flash_end - _user_part_flash_start == _user_part_flash_size && _user_part_flash_size > 0) &&
    (_user_part_module_info_flash_end - _user_part_module_info_flash_start == _user_part_module_info_flash_size && _user_part_module_info_flash_size > 0),
    "Invalid user part flash memory map!"
);
ASSERT(
    (_system_part2_flash_start == 0x8060000) &&
    (_system_part2_flash_end - _system_part2_flash_start == _system_part2_flash_size && _system_part2_flash_size > 0) &&
    (_system_part2_vector_table_flash_end - _system_part2_vector_table_flash_start == _system_part2_vector_table_flash_size && _system_part2_vector_table_flash_size > 0) &&
    (_system_part2_module_info_flash_end - _system_part2_module_info_flash_start == _system_part2_module_info_flash_size && _system_part2_module_info_flash_size > 0),
    "Invalid system part 2 flash memory map!"
);
ASSERT(
    (_system_part1_flash_start == 0x8020000) &&
    (_system_part1_flash_end - _system_part1_flash_start == _system_part1_flash_size && _system_part1_flash_size > 0) &&
    (_system_part1_vector_table_flash_end - _system_part1_vector_table_flash_start == _system_part1_vector_table_flash_size && _system_part1_vector_table_flash_size > 0) &&
    (_system_part1_module_info_flash_end - _system_part1_module_info_flash_start == _system_part1_module_info_flash_size && _system_part1_module_info_flash_size > 0),
    "Invalid system part 1 flash memory map!"
);
ASSERT(
    (_monolithic_firmware_flash_start == 0x8020000) &&
    (_monolithic_firmware_flash_end - _monolithic_firmware_flash_start == _monolithic_firmware_flash_size && _monolithic_firmware_flash_size > 0) &&
    (_monolithic_firmware_vector_table_flash_end - _monolithic_firmware_vector_table_flash_start == _monolithic_firmware_vector_table_flash_size && _monolithic_firmware_vector_table_flash_size > 0) &&
    (_monolithic_firmware_module_info_flash_end - _monolithic_firmware_module_info_flash_start == _monolithic_firmware_module_info_flash_size && _monolithic_firmware_module_info_flash_size > 0),
    "Invalid monolithic firmware flash memory map!"
);
ASSERT(
    (_eeprom2_flash_end - _eeprom2_flash_start == _eeprom2_flash_size && _eeprom2_flash_size > 0) &&
    (_eeprom1_flash_end - _eeprom1_flash_start == _eeprom1_flash_size && _eeprom1_flash_size > 0),
    "Invalid EEPROM flash memory map!"
);
ASSERT(
    (_dct2_flash_end - _dct2_flash_start == _dct2_flash_size && _dct2_flash_size > 0) &&
    (_dct1_flash_end - _dct1_flash_start == _dct1_flash_size && _dct1_flash_size > 0),
    "Invalid DCT flash memory map!"
);
ASSERT(
    (_bootloader_flash_start == 0x8000000) &&
    (_bootloader_flash_end - _bootloader_flash_start == _bootloader_flash_size && _bootloader_flash_size > 0) &&
    (_bootloader_vector_table_flash_end - _bootloader_vector_table_flash_start == _bootloader_vector_table_flash_size && _bootloader_vector_table_flash_size > 0) &&
    (_bootloader_module_info_flash_end - _bootloader_module_info_flash_start == _bootloader_module_info_flash_size && _bootloader_module_info_flash_size > 0),
    "Invalid bootloader flash memory map"
);
