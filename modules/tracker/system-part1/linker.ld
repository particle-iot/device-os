/* Memory spaces definitions */
INCLUDE platform_ram.ld
INCLUDE platform_flash.ld

MEMORY
{
    INCLUDE memory_backup_ram.ld

    APP_FLASH (rx)  : ORIGIN = platform_system_part1_flash_start,      LENGTH = platform_system_part1_flash_size
    SRAM      (rwx) : ORIGIN = platform_system_part1_static_ram_start, LENGTH = platform_system_part1_static_ram_size
}
    
/* Define the static locations of the part1 module so the dynamic invoke functions have the base address of each library table. */
INCLUDE module_system_part1_export.ld

/* Definitions for the static parts of the user module */
INCLUDE module_user_export.ld

INCLUDE linker_system_part1_common.ld