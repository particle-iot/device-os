/*
Platform FLASH memory map for Tron.

Monolithic firmware:

+---------------------------------------+ 0x08800000
|                                       |
| Reserved 2                            |
|                                       |
+---------------------------------------+ 0x0801C000
| Monolithic firmware RAM code/data     |
+~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+
| Monolithic firmware XIP code          |
+---------------------------------------+ 0x08014018
| Monolithic firmware module info       |
+---------------------------------------+ 0x08014000
|                                       |
| Reserved 1                            |
|                                       |
+---------------------------------------+ 0x08004000
| System data                           |
+---------------------------------------+ 0x08003000
| Backup                                |
+---------------------------------------+ 0x08002000
| Bootloader RAM code/data              |
+~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+
| Bootloader RAM header                 |
+~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+
| Bootloader XIP code                   |
+---------------------------------------+ 0x08000058
| Bootloader module info                |
+---------------------------------------+ 0x08000040
| Bootloader XIP header                 |
+---------------------------------------+ 0x08000000

Note: Keep the diagrams up to date with the actual memory maps defined in this file.
*/

/* Platform interrupt vector table size. Please reference to the interrupt vector table in platform startup file. */
platform_vector_table_flash_size = 164;

/* Module info size */
platform_module_info_flash_size = 24;

/* Realtek specific image header */
platform_image_header_flash_size = 64;


/* Total amount of flash available on the MCU */
platform_flash_size  = 8M;
platform_flash_start = 0x08000000;
platform_flash_end   = platform_flash_start + platform_flash_size;


/* Bootloader */
platform_bootloader_flash_size  = 8K;
platform_bootloader_flash_start = platform_flash_start;
platform_bootloader_flash_end   = platform_bootloader_flash_start + platform_bootloader_flash_size;

/* Bootloader module info */
platform_bootloader_module_info_flash_size  = platform_module_info_flash_size;
platform_bootloader_module_info_flash_start = platform_bootloader_flash_start + platform_image_header_flash_size;
platform_bootloader_module_info_flash_end   = platform_bootloader_module_info_flash_start + platform_bootloader_module_info_flash_size;


/* Realtek specific backup region */
platform_backup_flash_size  = 4K;
platform_backup_flash_start = platform_bootloader_flash_end;
platform_backup_flash_end   = platform_backup_flash_start + platform_backup_flash_size;


/* Realtek specific system data */
platform_system_data_flash_size  = 4K;
platform_system_data_flash_start = platform_backup_flash_end;
platform_system_data_flash_end   = platform_system_data_flash_start + platform_system_data_flash_size;


/* Reserved for KM4 bootloader, see platform_flash.ld */
platform_reserved1_flash_size  = 64K;
platform_reserved1_flash_start = platform_system_data_flash_end;
platform_reserved1_flash_end   = platform_reserved1_flash_start + platform_reserved1_flash_size;


/* Monolithic firmware*/
platform_monolithic_firmware_flash_size  = 32K;
platform_monolithic_firmware_flash_start = platform_reserved1_flash_end;
platform_monolithic_firmware_flash_end   = platform_monolithic_firmware_flash_start + platform_monolithic_firmware_flash_size;

/* Monolithic firmware module info */
platform_monolithic_firmware_module_info_flash_size  = platform_module_info_flash_size;
platform_monolithic_firmware_module_info_flash_start = platform_monolithic_firmware_flash_start;
platform_monolithic_firmware_module_info_flash_end   = platform_monolithic_firmware_module_info_flash_start + platform_monolithic_firmware_module_info_flash_size;


/* Reserved for KM4 application */
platform_reserved2_flash_end   = 0x08400000;
platform_reserved2_flash_start = platform_monolithic_firmware_flash_end;
platform_reserved2_flash_size  = platform_monolithic_firmware_flash_end - platform_reserved2_flash_start;
