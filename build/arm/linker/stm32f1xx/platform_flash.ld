/*
Platform FLASH memory map for Spark Core.

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

/* Platform interrupt vector table size. Please reference to the interrupt vector table in platform startup file. */
platform_vector_table_flash_size = 268;

/* Module info size */
platform_module_info_flash_size = 24;

/* Total amount of flash available on the MCU */
platform_flash_size  = 128K;
platform_flash_start = 0x08000000;
platform_flash_end   = platform_flash_start + platform_flash_size;


/* Monolithic firmware*/
platform_monolithic_firmware_flash_size  = 108K;
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


/* Bootloader */
platform_bootloader_flash_size  = 20K;
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
    (platform_bootloader_flash_size + platform_monolithic_firmware_flash_size == platform_flash_size),
    "Invalid overall flash memory map!"
);
ASSERT (
    (platform_monolithic_firmware_flash_start == 0x8005000) &&
    (platform_monolithic_firmware_flash_end - platform_monolithic_firmware_flash_start == platform_monolithic_firmware_flash_size && platform_monolithic_firmware_flash_size > 0) &&
    (platform_monolithic_firmware_vector_table_flash_end - platform_monolithic_firmware_vector_table_flash_start == platform_monolithic_firmware_vector_table_flash_size && platform_monolithic_firmware_vector_table_flash_size > 0) &&
    (platform_monolithic_firmware_module_info_flash_end - platform_monolithic_firmware_module_info_flash_start == platform_monolithic_firmware_module_info_flash_size && platform_monolithic_firmware_module_info_flash_size > 0),
    "Invalid monolithic firmware flash memory map!"
);
ASSERT (
    (platform_bootloader_flash_start == 0x8000000) &&
    (platform_bootloader_flash_end - platform_bootloader_flash_start == platform_bootloader_flash_size && platform_bootloader_flash_size > 0) &&
    (platform_bootloader_vector_table_flash_end - platform_bootloader_vector_table_flash_start == platform_bootloader_vector_table_flash_size && platform_bootloader_vector_table_flash_size > 0) &&
    (platform_bootloader_module_info_flash_end - platform_bootloader_module_info_flash_start == platform_bootloader_module_info_flash_size && platform_bootloader_module_info_flash_size > 0),
    "Invalid bootloader flash memory map"
);
