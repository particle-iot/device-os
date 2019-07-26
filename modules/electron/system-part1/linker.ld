INCLUDE platform_ram.ld
INCLUDE platform_flash.ld

MEMORY
{
    APP_FLASH (rx)  : ORIGIN = _system_part1_flash_start,      LENGTH = _system_part1_flash_size
    SRAM      (rwx) : ORIGIN = _system_part1_static_ram_start, LENGTH = _system_part1_static_ram_size
}

INCLUDE module_system_part1_export.ld
INCLUDE module_system_part3_export.ld
INCLUDE module_system_part2_export.ld

INCLUDE linker_system_part1_common.ld

ASSERT(
    _system_part1_static_ram_start == _system_part3_static_ram_end,
    "System part 1 static RAM should come immediately after system part 3 static RAM"
);
