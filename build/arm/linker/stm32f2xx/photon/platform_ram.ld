/*
Platform RAM memory map for Photon and P1.

Modular firmware:                       Monolithic firmware:                          Bootloader:

+--------------------------+ 0x40025000 +--------------------------------+ 0x40025000 +--------------------------+
| System part backup RAM   |            | System part backup RAM         |            | System part backup RAM   |
+--------------------------+ 0x40024C00 +--------------------------------+ 0x40024C00 +--------------------------+
| User part backup RAM     |            | User part backup RAM           |            | User part backup RAM     |
+--------------------------+ 0x40024000 +--------------------------------+ 0x40024000 +--------------------------+
x                          x            x                                x            x                          x
x                          x            x                                x            x                          x
+--------------------------+ 0x20020000 +--------------------------------+ 0x20020000 +--------------------------+
| Main stack               |            | Main stack                     |            | Main stack               |
+--------------------------+ 0x2001f000 +--------------------------------+ 0x2001f000 +--------------------------+
| System part 2 static RAM |            |                                |            | System part 2 static RAM |
+--------------------------+ 0x20014800 |                                | 0x20014800 +--------------------------+
| System part 1 static RAM |            | Heap                           |            | System part 1 static RAM |
+--------------------------+ 0x20014400 |                                | 0x20014400 +--------------------------+
|                          |            |                                |            | Heap                     |
| Heap                     |            +~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+            +~~~~~~~~~~~~~~~~~~~~~~~~~~+
|                          |            |                                |            | Bootloader static RAM    |
+~~~~~~~~~~~~~~~~~~~~~~~~~~+            | Monolithic firmware static RAM | 0x20002000 +--------------------------+
| User part static RAM     |            |                                |            | User part reserved       |
+--------------------------+ 0x20000000 +--------------------------------+ 0x20000000 +--------------------------+

Note: Keep the diagrams up to date with the actual memory maps defined in this file.
*/

/* Total amount of RAM available on the MCU */
platform_ram_size  = 128K;
platform_ram_start = 0x20000000;
platform_ram_end   = platform_ram_start + platform_ram_size;


/* Independent backup RAM */
platform_backup_ram_size  = 4K;
platform_backup_ram_start = 0x40024000;
platform_backup_ram_end   = platform_backup_ram_start + platform_backup_ram_size;

/* System backup RAM */
platform_system_backup_ram_size  = 1K;
platform_system_backup_ram_end   = platform_backup_ram_end;
platform_system_backup_ram_start = platform_system_backup_ram_end - platform_system_backup_ram_size;

/* User backup RAM */
platform_user_backup_ram_size  = 3K;
platform_user_backup_ram_end   = platform_system_backup_ram_start;
platform_user_backup_ram_start = platform_user_backup_ram_end - platform_user_backup_ram_size;


/* Modular firmware main stack
 * platform_modular_firmware_stack_start is the lowest address of the stack. */
platform_modular_firmware_stack_size  = 4K;
platform_modular_firmware_stack_end   = platform_ram_end;
platform_modular_firmware_stack_start = platform_modular_firmware_stack_end - platform_modular_firmware_stack_size;

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


/* System part 2
 * platform_system_part2_static_ram_size is preallocated appropriately to *JUST* fit the system part2's .data and .bss sections. */
platform_system_part2_static_ram_size  = 42K;
platform_system_part2_static_ram_end   = platform_modular_firmware_stack_start;
platform_system_part2_static_ram_start = platform_system_part2_static_ram_end - platform_system_part2_static_ram_size;


/* System part 1
 * platform_system_part1_static_ram_size is preallocated appropriately to *JUST* fit the system part1's .data and .bss sections.
 * platform_system_part1_static_ram_start is the END of heap for modular firmware. */
platform_system_part1_static_ram_size  = 1K;
platform_system_part1_static_ram_end   = platform_system_part2_static_ram_start;
platform_system_part1_static_ram_start = platform_system_part1_static_ram_end - platform_system_part1_static_ram_size;


/* User part
 * platform_user_part_static_ram_size is preallocated large enough to fit the user part's .data and .bss sections.
 * The actual end of the user part static ram is calculated after linkage.
 * platform_user_part_static_ram_end is passed to system part 2 at startup for calculating the actual START of the heap for modular firmware.*/
