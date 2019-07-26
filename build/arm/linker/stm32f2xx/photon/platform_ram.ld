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
_ram_size  = 128K;
_ram_start = 0x20000000;
_ram_end   = _ram_start + _ram_size;

/* Independent backup RAM */
_backup_ram_size  = 4K;
_backup_ram_start = 0x40024000;
_backup_ram_end   = _backup_ram_start + _backup_ram_size;

/* System backup RAM */
_system_backup_ram_size  = 1K;
_system_backup_ram_end   = _backup_ram_end;
_system_backup_ram_start = _system_backup_ram_end - _system_backup_ram_size;

/* User backup RAM */
_user_backup_ram_size  = 3K;
_user_backup_ram_end   = _system_backup_ram_start;
_user_backup_ram_start = _user_backup_ram_end - _user_backup_ram_size;

/* Modular firmware main stack
 * _modular_firmware_stack_start is the lowest address of the stack. */
_modular_firmware_stack_size  = 4K;
_modular_firmware_stack_end   = _ram_end;
_modular_firmware_stack_start = _modular_firmware_stack_end - _modular_firmware_stack_size;

/* Monolithic firmware main stack
 * _monolithic_firmware_stack_start is the lowest address of the stack. */
_monolithic_firmware_stack_size  = 4K;
_monolithic_firmware_stack_end   = _ram_end;
_monolithic_firmware_stack_start = _monolithic_firmware_stack_end - _monolithic_firmware_stack_size;

/* Bootloader main stack
 * _bootloader_stack_start is the lowest address of the stack. */
_bootloader_stack_size  = 4K;
_bootloader_stack_end   = _ram_end;
_bootloader_stack_start = _bootloader_stack_end - _bootloader_stack_size;

/* System part 2
 * _system_part2_static_ram_size is preallocated appropriately to *JUST* fit the system part2's .data and .bss sections. */
_system_part2_static_ram_size  = 42K;
_system_part2_static_ram_end   = _modular_firmware_stack_start;
_system_part2_static_ram_start = _system_part2_static_ram_end - _system_part2_static_ram_size;

/* System part 1
 * _system_part1_static_ram_size is preallocated appropriately to *JUST* fit the system part1's .data and .bss sections.
 * _system_part1_static_ram_start is the END of heap for modular firmware. */
_system_part1_static_ram_size  = 1K;
_system_part1_static_ram_end   = _system_part2_static_ram_start;
_system_part1_static_ram_start = _system_part1_static_ram_end - _system_part1_static_ram_size;

/* User part
 * _user_part_static_ram_size is preallocated large enough to fit the user part's .data and .bss sections.
 * The actual end of the user part static ram is calculated after linkage.
 * _user_part_static_ram_end is passed to system part 2 at startup for calculating the actual START of the heap for modular firmware.*/
_user_part_static_ram_size  = _ram_size - _modular_firmware_stack_size - _system_part2_static_ram_size - _system_part1_static_ram_size;
_user_part_static_ram_start = _ram_start;
_user_part_static_ram_end   = _ram_start + _user_part_static_ram_size;

/* Monolithic firmware
 * _monolithic_firmware_static_ram_size is preallocated large enough to fit the monolithic firmware's .data and .bss sections.
 * _monolithic_firmware_static_ram_end is the END of the heap for monolithic firmware.
 * The actual end of the monolithic firmware static ram is calculated after linkage, and it is the actual START of the heap for monolithic firmware. */
_monolithic_firmware_static_ram_size  = _ram_size - _monolithic_firmware_stack_size;
_monolithic_firmware_static_ram_start = _ram_start;
_monolithic_firmware_static_ram_end   = _monolithic_firmware_static_ram_start + _monolithic_firmware_static_ram_size;

/* Bootloader
 * FIXME: _bootloader_static_ram_offset for backwards compatibility?
 * Ensure RAM region doesn't overlap with the system modules, since the bootloader imports dynalib functions dynamically on certain platforms.
 * _bootloader_static_ram_size is preallocated large enough to fit the bootloader's .data and .bss sections.
 * _bootloader_static_ram_end is the END of heap for bootloader.
 * The actual end of the bootloader static ram is calculated after linkage, and it is the actual START of the heap for bootloader. */
_bootloader_static_ram_offset = 8K;
_bootloader_static_ram_size   = _ram_size - _bootloader_stack_size - _system_part2_static_ram_size - _system_part1_static_ram_size - _bootloader_static_ram_offset;
_bootloader_static_ram_start  = _ram_start + _bootloader_static_ram_offset;
_bootloader_static_ram_end    = _bootloader_static_ram_start + _bootloader_static_ram_size;

/* The heap size after linkage should meet the minimum heap requirement.  */
_heap_min_size = 16K;

/* Basic sanity checks */
ASSERT (
    (_ram_end - _ram_start == _ram_size && _ram_size > 0) &&
    (_modular_firmware_stack_size + _system_part2_static_ram_size + _system_part1_static_ram_size + _user_part_static_ram_size == _ram_size) &&
    (_monolithic_firmware_stack_size + _monolithic_firmware_static_ram_size == _ram_size) &&
    (_bootloader_stack_size + _system_part2_static_ram_size + _system_part1_static_ram_size + _bootloader_static_ram_size + _bootloader_static_ram_offset == _ram_size),
    "Invalid overall RAM memory map!"
);
ASSERT (
    (_system_backup_ram_size + _user_backup_ram_size == _backup_ram_size) &&
    (_backup_ram_end - _backup_ram_start == _backup_ram_size && _backup_ram_size > 0) &&
    (_system_backup_ram_end - _system_backup_ram_start == _system_backup_ram_size && _system_backup_ram_size > 0) &&
    (_user_backup_ram_end - _user_backup_ram_start == _user_backup_ram_size && _user_backup_ram_size > 0),
    "Invalid backup RAM memory map!"
);
ASSERT (
    (_modular_firmware_stack_end - _modular_firmware_stack_start == _modular_firmware_stack_size && _modular_firmware_stack_size > 0) &&
    (_monolithic_firmware_stack_end - _monolithic_firmware_stack_start == _monolithic_firmware_stack_size && _monolithic_firmware_stack_size > 0) &&
    (_bootloader_stack_end - _bootloader_stack_start == _bootloader_stack_size && _bootloader_stack_size > 0),
    "Invalid stack memory map!"
);
ASSERT (
    (_system_part2_static_ram_end - _system_part2_static_ram_start == _system_part2_static_ram_size && _system_part2_static_ram_size > 0),
    "Invalid system part 2 RAM memory map!"
);
ASSERT (
    (_system_part1_static_ram_end - _system_part1_static_ram_start == _system_part1_static_ram_size && _system_part1_static_ram_size > 0),
    "Invalid system part 1 RAM memory map!"
);
ASSERT (
    (_user_part_static_ram_end - _user_part_static_ram_start == _user_part_static_ram_size && _user_part_static_ram_size > 0),
    "Invalid user part RAM memory map!"
);
ASSERT (
    (_monolithic_firmware_static_ram_end - _monolithic_firmware_static_ram_start == _monolithic_firmware_static_ram_size && _monolithic_firmware_static_ram_size > 0),
    "Invalid monolithic firmware RAM memory map!"
);
ASSERT (
    (_bootloader_static_ram_end - _bootloader_static_ram_start == _bootloader_static_ram_size && _bootloader_static_ram_size > 0),
    "Invalid bootloader RAM memory map!"
);
