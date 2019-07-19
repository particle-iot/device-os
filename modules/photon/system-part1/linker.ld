INCLUDE platform_ram.ld
INCLUDE platform_flash.ld

MEMORY
{
    APP_FLASH (rx)  : ORIGIN = _system_part1_flash_start,      LENGTH = _system_part1_flash_size
    SRAM      (rwx) : ORIGIN = _system_part1_static_ram_start, LENGTH = _system_part1_static_ram_size
}

/* Define the static locations of the part1 module so the dynamic invoke functions have the base address of each library table. */
INCLUDE module_system_part1_export.ld

/* Define the static locations of the part2 module so the dynamic invoke functions have the base address of each library table. */
INCLUDE module_system_part2_export.ld

INCLUDE ../../shared/stm32f2xx/part1.ld
