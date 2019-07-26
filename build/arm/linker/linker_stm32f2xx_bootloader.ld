/* Default stack sizes. These are used by the startup in order to allocate stacks for the
   different modes. */
__Stack_Size = 2048;
__Stack_Init = _estack - __Stack_Size;

/* There will be a link error if there is not this amount of RAM free at the end. */
_Minimum_Stack_Size = 0x1400;

/* Memory Spaces Definitions */
INCLUDE platform_ram.ld
INCLUDE platform_flash.ld

MEMORY
{
    RAM        (xrw) : ORIGIN = _bootloader_static_ram_start, LENGTH = _bootloader_static_ram_size
    APP_FLASH  (rx)  : ORIGIN = _bootloader_flash_start,      LENGTH = _bootloader_flash_size
    DCT1_FLASH (rx)  : ORIGIN = _dct1_flash_start,            LENGTH = _dct1_flash_size
    DCT2_FLASH (rx)  : ORIGIN = _dct2_flash_start,            LENGTH = _dct2_flash_size
  
  INCLUDE memory_backup_ram.ld
}

INCLUDE linker_stm32f2xx_common.ld
