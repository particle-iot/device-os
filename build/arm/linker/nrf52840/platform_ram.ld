/*
Platform RAM memory map for Argon, Boron, Xenon, ASoM, BSoM, XSoM.

Modular firmware:                       Monolithic firmware:                          Bootloader:

+--------------------------+ 0x20040000 +--------------------------------+ 0x20040000 +-----------------------+
| User backup RAM          |            | User backup RAM                |            | User backup RAM       |
+--------------------------+ 0x2003f400 +--------------------------------+ 0x2003f400 +-----------------------+
| System flags             |            | System flags                   |            | System flags          |
+--------------------------+ 0x2003f3c0 +--------------------------------+ 0x2003f3c0 +-----------------------+
| Backup registers         |            | Backup registers               |            | Backup registers      |
+--------------------------+ 0x2003f380 +--------------------------------+ 0x2003f380 +-----------------------+
| System backup RAM        |            | System backup RAM              |            | System backup RAM     |
+--------------------------+ 0x2003f000 +--------------------------------+ 0x2003f000 +-----------------------+
| Main stack               |            | Main stack                     |            | Main stack            |
+--------------------------+ 0x2003e800 +--------------------------------+ 0x2003e800 +-----------------------+
| User part static RAM     |            |                                |            |                       |
+~~~~~~~~~~~~~~~~~~~~~~~~~~+            |                                |            |                       |
|                          |            | Heap                           |            | Heap                  |
| Heap                     |            |                                |            |                       |
|                          |            |                                |            |                       |
+~~~~~~~~~~~~~~~~~~~~~~~~~~+            +~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+            +~~~~~~~~~~~~~~~~~~~~~~~+
| System part 1 static RAM |            | Monolithic firmware static RAM |            | Bootloader static RAM |
+--------------------------+ 0x20004000 +--------------------------------+ 0x20004000 +-----------------------+
| SoftDevice RAM           |            | SoftDevice RAM                 |            | SoftDevice RAM        |
+--------------------------+ 0x20000000 +--------------------------------+ 0x20000000 +-----------------------+

Note: Keep the diagrams up to date with the actual memory maps defined in this file.
*/

/* Total amount of RAM available on the MCU */
platform_ram_size  = 256K;
platform_ram_start = 0x20000000;
platform_ram_end   = platform_ram_start + platform_ram_size;


/* All backup RAM size */
platform_backup_ram_all_size = 4K;

/* User backup memory */
platform_user_backup_ram_size  = 3K;
platform_user_backup_ram_end   = platform_ram_end;
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


/* Modular firmware main stack
 * platform_modular_firmware_stack_size is equal to 2K, which is defined in part1_build.mk and user_build.mk under modules/shared/nRF52840.
 * platform_modular_firmware_stack_start is the lowest address of the stack. */
platform_modular_firmware_stack_size  = __STACKSIZE__;
platform_modular_firmware_stack_end   = platform_system_backup_ram_start;
platform_modular_firmware_stack_start = platform_modular_firmware_stack_end - platform_modular_firmware_stack_size;

/* Monolithic firmware main stack
 * platform_monolithic_firmware_stack_size is equal to 2K, which is defined in include.mk under hal/src/nRF52840.
 * platform_monolithic_firmware_stack_start is the lowest address of the stack. */
platform_monolithic_firmware_stack_size  = __STACKSIZE__;
platform_monolithic_firmware_stack_end   = platform_system_backup_ram_start;
platform_monolithic_firmware_stack_start = platform_monolithic_firmware_stack_end - platform_monolithic_firmware_stack_size;

/* Bootloader main stack
 * platform_bootloader_stack_size is equal to 8K, which is defined in bootloader/src/nRF52840/include.mk. */
platform_bootloader_stack_size  = __STACKSIZE__;
platform_bootloader_stack_end   = platform_system_backup_ram_start;
platform_bootloader_stack_start = platform_bootloader_stack_end - platform_bootloader_stack_size;


/* User part
 * platform_user_part_static_ram_size is referenced as a constant value when linking system part 1.
 * platform_user_part_static_ram_size is defined in platform_user_ram.ld, which is generated by modules/shared/nRF52840/bbuild_linker_script.mk, when linking the user part.
 * platform_user_part_static_ram_size is preallocated to be large enough during the user part first link stage. 
 * platform_user_part_static_ram_size is decreased to that just fits the user part's .data and .bss sections during the user part final link stage.
 * platform_user_part_static_ram_start is passed to system part 1 at startup for calculating the actual END of the heap for modular firmware. */
platform_user_part_static_ram_size  = DEFINED(platform_user_part_static_ram_size) ? platform_user_part_static_ram_size : 96K;
platform_user_part_static_ram_end   = platform_modular_firmware_stack_start;
platform_user_part_static_ram_start = platform_user_part_static_ram_end - platform_user_part_static_ram_size;


/* SoftDevice */
platform_softdevice_ram_size  = 23K;
platform_softdevice_ram_start = platform_ram_start;
platform_softdevice_ram_end   = platform_softdevice_ram_start + platform_softdevice_ram_size;


/* System part 1
 * platform_system_part1_static_ram_size is preallocated large enough to fit the system part's .data and .bss sections.
 * The actual end of the system part static ram is calculated after linkage, and it is the actual START of the heap for modular firmware. */
