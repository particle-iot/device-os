/* Memory spaces definitions */
INCLUDE platform_ram.ld
INCLUDE platform_flash.ld

MEMORY
{
    APP_FLASH (rx)  : ORIGIN = platform_user_part_flash_start,      LENGTH = platform_user_part_flash_size
    SRAM      (rwx) : ORIGIN = platform_user_part_static_ram_start, LENGTH = platform_user_part_static_ram_size
    
    INCLUDE memory_backup_ram.ld
}

/* Define the static locations of the part1 module so the dynamic invoke functions have the base address of each library table. */
INCLUDE module_system_part1_export.ld

/* Define the static locations of the part2 module so the dynamic invoke functions have the base address of each library table. */
INCLUDE module_system_part2_export.ld

INCLUDE linker_user_part_common.ld
