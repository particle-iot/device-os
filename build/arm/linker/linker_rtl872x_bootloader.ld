/* Default stack sizes. These are used by the startup in order to allocate stacks for the
   different modes. */
__Stack_Size = __STACKSIZE__;
__Stack_Init = 0x10004000;

/* There will be a link error if there is not this amount of RAM free at the end. */
_Minimum_Stack_Size = 0x1000;

/* Memory Spaces Definitions */
INCLUDE platform_ram.ld

bootloader_ram_origin = 0x10005000;

bootloader_flash_length = 64K;
bootloader_flash_origin = 0x08004000;


MEMORY
{
  INCLUDE backup_ram_memory.ld
  RAM (xrw) : ORIGIN = bootloader_ram_origin, LENGTH = 456K + 16K + 4K - (DEFINED(GCC_LD_BROKEN) ? 4K : LENGTH(BACKUPSRAM_ALL))
  APP_FLASH (rx) : ORIGIN = bootloader_flash_origin, LENGTH = bootloader_flash_length
}

INCLUDE linker_rtl872x_common.ld
INCLUDE linker_rtl872x_rom.ld
