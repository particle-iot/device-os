/*
Platform FLASH memory map for Argon, Boron, Xenon, ASoM, BSoM, XSoM.

Modular firmware:                          Monolithic firmware:

+-----------------------------+ 0x00100000 +----------------------------------+
|                             |            |                                  |
| Bootloader                  |            | Bootloader                       |
|                             |            |                                  |
+-----------------------------+ 0x000F4218 +----------------------------------+
| Bootloader module info      |            | Bootloader module info           |
+-----------------------------+ 0x000F4200 +----------------------------------+
| Bootloader vector table     |            | Bootloader vector table          |
+-----------------------------+ 0x000F4000 +----------------------------------+
|                             |            |                                  |
| User part                   |            |                                  |
|                             |            |                                  |
+~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+            |                                  |
| User part dynalib table     |            |                                  |
+-----------------------------+ 0x000D4018 |                                  |
| User part module info       |            | Monolithic firmware              |
+-----------------------------+ 0x000D4000 |                                  |
|                             |            |                                  |
| System part 1               |            |                                  |
|                             |            |                                  |
+~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+            |                                  |
| System part 1 dynalib table |            |                                  |
+-----------------------------+ 0x00030218 +----------------------------------+
| System part 1 module info   |            | Monolithic firmware module info  |
+-----------------------------+ 0x00030200 +----------------------------------+
| System part 1 vector table  |            | Monolithic firmware vector table |
+-----------------------------+ 0x00030000 +----------------------------------+
| Reserved for SoftDevice     |            | Reserved for SoftDevice          |
+-----------------------------+ 0x00026000 +----------------------------------+
| SoftDevice                  |            | SoftDevice                       |
+-----------------------------+ 0x00000000 +----------------------------------+

Note: Keep the diagrams up to date with the actual memory maps defined in this file.
*/

/* This is only referenced in this file */
_platform_vector_table_flash_size = 512;

/* This is only referenced in this file */
_module_info_flash_size = 24;

/* Total amount of flash available on the MCU */
_flash_size  = 1M;
_flash_start = 0x00000000;
_flash_end   = _flash_start + _flash_size;

/* Bootloader */
_bootloader_flash_size  = 48K;
_bootloader_flash_end   = _flash_end;
_bootloader_flash_start = _bootloader_flash_end - _bootloader_flash_size;

/* Bootloader vector table */
_bootloader_vector_table_flash_size  = _platform_vector_table_flash_size;
_bootloader_vector_table_flash_start = _bootloader_flash_start;
_bootloader_vector_table_flash_end   = _bootloader_vector_table_flash_start + _bootloader_vector_table_flash_size;

/* Bootloader module info */
_bootloader_module_info_flash_size  = _module_info_flash_size;
_bootloader_module_info_flash_start = _bootloader_vector_table_flash_end;
_bootloader_module_info_flash_end   = _bootloader_module_info_flash_start + _bootloader_module_info_flash_size;

/* User part */
_user_part_flash_size  = 128K;
_user_part_flash_end   = _bootloader_flash_start;
_user_part_flash_start = _user_part_flash_end - _user_part_flash_size;

/* User part module info */
_user_part_module_info_flash_size  = _module_info_flash_size;
_user_part_module_info_flash_start = _user_part_flash_start;
_user_part_module_info_flash_end   = _user_part_module_info_flash_start + _user_part_module_info_flash_size;

/* User part dynalib table */
_user_part_dynalib_table_flash_start = _user_part_module_info_flash_end;

/* System part 1 */
_system_part1_flash_size  = 656K;
_system_part1_flash_end   = _user_part_flash_start;
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
_monolithic_firmware_flash_size  = 784K;
_monolithic_firmware_flash_end   = _bootloader_flash_start;
_monolithic_firmware_flash_start = _monolithic_firmware_flash_end - _monolithic_firmware_flash_size;

/* Monolithic firmware vector table */
_monolithic_firmware_vector_table_flash_size  = _platform_vector_table_flash_size;
_monolithic_firmware_vector_table_flash_start = _monolithic_firmware_flash_start;
_monolithic_firmware_vector_table_flash_end   = _monolithic_firmware_vector_table_flash_start + _monolithic_firmware_vector_table_flash_size;

