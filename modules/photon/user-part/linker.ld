

MEMORY
{
    APP_FLASH                  (rx)  : ORIGIN = 0x080A0000,     LENGTH = 128K

     /* The SRAM Origin is system_part1_module_ram_end, and extends to
        system_static_ram_start */
    SRAM      (rwx) : ORIGIN = 0x20000300, LENGTH = 0x20000 - 0x300 - 42K
    INCLUDE backup_ram_memory.ld
}

INCLUDE module_system_part1_export.ld
INCLUDE module_system_part2_export.ld

INCLUDE ../../shared/stm32f2xx/user.ld
