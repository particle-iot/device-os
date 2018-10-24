INCLUDE softdevice.ld
INCLUDE ../../shared/nRF52840/common.ld

INCLUDE module_user_memory.ld

MEMORY
{
    INCLUDE backup_ram_memory.ld
    APP_FLASH (rx) : ORIGIN = user_module_app_flash_origin, LENGTH = user_module_app_flash_length
    SRAM (rwx) : ORIGIN = 0x20040000 - LENGTH(BACKUPSRAM_ALL) - __Stack_Size - user_module_sram_length, LENGTH = user_module_sram_length
}

INCLUDE module_system_part1_export.ld

INCLUDE ../../shared/nRF52840/module_user_export.ld

INCLUDE ../../shared/nRF52840/user.ld
