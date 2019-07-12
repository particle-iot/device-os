/* Memory Spaces Definitions */
INCLUDE platform_ram.ld
INCLUDE platform_flash.ld

MEMORY
{
  INCLUDE backup_ram_memory.ld
  
  RAM (xrw)      : ORIGIN = _bootloader_static_ram_start, LENGTH = _bootloader_static_ram_size
  APP_FLASH (rx) : ORIGIN = _bootloader_flash_start,      LENGTH = _bootloader_flash_size
}

INCLUDE linker_nrf52840_common.ld
