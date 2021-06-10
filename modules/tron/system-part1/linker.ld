INCLUDE platform_ram.ld
INCLUDE ../../shared/rtl872x/common.ld

MEMORY
{
    INCLUDE backup_ram_memory.ld

    /* FIXME: bootloader size should come from a common .ld */
    APP_FLASH (rx)  : ORIGIN = 0x08006000, LENGTH = 1M

    SRAM      (rwx) : ORIGIN = _system_part1_static_ram_start, LENGTH = _system_part1_static_ram_size
}


/* Define the static locations of the part1 module so the dynamic invoke functions have the base address of each library table. */
INCLUDE module_system_part1_export.ld
/* Definitions for the static parts of the user module */
INCLUDE module_user_export.ld

INCLUDE ../../shared/rtl872x/part1.ld