/* Monolithic firmware module info */
_monolithic_firmware_module_info_flash_size  = _module_info_flash_size;
_monolithic_firmware_module_info_flash_start = _monolithic_firmware_vector_table_flash_end;
_monolithic_firmware_module_info_flash_end   = _monolithic_firmware_module_info_flash_start + _monolithic_firmware_module_info_flash_size;

/* Reserved for Softdevice extention */
_softdevice_reserved_flash_size  = 40K;
_softdevice_reserved_flash_end   = _system_part1_flash_start;
_softdevice_reserved_flash_start = _softdevice_reserved_flash_end - _softdevice_reserved_flash_size;

/* Softdevice */
_softdevice_flash_size  = 152K;
_softdevice_flash_start = _flash_start;
_softdevice_flash_end   = _softdevice_flash_start + _softdevice_flash_size;

/* Basic sanity checks */
ASSERT(
    (_flash_end - _flash_start == _flash_size && _flash_size > 0) &&
    (_softdevice_flash_size + _softdevice_reserved_flash_size + _system_part1_flash_size + _user_part_flash_size + _bootloader_flash_size == _flash_size) &&
    (_softdevice_flash_size + _softdevice_reserved_flash_size + _monolithic_firmware_flash_size + _bootloader_flash_size == _flash_size),
    "Invalid overall flash memory map!"
);
ASSERT(
    (_bootloader_flash_start == 0xF4000) &&
    (_bootloader_flash_end - _bootloader_flash_start == _bootloader_flash_size && _bootloader_flash_size > 0) &&
    (_bootloader_vector_table_flash_end - _bootloader_vector_table_flash_start == _bootloader_vector_table_flash_size && _bootloader_vector_table_flash_size > 0) &&
    (_bootloader_module_info_flash_end - _bootloader_module_info_flash_start == _bootloader_module_info_flash_size && _bootloader_module_info_flash_size > 0),
    "Invalid bootloader flash memory map!"
);
ASSERT(
    (_user_part_flash_start == 0xD4000) &&
    (_user_part_flash_end - _user_part_flash_start == _user_part_flash_size && _user_part_flash_size > 0) &&
    (_user_part_module_info_flash_end - _user_part_module_info_flash_start == _user_part_module_info_flash_size && _user_part_module_info_flash_size > 0),
    "Invalid user part flash memory map!"
);
ASSERT(
    (_system_part1_flash_start == 0x30000) &&
    (_system_part1_flash_end - _system_part1_flash_start == _system_part1_flash_size && _system_part1_flash_size > 0) &&
    (_system_part1_vector_table_flash_end - _system_part1_vector_table_flash_start == _system_part1_vector_table_flash_size && _system_part1_vector_table_flash_size > 0) &&
    (_system_part1_module_info_flash_end - _system_part1_module_info_flash_start == _system_part1_module_info_flash_size && _system_part1_module_info_flash_size > 0),
    "Invalid system part 1 flash memory map!"
);
ASSERT(
    (_monolithic_firmware_flash_start == 0x30000) &&
    (_monolithic_firmware_flash_end - _monolithic_firmware_flash_start == _monolithic_firmware_flash_size && _monolithic_firmware_flash_size > 0) &&
    (_monolithic_firmware_vector_table_flash_end - _monolithic_firmware_vector_table_flash_start == _monolithic_firmware_vector_table_flash_size && _monolithic_firmware_vector_table_flash_size > 0) &&
    (_monolithic_firmware_module_info_flash_end - _monolithic_firmware_module_info_flash_start == _monolithic_firmware_module_info_flash_size && _monolithic_firmware_module_info_flash_size > 0),
    "Invalid monolithic firmware flash memory map!"
);
ASSERT(
    (_softdevice_reserved_flash_end - _softdevice_reserved_flash_start == _softdevice_reserved_flash_size && _softdevice_reserved_flash_size > 0) &&
    (_softdevice_flash_end - _softdevice_flash_start == _softdevice_flash_size && _softdevice_flash_size > 0),
    "Invalid softdevice flash memory map!"
);
