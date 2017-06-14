MEMORY
{
    APP_FLASH (rx)  : ORIGIN = 0x08020000, LENGTH = 256K

    /* todo - SRAM must start also at an offset after what has been reserved for system-module 1 */
    SRAM      (rwx) : ORIGIN = 0x20000000, LENGTH = 768
}

INCLUDE module_system_part1_export.ld
INCLUDE module_system_part2_export.ld

INCLUDE ../../shared/stm32f2xx/part1.ld
