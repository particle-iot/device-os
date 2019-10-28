/*
Platform RAM memory map for Spark Core.

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
platform_ram_size  = 20K;
platform_ram_start = 0x20000000;
platform_ram_end   = platform_ram_start + platform_ram_size;


/* Monolithic firmware main stack
 * platform_monolithic_firmware_stack_start is the lowest address of the stack. */
platform_monolithic_firmware_stack_size  = 2K;
platform_monolithic_firmware_stack_end   = platform_ram_end;
platform_monolithic_firmware_stack_start = platform_monolithic_firmware_stack_end - platform_monolithic_firmware_stack_size;

/* Bootloader main stack
 * platform_bootloader_stack_start is the lowest address of the stack. */
platform_bootloader_stack_size  = 2K;
platform_bootloader_stack_end   = platform_ram_end;
platform_bootloader_stack_start = platform_bootloader_stack_end - platform_bootloader_stack_size;


/* Monolithic firmware
 * platform_monolithic_firmware_static_ram_size is preallocated large enough to fit the monolithic firmware's .data and .bss sections.
 * platform_monolithic_firmware_static_ram_end is the END of the heap for monolithic firmware.
 * The actual end of the monolithic firmware static ram is calculated after linkage, and it is the actual START of the heap for monolithic firmware. */
platform_monolithic_firmware_static_ram_size  = platform_ram_size - platform_monolithic_firmware_stack_size;
platform_monolithic_firmware_static_ram_start = platform_ram_start;
platform_monolithic_firmware_static_ram_end   = platform_monolithic_firmware_static_ram_start + platform_monolithic_firmware_static_ram_size;


/* Bootloader
 * platform_bootloader_static_ram_size is preallocated large enough to fit the bootloader's .data and .bss sections.
 * platform_bootloader_static_ram_end is the END of heap for bootloader.
 * The actual end of the bootloader static ram is calculated after linkage, and it is the actual START of the heap for bootloader. */
platform_bootloader_static_ram_size   = platform_ram_size - platform_bootloader_stack_size;
platform_bootloader_static_ram_start  = platform_ram_start;
platform_bootloader_static_ram_end    = platform_bootloader_static_ram_start + platform_bootloader_static_ram_size;


/* The heap size after linkage should meet the minimum heap requirement.  */
platform_heap_min_size = 8K;


/* Basic sanity checks */
ASSERT (
    (platform_ram_end - platform_ram_start == platform_ram_size && platform_ram_size > 0) &&
    (platform_monolithic_firmware_stack_size + platform_monolithic_firmware_static_ram_size == platform_ram_size) &&
    (platform_bootloader_stack_size + platform_bootloader_static_ram_size == platform_ram_size),
    "Invalid overall RAM memory map!"
);
ASSERT (
    (platform_monolithic_firmware_stack_end - platform_monolithic_firmware_stack_start == platform_monolithic_firmware_stack_size && platform_monolithic_firmware_stack_size > 0) &&
    (platform_bootloader_stack_end - platform_bootloader_stack_start == platform_bootloader_stack_size && platform_bootloader_stack_size > 0),
    "Invalid stack memory map!"
);
ASSERT (
    (platform_monolithic_firmware_static_ram_end - platform_monolithic_firmware_static_ram_start == platform_monolithic_firmware_static_ram_size && platform_monolithic_firmware_static_ram_size > 0),
    "Invalid monolithic firmware RAM memory map!"
);
ASSERT (
    (platform_bootloader_static_ram_end - platform_bootloader_static_ram_start == platform_bootloader_static_ram_size && platform_bootloader_static_ram_size > 0),
    "Invalid bootloader RAM memory map!"
);
