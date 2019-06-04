INCLUDE ram_map.ld

MEMORY
{
    APP_FLASH (rx)  : ORIGIN = 0x08020000, LENGTH = 256K
    SRAM      (rwx) : ORIGIN = _system_part1_static_ram_start, LENGTH = _system_part1_static_ram_size
}

INCLUDE module_system_part1_export.ld
INCLUDE module_system_part2_export.ld

INCLUDE ../../shared/stm32f2xx/part1.ld
