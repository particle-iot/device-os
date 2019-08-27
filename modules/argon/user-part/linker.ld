/* This file is generated at build time (see build_linker_script.mk) */
INCLUDE platform_user_ram.ld

INCLUDE platform_ram.ld
INCLUDE softdevice.ld
INCLUDE ../../shared/nRF52840/common.ld

MEMORY
{
    INCLUDE backup_ram_memory.ld
    APP_FLASH (rx) : ORIGIN = 0x000d4000, LENGTH = 128K
    SRAM (rwx) : ORIGIN = _user_part_static_ram_start, LENGTH = _user_part_static_ram_size
}

INCLUDE module_system_part1_export.ld

INCLUDE ../../shared/nRF52840/module_user_export.ld

INCLUDE ../../shared/nRF52840/user.ld
