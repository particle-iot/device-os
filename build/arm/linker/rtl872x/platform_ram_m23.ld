/*
Platform RAM memory map for Tron.

Bootloader:                                Monolithic firmware:

+-----------------------------+ 0x1007C000 +---------------------------------------+
| User backup RAM             |            | User backup RAM                       |
+-----------------------------+ 0x1007B380 +---------------------------------------+
| System backup RAM           |            | System backup RAM                     |
+-----------------------------+ 0x1007B000 +---------------------------------------+
x                                                                                  x
x                                                                                  x
+-----------------------------+ 0x000C0400 +---------------------------------------+
| Retention SRAM              |            | Retention SRAM                        |
+-----------------------------+ 0x000C0000 +---------------------------------------+
x                                                                                  x
x                                                                                  x
+-----------------------------+ 0x00090000 +---------------------------------------+
| Main stack                  |            | Main stack                            |
+-----------------------------+ 0x0008F000 +---------------------------------------+
|                             |            |                                       |
| Heap                        |            |                                       |
+~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+            |                                       |
| KM0 part1 static RAM        |            |                                       |
+~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+            | Heap                                  |
| KM0 part1 RAM code          |            |                                       |
+-----------------------------+ 0x00086000 |                                       |
|                             |            |                                       |
| KM0 MBR static RAM          |            |                                       |
+~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+            +~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+
| KM0 MBR RAM code            |            | KM0 monolithic firmware static RAM    |
+-----------------------------+            +---------------------------------------+
| KM0 MBR start table         |            | KM0 monolithic firmware RAM code      |
+-----------------------------+ 0x00082000 +---------------------------------------+

Note: Keep the diagrams up to date with the actual memory maps defined in this file.
*/

/* Total amount of RAM available on the MCU */
platform_km0_ram_size  = 56K;
platform_km0_ram_start = 0x00082000;
platform_km0_ram_end   = platform_km0_ram_start + platform_km0_ram_size;

platform_retention_ram_size = 1K;
platform_retention_ram_start = 0x000C0000;
platform_retention_ram_end = platform_retention_ram_start + platform_retention_ram_size;

/* FIXME: move to common file */
/* All backup RAM size */
platform_backup_ram_all_size = 4K;

/* User backup memory */
platform_user_backup_ram_size  = 3K;
platform_user_backup_ram_end   = 0x1007C000;
platform_user_backup_ram_start = platform_user_backup_ram_end - platform_user_backup_ram_size;

/* System backup memory */
platform_system_backup_ram_size  = platform_backup_ram_all_size - platform_user_backup_ram_size;
platform_system_backup_ram_end   = platform_user_backup_ram_start;
platform_system_backup_ram_start = platform_system_backup_ram_end - platform_system_backup_ram_size;

/* System retention RAM */
platform_system_retention_ram_size = 120;
platform_system_retention_ram_start = 0x000c0000 + 0x80 + 14 * 4;
platform_system_retention_ram_end = platform_system_retention_ram_start + platform_system_retention_ram_size;

/* Monolithic firmware main stack
 * platform_monolithic_firmware_stack_start is the lowest address of the stack. */
platform_km0_monolithic_firmware_stack_size  = 4K;
platform_km0_monolithic_firmware_stack_end   = platform_km0_ram_end;
platform_km0_monolithic_firmware_stack_start = platform_km0_monolithic_firmware_stack_end - platform_km0_monolithic_firmware_stack_size;

/* Bootloader main stack
 * platform_bootloader_stack_start is the lowest address of the stack. */
platform_km0_bootloader_stack_size  = 4K;
platform_km0_bootloader_stack_end   = platform_km0_ram_end;
platform_km0_bootloader_stack_start = platform_km0_bootloader_stack_end - platform_km0_bootloader_stack_size;


/* Bootloader RAM, contains static data and code */
platform_km0_mbr_ram_size  = 16K;
platform_km0_mbr_ram_start = platform_km0_ram_start;
platform_km0_mbr_ram_end   = platform_km0_mbr_ram_start + platform_km0_mbr_ram_size;

platform_km0_part1_ram_start = platform_km0_mbr_ram_end;
platform_km0_part1_ram_end = platform_km0_bootloader_stack_start;
platform_km0_part1_ram_size = platform_km0_part1_ram_end - platform_km0_part1_ram_start;

platform_km0_part1_dynalib_table_ram_start = platform_km0_part1_ram_start;


/* Monolithic firmware RAM, contains static data and code */
platform_km0_monolithic_firmware_ram_size  = platform_km0_ram_size - platform_km0_monolithic_firmware_stack_size;
platform_km0_monolithic_firmware_ram_start = platform_km0_ram_start;
platform_km0_monolithic_firmware_ram_end   = platform_km0_monolithic_firmware_ram_start + platform_km0_monolithic_firmware_ram_size;


/* The heap size after linkage should meet the minimum heap requirement. */
platform_heap_min_size = 32K;