platform_user_part_static_ram_size  = platform_ram_size - platform_modular_firmware_stack_size - platform_system_part2_static_ram_size - platform_system_part1_static_ram_size;
platform_user_part_static_ram_start = platform_ram_start;
platform_user_part_static_ram_end   = platform_ram_start + platform_user_part_static_ram_size;


/* Monolithic firmware
 * platform_monolithic_firmware_static_ram_size is preallocated large enough to fit the monolithic firmware's .data and .bss sections.
 * platform_monolithic_firmware_static_ram_end is the END of the heap for monolithic firmware.
 * The actual end of the monolithic firmware static ram is calculated after linkage, and it is the actual START of the heap for monolithic firmware. */
platform_monolithic_firmware_static_ram_size  = platform_ram_size - platform_monolithic_firmware_stack_size;
platform_monolithic_firmware_static_ram_start = platform_ram_start;
platform_monolithic_firmware_static_ram_end   = platform_monolithic_firmware_static_ram_start + platform_monolithic_firmware_static_ram_size;

/* Bootloader
 * FIXME: _bootloader_static_ram_offset for backwards compatibility?
 * Ensure RAM region doesn't overlap with the system modules, since the bootloader imports dynalib functions dynamically on certain platforms.
 * platform_bootloader_static_ram_size is preallocated large enough to fit the bootloader's .data and .bss sections.
 * platform_bootloader_static_ram_end is the END of heap for bootloader.
 * The actual end of the bootloader static ram is calculated after linkage, and it is the actual START of the heap for bootloader. */
platform_bootloader_static_ram_offset = 8K;
platform_bootloader_static_ram_size   = platform_ram_size - platform_bootloader_stack_size - platform_system_part2_static_ram_size - platform_system_part1_static_ram_size - platform_bootloader_static_ram_offset;
platform_bootloader_static_ram_start  = platform_ram_start + platform_bootloader_static_ram_offset;
platform_bootloader_static_ram_end    = platform_bootloader_static_ram_start + platform_bootloader_static_ram_size;


/* The heap size after linkage should meet the minimum heap requirement.  */
platform_heap_min_size = 16K;


/* Basic sanity checks */
ASSERT (
    (platform_ram_end - platform_ram_start == platform_ram_size && platform_ram_size > 0) &&
    (platform_modular_firmware_stack_size + platform_system_part2_static_ram_size + platform_system_part1_static_ram_size + platform_user_part_static_ram_size == platform_ram_size) &&
    (platform_monolithic_firmware_stack_size + platform_monolithic_firmware_static_ram_size == platform_ram_size) &&
    (platform_bootloader_stack_size + platform_system_part2_static_ram_size + platform_system_part1_static_ram_size + platform_bootloader_static_ram_size + platform_bootloader_static_ram_offset == platform_ram_size),
    "Invalid overall RAM memory map!"
);
ASSERT (
    (platform_system_backup_ram_size + platform_user_backup_ram_size == platform_backup_ram_size) &&
    (platform_backup_ram_end - platform_backup_ram_start == platform_backup_ram_size && platform_backup_ram_size > 0) &&
    (platform_system_backup_ram_end - platform_system_backup_ram_start == platform_system_backup_ram_size && platform_system_backup_ram_size > 0) &&
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
    (platform_system_part2_static_ram_end - platform_system_part2_static_ram_start == platform_system_part2_static_ram_size && platform_system_part2_static_ram_size > 0),
    "Invalid system part 2 RAM memory map!"
);
ASSERT (
    (platform_system_part1_static_ram_end - platform_system_part1_static_ram_start == platform_system_part1_static_ram_size && platform_system_part1_static_ram_size > 0),
    "Invalid system part 1 RAM memory map!"
);
ASSERT (
    (platform_user_part_static_ram_end - platform_user_part_static_ram_start == platform_user_part_static_ram_size && platform_user_part_static_ram_size > 0),
    "Invalid user part RAM memory map!"
);
ASSERT (
    (platform_monolithic_firmware_static_ram_end - platform_monolithic_firmware_static_ram_start == platform_monolithic_firmware_static_ram_size && platform_monolithic_firmware_static_ram_size > 0),
    "Invalid monolithic firmware RAM memory map!"
);
ASSERT (
    (platform_bootloader_static_ram_end - platform_bootloader_static_ram_start == platform_bootloader_static_ram_size && platform_bootloader_static_ram_size > 0),
    "Invalid bootloader RAM memory map!"
);
