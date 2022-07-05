/*
Platform RAM memory map for Tron.

Bootloader:                                Modular firmware:                           Monolithic firmware:

+-----------------------------+ 0x10080000 +-------------------------------+ 0x10080000 +------------------------------------+
| Secure main stack           |            | Secure main stack             |            | Secure main stack                  |
+-----------------------------+ 0x1007F000 +-------------------------------+            +------------------------------------+
|                             |            |                               |            |                                    |
| Bootloader secure RAM code  |            | User part secure RAM code     |            | Monolithic firmware                |
|                             | 0x1007E000 +-------------------------------+ 0x1007E000 | secure RAM code                    |
+-----------------------------+            |                               |            |                                    |
| Bootloader start table      |            | System part 1 secure RAM code |            |                                    |
+-----------------------------+ 0x1007D000 +-------------------------------+ 0x1007D000 +------------------------------------+
x                                                                                                                            x
x                                                                                                                            x
+-----------------------------+ 0x1007C000 +-------------------------------+ 0x1007C000 +------------------------------------+
| User backup RAM             |            | User backup RAM               |            | User backup RAM                    |
+-----------------------------+ 0x1007B380 +-------------------------------+ 0x1007B380 +------------------------------------+
| System backup RAM           |            | System backup RAM             |            | System backup RAM                  |
+-----------------------------+ 0x1007B000 +-------------------------------+ 0x1007B000 +------------------------------------+
|                             |            | User part static RAM          |            |                                    |
| Heap                        |            +~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+            |                                    |
|                             |            |                               |            | Heap                               |
+~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+            | Heap                          |            |                                    |
| Bootloader static RAM       |            |                               |            |                                    |
+~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+            +~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+            +~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+
| Bootloader RAM code         |            | System part 1 static RAM      |            | Monolithic firmware static RAM     |
+-----------------------------+ 0x10005000 +-------------------------------+ 0x10005000 +------------------------------------+
| Non-secure stack            |            | Non-secure stack              |            | Non-secure stack                   |
+-----------------------------+ 0x10004000 +-------------------------------+ 0x10004000 +------------------------------------+
x                                                                                                                            x
x                                                                                                                            x
+-----------------------------+ 0x02400000 +-------------------------------+ 0x02400000 +------------------------------------+
|                             |            | User part static RAM alt      |            |                                    |
|                             |            +~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+            |                                    |
|                             |            | User part PSRAM code          |            |                                    |
|                             |            +~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+            |                                    |
|                             |            | User part dynalib table       |            | Heap alt                           |
|                             |            +~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+            |                                    |
|        NOT USED             |            | Heap alt                      |            |                                    |
|                             |            +~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+            +~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+
|                             |            | System part 1 static RAM alt  |            | Monolithic firmware static RAM alt |
|                             |            +~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+            +~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+
|                             |            | System part 1 PSRAM code      |            |                                    |
|                             |            +~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+            |                                    |
|                             |            | System part 1 dynalib table   |            | Monolithic firmware PSRAM code     |
|-----------------------------| 0x02000000 +-------------------------------+ 0x02000000 +------------------------------------+

Note: Keep the diagrams up to date with the actual memory maps defined in this file.
*/

/* Total amount of RAM available on the MCU */
platform_ram_size  = 476K;
platform_ram_start = 0x10005000;
platform_ram_end   = platform_ram_start + platform_ram_size;

platform_secure_ram_size  = 12K;
platform_secure_ram_start = 0x1007D000;
platform_secure_ram_end   = platform_secure_ram_start + platform_secure_ram_size;

platform_psram_size  = 4M;
platform_psram_start = 0x02000000;
platform_psram_end   = platform_psram_start + platform_psram_size;


/* FIXME: move to common file */
/* All backup RAM size */
platform_backup_ram_all_size = 4K;

/* User backup memory */
platform_user_backup_ram_size  = 3K;
platform_user_backup_ram_end   = platform_ram_end;
platform_user_backup_ram_start = platform_user_backup_ram_end - platform_user_backup_ram_size;

