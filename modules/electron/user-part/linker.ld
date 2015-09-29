

MEMORY
{
    APP_FLASH                  (rx)  : ORIGIN = 0x08080000,     LENGTH = 128K

     /* The SRAM Origin is system_part1_module_ram_end, and extends to
        system_static_ram_start */
    SRAM      (rwx) : ORIGIN = 0x20000400, LENGTH = 0x20000 - 0x400 - 40K
    BACKUPSRAM (rwx) : ORIGIN = 0x40024000, LENGTH = 4K
}

INCLUDE module_system_part1_export.ld
INCLUDE module_system_part2_export.ld

INCLUDE ../../shared/stm32f2xx/user.ld