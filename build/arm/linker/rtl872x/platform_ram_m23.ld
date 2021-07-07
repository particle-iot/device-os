/*
Platform RAM memory map for Tron.

Bootloader:                                Monolithic firmware:

+-----------------------------+ 0x1007C000 +-----------------------+
| User backup RAM             |            | User backup RAM       |
+-----------------------------+ 0x1007B400 +-----------------------+
| System flags                |            | System flags          |
+-----------------------------+ 0x1007B3C0 +-----------------------+
| Backup registers            |            | Backup registers      |
+-----------------------------+ 0x1007B380 +-----------------------+
| System backup RAM           |            | System backup RAM     |
+-----------------------------+ 0x1007B000 +-----------------------+
x                                                                  x
x                                                                  x
+-----------------------------+ 0x00090000 +-----------------------+
| Main stack                  |            | Main stack            |
+-----------------------------+ 0x0008F000 +-----------------------+
| Heap                        |            | Heap                  |
+~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+            +~~~~~~~~~~~~~~~~~~~~~~~+
| KM0 bootloader static RAM   |            | KM0 app static RAM    |
+~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+            +~~~~~~~~~~~~~~~~~~~~~~~+
| KM0 bootloader RAM code     |            | KM0 app RAM code      |
+-----------------------------+ 0x00082000 +-----------------------+

Note: Keep the diagrams up to date with the actual memory maps defined in this file.
*/

/* Total amount of RAM available on the MCU */
platform_ram_size  = 56K;
platform_ram_start = 0x00082000;
platform_ram_end   = platform_ram_start + platform_ram_size;


/* All backup RAM size */
platform_backup_ram_all_size = 4K;

/* User backup memory */
platform_user_backup_ram_size  = 3K;
platform_user_backup_ram_end   = 0x1007C000;
platform_user_backup_ram_start = platform_user_backup_ram_end - platform_user_backup_ram_size;

/* System flags */
platform_system_flags_ram_size  = 64;
platform_system_flags_ram_end   = platform_user_backup_ram_start;
platform_system_flags_ram_start = platform_system_flags_ram_end - platform_system_flags_ram_size;

/* Backup registers */
platform_backup_registers_ram_size  = 64;
platform_backup_registers_ram_end   = platform_system_flags_ram_start;
platform_backup_registers_ram_start = platform_backup_registers_ram_end - platform_backup_registers_ram_size;

/* System backup memory */
platform_system_backup_ram_size  = platform_backup_ram_all_size - platform_user_backup_ram_size - platform_system_flags_ram_size - platform_backup_registers_ram_size;
platform_system_backup_ram_end   = platform_backup_registers_ram_start;
platform_system_backup_ram_start = platform_system_backup_ram_end - platform_system_backup_ram_size;


/* Monolithic firmware main stack
 * platform_monolithic_firmware_stack_start is the lowest address of the stack. */
platform_monolithic_firmware_stack_size  = 4K;
platform_monolithic_firmware_stack_end   = platform_ram_end;
platform_monolithic_firmware_stack_start = platform_monolithic_firmware_stack_end - platform_monolithic_firmware_stack_size;

/* Bootloader main stack
 * platform_bootloader_stack_start is the lowest address of the stack. */
platform_bootloader_stack_size  = 4K;
platform_bootloader_stack_end   = platform_ram_end;
platform_bootloader_stack_start = platform_bootloader_stack_end - platform_bootloader_stack_size;


/* Bootloader static RAM */
platform_bootloader_static_ram_size  = platform_ram_size - platform_bootloader_stack_size;
platform_bootloader_static_ram_start = platform_ram_start;
platform_bootloader_static_ram_end   = platform_bootloader_static_ram_start + platform_bootloader_static_ram_size;

/* Bootloader RAM code, overlapped with bootloader static RAM */
platform_bootloader_ram_code_size  = platform_bootloader_static_ram_size;
platform_bootloader_ram_code_start = platform_ram_start;
platform_bootloader_ram_code_end   = platform_bootloader_ram_code_start + platform_bootloader_ram_code_size;


/* Monolithic firmware static RAM */
platform_monolithic_firmware_static_ram_size  = platform_ram_size - platform_monolithic_firmware_stack_size;
platform_monolithic_firmware_static_ram_start = platform_ram_start;
platform_monolithic_firmware_static_ram_end   = platform_monolithic_firmware_static_ram_start + platform_monolithic_firmware_static_ram_size;

/* Monolithic firmware RAM code, overlapped with monolithic firmware static RAM */
platform_monolithic_firmware_ram_code_size  = platform_monolithic_firmware_static_ram_size;
platform_monolithic_firmware_ram_code_start = platform_ram_start;
platform_monolithic_firmware_ram_code_end   = platform_monolithic_firmware_ram_code_start + platform_monolithic_firmware_ram_code_size;


/* The heap size after linkage should meet the minimum heap requirement. */
platform_heap_min_size = 32K;
