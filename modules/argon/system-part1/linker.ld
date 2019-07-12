INCLUDE platform_ram.ld
INCLUDE platform_flash.ld

MEMORY
{
    INCLUDE backup_ram_memory.ld

    APP_FLASH (rx)  : ORIGIN = _system_part1_flash_start,      LENGTH = _system_part1_flash_size
    SRAM      (rwx) : ORIGIN = _system_part1_static_ram_start, LENGTH = _system_part1_static_ram_size
}

INCLUDE linker_system_part1_common.ld
