INCLUDE ram_map.ld

MEMORY
{
    APP_FLASH (rx)  : ORIGIN = 0x08060000, LENGTH = 128K
    SRAM      (rwx) : ORIGIN = _system_part3_static_ram_start, LENGTH = _system_part3_static_ram_size
}

INCLUDE module_system_part1_export.ld
INCLUDE module_system_part2_export.ld
INCLUDE module_system_part3_export.ld

INCLUDE ../../shared/stm32f2xx/part3.ld
