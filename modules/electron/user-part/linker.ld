INCLUDE platform_ram.ld

MEMORY
{
    APP_FLASH (rx) : ORIGIN = 0x08080000, LENGTH = 128K
    SRAM (rwx) : ORIGIN = _user_part_static_ram_start, LENGTH = _user_part_static_ram_size
    INCLUDE backup_ram_memory.ld
}

INCLUDE module_system_part1_export.ld
INCLUDE module_system_part3_export.ld
INCLUDE module_system_part2_export.ld

INCLUDE ../../shared/stm32f2xx/user.ld