platform_system_part1_static_ram_size  = platform_ram_size - platform_backup_ram_all_size - platform_modular_firmware_stack_size - platform_user_part_static_ram_size - platform_softdevice_ram_size;
platform_system_part1_static_ram_start = platform_softdevice_ram_end;
platform_system_part1_static_ram_end   = platform_system_part1_static_ram_start + platform_system_part1_static_ram_size;


/* Monolithic firmware
 * platform_monolithic_firmware_static_ram_size is preallocated large enough to fit the monolithic firmware's .data and .bss sections.
 * platform_monolithic_firmware_static_ram_end is the END of the heap for monolithic firmware.
 * The actual end of the monolithic firmware static ram is calculated after linkage, and it is the actual START of the heap for monolithic firmware. */
platform_monolithic_firmware_static_ram_size  = platform_ram_size - platform_backup_ram_all_size - platform_monolithic_firmware_stack_size - platform_softdevice_ram_size;
platform_monolithic_firmware_static_ram_start = platform_softdevice_ram_end;
platform_monolithic_firmware_static_ram_end   = platform_monolithic_firmware_static_ram_start + platform_monolithic_firmware_static_ram_size;


/* Bootloader
 * platform_bootloader_static_ram_size is preallocated large enough to fit the bootloader's .data and .bss sections.
 * platform_bootloader_static_ram_end is the END of heap for bootloader.
 * The actual end of the bootloader static ram is calculated after linkage, and it is the actual START of the heap for bootloader. */
platform_bootloader_static_ram_size  = platform_ram_size - platform_backup_ram_all_size - platform_bootloader_stack_size - platform_softdevice_ram_size;
platform_bootloader_static_ram_start = platform_softdevice_ram_end;
platform_bootloader_static_ram_end   = platform_bootloader_static_ram_start + platform_bootloader_static_ram_size;


/* The heap size after linkage should meet the minimum heap requirement. */
platform_heap_min_size = 48K;
platform_heap_modular_max_location_end = platform_modular_firmware_stack_start;

/* Basic sanity checks */
ASSERT (
    (platform_ram_end - platform_ram_start == platform_ram_size && platform_ram_size > 0) &&
    (platform_backup_ram_all_size + platform_modular_firmware_stack_size + platform_user_part_static_ram_size + platform_system_part1_static_ram_size + platform_softdevice_ram_size <= platform_ram_size) &&
    (platform_backup_ram_all_size + platform_monolithic_firmware_stack_size + platform_monolithic_firmware_static_ram_size + platform_softdevice_ram_size == platform_ram_size) &&
    (platform_backup_ram_all_size + platform_bootloader_stack_size + platform_bootloader_static_ram_size + platform_softdevice_ram_size == platform_ram_size),
    "Invalid overall RAM memory map!"
);
ASSERT (
    (platform_system_backup_ram_end - platform_system_backup_ram_start == platform_system_backup_ram_size && platform_system_backup_ram_size > 0) &&
    (platform_backup_registers_ram_end - platform_backup_registers_ram_start == platform_backup_registers_ram_size && platform_backup_registers_ram_size > 0) &&
    (platform_system_flags_ram_end - platform_system_flags_ram_start == platform_system_flags_ram_size && platform_system_flags_ram_size > 0) &&
    (platform_user_backup_ram_end - platform_user_backup_ram_start == platform_user_backup_ram_size && platform_user_backup_ram_size > 0),
    "Invalid backup RAM memory map!"
);
ASSERT (
    (platform_modular_firmware_stack_end - platform_modular_firmware_stack_start == platform_modular_firmware_stack_size && platform_modular_firmware_stack_size > 0) &&
    (platform_monolithic_firmware_stack_end - platform_monolithic_firmware_stack_start == platform_monolithic_firmware_stack_size && platform_monolithic_firmware_stack_size > 0) &&
    (platform_bootloader_stack_end - platform_bootloader_stack_start == platform_bootloader_stack_size && platform_bootloader_stack_size > 0),
    "Invalid stack memory map!"
);
ASSERT (
    (platform_user_part_static_ram_end - platform_user_part_static_ram_start == platform_user_part_static_ram_size && platform_user_part_static_ram_size > 0),
    "Invalid user part RAM memory map!"
);
ASSERT (
    (platform_system_part1_static_ram_end - platform_system_part1_static_ram_start == platform_system_part1_static_ram_size && platform_system_part1_static_ram_size > 0),
    "Invalid system part 1 RAM memory map!"
);
ASSERT (
    (platform_monolithic_firmware_static_ram_end - platform_monolithic_firmware_static_ram_start == platform_monolithic_firmware_static_ram_size && platform_monolithic_firmware_static_ram_size > 0),
    "Invalid monolithic firmware RAM memory map!"
);
ASSERT (
    (platform_bootloader_static_ram_end - platform_bootloader_static_ram_start == platform_bootloader_static_ram_size && platform_bootloader_static_ram_size > 0),
    "Invalid bootloader RAM memory map!"
);
ASSERT (
    (platform_softdevice_ram_end - platform_softdevice_ram_start == platform_softdevice_ram_size && platform_softdevice_ram_size > 0),
    "Invalid softdevice RAM memory map!"
);
