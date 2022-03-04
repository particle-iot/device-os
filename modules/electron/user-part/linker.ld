/* Memory spaces definitions */
INCLUDE platform_ram.ld
INCLUDE platform_flash.ld

MEMORY
{
    APP_FLASH (rx)  : ORIGIN = platform_user_part_flash_start,      LENGTH = platform_user_part_flash_size
    SRAM      (rwx) : ORIGIN = platform_user_part_static_ram_start, LENGTH = platform_user_part_static_ram_size
    
    INCLUDE memory_backup_ram.ld
}

INCLUDE module_system_part1_export.ld
INCLUDE module_system_part3_export.ld
INCLUDE module_system_part2_export.ld

INCLUDE linker_user_part_common.ld
