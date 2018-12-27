
INCLUDE softdevice.ld
INCLUDE ../../shared/nRF52840/common.ld

MEMORY
{
    INCLUDE backup_ram_memory.ld

    /* FIXME: bootloader size should come from a common .ld */
    APP_FLASH (rx)  : ORIGIN = APP_CODE_BASE, LENGTH = 1M - APP_CODE_BASE - 128K - 48K

    SRAM      (rwx) : ORIGIN = 0x20000000 + APP_RAM_BASE, LENGTH = 104K
}


/* Define the static locations of the part1 module so the dynamic invoke functions have the base address of each library table. */
INCLUDE module_system_part1_export.ld
/* Definitions for the static parts of the user module */
INCLUDE module_user_export.ld

INCLUDE ../../shared/nRF52840/part1.ld

ASSERT ( ( system_part1_static_ram_size == 104K ), "please update the static ram size in the memory regions above" );
