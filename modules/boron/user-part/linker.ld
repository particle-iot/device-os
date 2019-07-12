/* This file is generated at build time (see build_linker_script.mk) */
INCLUDE platform_user_ram.ld

INCLUDE platform_ram.ld
INCLUDE platform_flash.ld

MEMORY
{
    INCLUDE backup_ram_memory.ld
    
    APP_FLASH (rx)  : ORIGIN = _user_part_flash_start,      LENGTH = _user_part_flash_size
    SRAM      (rwx) : ORIGIN = _user_part_static_ram_start, LENGTH = _user_part_static_ram_size
}

INCLUDE linker_user_part_common.ld
