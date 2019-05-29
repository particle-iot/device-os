INCLUDE ../../shared/stm32f2xx/photon/platform_ram.ld
INCLUDE module_user_memory.ld

MEMORY
{
    APP_FLASH (rx) : ORIGIN = user_module_app_flash_origin, LENGTH = user_module_app_flash_length
    SRAM (rwx) : ORIGIN = _user_static_ram_start, LENGTH = _user_static_ram_size
    INCLUDE backup_ram_memory.ld
}

INCLUDE module_system_part1_export.ld
INCLUDE module_system_part2_export.ld

INCLUDE ../../shared/stm32f2xx/user.ld
