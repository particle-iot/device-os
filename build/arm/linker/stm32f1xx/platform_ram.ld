/*
Platform RAM memory map for Spark Core (Medium Desity).

Monolithic firmware:                          Bootloader:

+--------------------------------+ 0x20005000 +--------------------------+
| Main stack                     |            | Main stack               |
+--------------------------------+ 0x20004800 +--------------------------+
| Heap                           |            | Heap                     |
+~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+            +~~~~~~~~~~~~~~~~~~~~~~~~~~+
| Monolithic firmware static RAM |            | Bootloader static RAM    |
+--------------------------------+ 0x20000000 +--------------------------+

Note: Keep the diagrams up to date with the actual memory maps defined in this file.
*/

/* Total amount of RAM available on the MCU */
_ram_size  = 20K;
_ram_start = 0x20000000;
_ram_end   = _ram_start + _ram_size;

/* Monolithic firmware main stack
 * _monolithic_firmware_stack_start is the lowest address of the stack. */
_monolithic_firmware_stack_size  = 2K;
_monolithic_firmware_stack_end   = _ram_end;
_monolithic_firmware_stack_start = _monolithic_firmware_stack_end - _monolithic_firmware_stack_size;

/* Bootloader main stack
 * _bootloader_stack_start is the lowest address of the stack. */
_bootloader_stack_size  = 2K;
_bootloader_stack_end   = _ram_end;
_bootloader_stack_start = _bootloader_stack_end - _bootloader_stack_size;

/* Monolithic firmware
 * _monolithic_firmware_static_ram_size is preallocated large enough to fit the monolithic firmware's .data and .bss sections.
 * _monolithic_firmware_static_ram_end is the END of the heap for monolithic firmware.
 * The actual end of the monolithic firmware static ram is calculated after linkage, and it is the actual START of the heap for monolithic firmware. */
_monolithic_firmware_static_ram_size  = _ram_size - _monolithic_firmware_stack_size;
_monolithic_firmware_static_ram_start = _ram_start;
_monolithic_firmware_static_ram_end   = _monolithic_firmware_static_ram_start + _monolithic_firmware_static_ram_size;

/* Bootloader
 * _bootloader_static_ram_size is preallocated large enough to fit the bootloader's .data and .bss sections.
 * _bootloader_static_ram_end is the END of heap for bootloader.
 * The actual end of the bootloader static ram is calculated after linkage, and it is the actual START of the heap for bootloader. */
_bootloader_static_ram_size   = _ram_size - _bootloader_stack_size;
_bootloader_static_ram_start  = _ram_start;
_bootloader_static_ram_end    = _bootloader_static_ram_start + _bootloader_static_ram_size;

/* The heap size after linkage should meet the minimum heap requirement.  */
_heap_min_size = 8K;

/* Basic sanity checks */
ASSERT (
    (_ram_end - _ram_start == _ram_size && _ram_size > 0) &&
    (_monolithic_firmware_stack_size + _monolithic_firmware_static_ram_size == _ram_size) &&
    (_bootloader_stack_size + _bootloader_static_ram_size == _ram_size),
    "Invalid overall RAM memory map!"
);
ASSERT (
    (_monolithic_firmware_stack_end - _monolithic_firmware_stack_start == _monolithic_firmware_stack_size && _monolithic_firmware_stack_size > 0) &&
    (_bootloader_stack_end - _bootloader_stack_start == _bootloader_stack_size && _bootloader_stack_size > 0),
    "Invalid stack memory map!"
);
ASSERT (
    (_monolithic_firmware_static_ram_end - _monolithic_firmware_static_ram_start == _monolithic_firmware_static_ram_size && _monolithic_firmware_static_ram_size > 0),
    "Invalid monolithic firmware RAM memory map!"
);
ASSERT (
    (_bootloader_static_ram_end - _bootloader_static_ram_start == _bootloader_static_ram_size && _bootloader_static_ram_size > 0),
    "Invalid bootloader RAM memory map!"
);