/* System backup memory */
platform_system_backup_ram_size  = platform_backup_ram_all_size - platform_user_backup_ram_size;
platform_system_backup_ram_end   = platform_user_backup_ram_start;
platform_system_backup_ram_start = platform_system_backup_ram_end - platform_system_backup_ram_size;

platform_backup_ram_all_start = platform_system_backup_ram_start;
platform_backup_ram_all_end = platform_system_backup_ram_start + platform_backup_ram_all_size;

/* System retention RAM */
platform_system_retention_ram_size = 120;
platform_system_retention_ram_start = 0x000c0000 + 0x80 + 14 * 4;
platform_system_retention_ram_end = platform_system_retention_ram_start + platform_system_retention_ram_size;

/* Modular firmware main stack
 * platform_modular_firmware_stack_size is equal to 8K, which is defined in part1_build.mk and user_build.mk under modules/shared/rtl872x.
 * platform_modular_firmware_stack_start is the lowest address of the stack. */
platform_modular_firmware_stack_size  = __STACKSIZE__;
platform_modular_firmware_stack_end   = platform_ram_start;
platform_modular_firmware_stack_start = platform_modular_firmware_stack_end - platform_modular_firmware_stack_size;

platform_modular_firmware_secure_stack_size  = __STACKSIZE__;
platform_modular_firmware_secure_stack_end   = platform_secure_ram_end;
platform_modular_firmware_secure_stack_start = platform_modular_firmware_secure_stack_end - platform_modular_firmware_secure_stack_size;

/* Monolithic firmware main stack
 * platform_monolithic_firmware_stack_size is equal to 4K, which is defined in include.mk under hal/src/rtl872x.
 * platform_monolithic_firmware_stack_start is the lowest address of the stack. */
platform_monolithic_firmware_stack_size  = __STACKSIZE__;
platform_monolithic_firmware_stack_end   = platform_ram_start;
platform_monolithic_firmware_stack_start = platform_monolithic_firmware_stack_end - platform_monolithic_firmware_stack_size;

platform_monolithic_firmware_secure_stack_size  = __STACKSIZE__;
platform_monolithic_firmware_secure_stack_end   = platform_secure_ram_end;
platform_monolithic_firmware_secure_stack_start = platform_monolithic_firmware_secure_stack_end - platform_monolithic_firmware_secure_stack_size;

/* Bootloader main stack
 * platform_bootloader_stack_size is equal to 4K, which is defined in bootloader/src/rtl872x/include.mk. */
platform_bootloader_stack_size  = __STACKSIZE__;
platform_bootloader_stack_end   = platform_secure_ram_end;
platform_bootloader_stack_start = platform_bootloader_stack_end - platform_bootloader_stack_size;


/* User part static RAM
 * platform_user_part_static_ram_size is referenced as a constant value when linking system part 1.
 * platform_user_part_static_ram_size is defined in platform_user_ram.ld, which is generated by modules/shared/rtl872x/build_linker_script.mk, when linking the user part.
 * platform_user_part_static_ram_size is preallocated to be large enough during the user part first link stage. 
 * platform_user_part_static_ram_size is decreased to that just fits the user part's .data and .bss sections during the user part final link stage.
 * platform_user_part_static_ram_start is passed to system part 1 at startup for calculating the actual END of the heap for modular firmware. */
platform_user_part_static_ram_size  = DEFINED(platform_user_part_static_ram_size) ? platform_user_part_static_ram_size : 128K;
platform_user_part_static_ram_end   = platform_system_backup_ram_start;
platform_user_part_static_ram_start = platform_user_part_static_ram_end - platform_user_part_static_ram_size;

/* User part PSRAM */
platform_user_part_psram_size  = DEFINED(platform_user_part_psram_size) ? platform_user_part_psram_size : 2M;
platform_user_part_psram_end   = platform_psram_end;
platform_user_part_psram_start = platform_user_part_psram_end - platform_user_part_psram_size;

/* User part secure RAM code */
platform_user_part_secure_ram_size  = 4K;
platform_user_part_secure_ram_end   = platform_modular_firmware_secure_stack_start;
platform_user_part_secure_ram_start = platform_user_part_secure_ram_end - platform_user_part_secure_ram_size;


