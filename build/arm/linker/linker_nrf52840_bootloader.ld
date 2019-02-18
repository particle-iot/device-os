/* Default stack sizes. These are used by the startup in order to allocate stacks for the
   different modes. */
__Stack_Size = __STACKSIZE__;
__Stack_Init = _estack - __Stack_Size;

/* There will be a link error if there is not this amount of RAM free at the end. */
_Minimum_Stack_Size = 0x1400;

/* Memory Spaces Definitions */

/* INCLUDE module_user_memory.ld */

/* Ensure RAM region doesn't overlap with the system modules, since the bootloader imports
   dynalib functions dynamically on certain platforms */
/* bootloader_ram_length = 16K; */
/* bootloader_ram_offset = 8K; */
/* bootloader_ram_origin = user_module_sram_origin + bootloader_ram_offset; */
/* ASSERT(bootloader_ram_offset + bootloader_ram_length <= user_module_sram_length, */
/*     "Insufficient space for RAM region"); */

INCLUDE softdevice.ld

bootloader_ram_origin = 0x20000000 + APP_RAM_BASE;

bootloader_flash_length = 48K;
bootloader_flash_origin = 0x100000 - bootloader_flash_length;


MEMORY
{
  INCLUDE backup_ram_memory.ld
  RAM (xrw) : ORIGIN = bootloader_ram_origin, LENGTH = 256K - APP_RAM_BASE - LENGTH(BACKUPSRAM_ALL)
  APP_FLASH (rx) : ORIGIN = bootloader_flash_origin, LENGTH = bootloader_flash_length
}

INCLUDE linker_nrf52840_common.ld
