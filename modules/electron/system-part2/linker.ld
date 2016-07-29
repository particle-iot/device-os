MEMORY
{
    APP_FLASH (rx)  : ORIGIN = 0x08020000, LENGTH = 128K

    SRAM      (rwx) : ORIGIN = 0x20000000, LENGTH = 1024
}

INCLUDE module_system_part1_export.ld
INCLUDE module_system_part3_export.ld
INCLUDE module_system_part2_export.ld

INCLUDE ../../shared/stm32f2xx/part1.ld