/* System part 1 static RAM
 * platform_system_part1_static_ram_size is preallocated large enough to fit the system part's .data and .bss sections.
 * The actual end of the system part static ram is calculated after linkage, and it is the actual START of the heap for modular firmware. */
platform_system_part1_static_ram_size  = platform_ram_size - platform_backup_ram_all_size - platform_modular_firmware_stack_size - platform_user_part_static_ram_size;
platform_system_part1_static_ram_start = platform_ram_start;
platform_system_part1_static_ram_end   = platform_system_part1_static_ram_start + platform_system_part1_static_ram_size;

/* System part 1 PSRAM code */
platform_system_part1_psram_size  = platform_psram_size - platform_user_part_psram_size;
platform_system_part1_psram_start = platform_psram_start;
platform_system_part1_psram_end   = platform_system_part1_psram_start + platform_system_part1_psram_size;

/* System part 1 secure RAM code */
platform_system_part1_secure_ram_size  = platform_secure_ram_size - platform_modular_firmware_secure_stack_size - platform_user_part_secure_ram_size;
platform_system_part1_secure_ram_start = platform_secure_ram_start;
platform_system_part1_secure_ram_end   = platform_system_part1_secure_ram_start + platform_system_part1_secure_ram_size;


/* Monolithic firmware static RAM
 * platform_monolithic_firmware_static_ram_size is preallocated large enough to fit the monolithic firmware's .data and .bss sections.
 * platform_monolithic_firmware_static_ram_end is the END of the heap for monolithic firmware.
 * The actual end of the monolithic firmware static ram is calculated after linkage, and it is the actual START of the heap for monolithic firmware. */
platform_monolithic_firmware_static_ram_size  = platform_ram_size - platform_backup_ram_all_size - platform_monolithic_firmware_stack_size;
platform_monolithic_firmware_static_ram_start = platform_ram_start;
platform_monolithic_firmware_static_ram_end   = platform_monolithic_firmware_static_ram_start + platform_monolithic_firmware_static_ram_size;

/* Monolithic firmware PSRAM code */
platform_monolithic_firmware_psram_size  = platform_psram_size;
platform_monolithic_firmware_psram_start = platform_psram_start;
platform_monolithic_firmware_psram_end   = platform_monolithic_firmware_psram_start + platform_monolithic_firmware_psram_size;

/* Monolithic firmware secure RAM code */
platform_monolithic_firmware_secure_ram_size  = platform_secure_ram_size - platform_monolithic_firmware_secure_stack_size;
platform_monolithic_firmware_secure_ram_start = platform_secure_ram_start;
platform_monolithic_firmware_secure_ram_end   = platform_monolithic_firmware_secure_ram_start + platform_monolithic_firmware_secure_ram_size;


/* Bootloader RAM, contains static data and code
 * platform_bootloader_ram_size is preallocated large enough to fit the bootloader's .data and .bss sections.
 * platform_bootloader_ram_end is the END of heap for bootloader.
 * The actual end of the bootloader static ram is calculated after linkage, and it is the actual START of the heap for bootloader. */
platform_bootloader_ram_size  = platform_ram_size - platform_backup_ram_all_size;
platform_bootloader_ram_start = platform_ram_start;
platform_bootloader_ram_end   = platform_bootloader_ram_start + platform_bootloader_ram_size;

platform_bootloader_psram_size  = platform_psram_size;
platform_bootloader_psram_start = platform_psram_start;
platform_bootloader_psram_end   = platform_bootloader_psram_start + platform_bootloader_psram_size;

/* Bootloader secure RAM code */
platform_bootloader_secure_ram_size  = platform_secure_ram_size - platform_bootloader_stack_size;
platform_bootloader_secure_ram_start = platform_secure_ram_start;
platform_bootloader_secure_ram_end   = platform_bootloader_secure_ram_start + platform_bootloader_secure_ram_size;


/* The heap size after linkage should meet the minimum heap requirement. */
platform_heap_min_size = 32K;
