/* Default stack sizes. These are used by the startup in order to allocate stacks for the
   different modes. */
__Stack_Size = 2048;
__Stack_Init = _estack - __Stack_Size;

/* There will be a link error if there is not this amount of RAM free at the end. */
_Minimum_Stack_Size = 0x1400;

/* Memory Spaces Definitions */

INCLUDE module_user_memory.ld

/* Ensure RAM region doesn't overlap with the system modules, since the bootloader imports
   dynalib functions dynamically on certain platforms */
bootloader_ram_length = 16K;
bootloader_ram_offset = 8K;
bootloader_ram_origin = user_module_sram_origin + bootloader_ram_offset;
ASSERT(bootloader_ram_offset + bootloader_ram_length <= user_module_sram_length,
    "Insufficient space for RAM region");

MEMORY
{
  RAM (xrw) : ORIGIN = bootloader_ram_origin, LENGTH = bootloader_ram_length
  APP_FLASH (rx) : ORIGIN = 0x08000000, LENGTH = 16K
  DCT1_FLASH (rx) : ORIGIN = 0x08004000, LENGTH = 16K
  DCT2_FLASH (rx) : ORIGIN = 0x08008000, LENGTH = 16K
  INCLUDE backup_ram_memory.ld
}

INCLUDE linker_stm32f2xx_common.ld
