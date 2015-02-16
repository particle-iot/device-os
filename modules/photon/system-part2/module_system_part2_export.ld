
system_part2_module_table = 0x8020190;

stack_size = 1400;


/**
 * Stack end at top of SRAM
 */
stack_end = 0x20020000;
stack_start = ALIGN(stack_end - stack_size, 4);
/** 8K just a guess at how much WICED might need */
min_heap_size = 8K;

system_static_ram_end = ORIGIN(SRAM) + 64K;

PROVIDE ( dynalib_location_hal = system_part2_module_table + 0 );
PROVIDE ( dynalib_location_rt = system_part2_module_table + 4 );
PROVIDE ( dynalib_location_system = system_part2_module_table + 8 );
PROVIDE ( dynalib_location_hal_peripherals = system_part2_module_table + 12 );