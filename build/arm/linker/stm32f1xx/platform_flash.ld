/*
Platform FLASH memory map for Spark Core (Medium Desity).

Monolithic firmware:

+----------------------------------+ 0x08020000
| Monolithic firmware              |
+----------------------------------+ 0x08005124
| Monolithic firmware module info  |
+----------------------------------+ 0x0800510C
| Monolithic firmware vector table |
+----------------------------------+ 0x08005000
| Bootloader                       |
+----------------------------------+ 0x08000124
| Bootloader module info           |
+----------------------------------+ 0x0800010C
| Bootloader vector table          |
+----------------------------------+ 0x08000000

Note: Keep the diagrams up to date with the actual memory maps defined in this file.
*/

/* This is only referenced in this file */
_platform_vector_table_flash_size = 268;

/* This is only referenced in this file */
_module_info_flash_size = 24;

/* Total amount of flash available on the MCU */
_flash_size  = 128K;
_flash_start = 0x08000000;
_flash_end   = _flash_start + _flash_size;

/* Monolithic firmware*/
_monolithic_firmware_flash_size  = 108K;
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

/* Bootloader */
_bootloader_flash_size  = 20K;
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
ASSERT (
    (_flash_end - _flash_start == _flash_size && _flash_size > 0) &&
    (_bootloader_flash_size + _monolithic_firmware_flash_size == _flash_size),
    "Invalid overall flash memory map!"
);
ASSERT (
    (_monolithic_firmware_flash_start == 0x8005000) &&
    (_monolithic_firmware_flash_end - _monolithic_firmware_flash_start == _monolithic_firmware_flash_size && _monolithic_firmware_flash_size > 0) &&
    (_monolithic_firmware_vector_table_flash_end - _monolithic_firmware_vector_table_flash_start == _monolithic_firmware_vector_table_flash_size && _monolithic_firmware_vector_table_flash_size > 0) &&
    (_monolithic_firmware_module_info_flash_end - _monolithic_firmware_module_info_flash_start == _monolithic_firmware_module_info_flash_size && _monolithic_firmware_module_info_flash_size > 0),
    "Invalid monolithic firmware flash memory map!"
);
ASSERT (
    (_bootloader_flash_start == 0x8000000) &&
    (_bootloader_flash_end - _bootloader_flash_start == _bootloader_flash_size && _bootloader_flash_size > 0) &&
    (_bootloader_vector_table_flash_end - _bootloader_vector_table_flash_start == _bootloader_vector_table_flash_size && _bootloader_vector_table_flash_size > 0) &&
    (_bootloader_module_info_flash_end - _bootloader_module_info_flash_start == _bootloader_module_info_flash_size && _bootloader_module_info_flash_size > 0),
    "Invalid bootloader flash memory map"
);
