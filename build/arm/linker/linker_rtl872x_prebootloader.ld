/* Default stack sizes. These are used by the startup in order to allocate stacks for the
   different modes. */
__Stack_Size = __STACKSIZE__;
__Stack_Init = 0x0008F000;

/* There will be a link error if there is not this amount of RAM free at the end. */
_Minimum_Stack_Size = 0x1000;

/* Memory Spaces Definitions */
INCLUDE platform_ram_m23.ld

prebootloader_ram_origin = 0x00082000;

prebootloader_flash_length = 8K;
prebootloader_flash_origin = 0x08000000;


MEMORY
{
  INCLUDE backup_ram_memory.ld
  RAM (xrw) : ORIGIN = prebootloader_ram_origin, LENGTH = 52K + __STACKSIZE__
  APP_FLASH (rx) : ORIGIN = prebootloader_flash_origin, LENGTH = prebootloader_flash_length

}

INCLUDE linker_rtl872x_common.ld
INCLUDE linker_rtl872x_rom_m23.ld

ASSERT ( link_rtl_header_location_end - link_rtl_header_location == 0x20, "rtl872x header is invalid" );
