
/**
 * The load address of the part2 module image
 */
system_part2_start = 0x8060000;

/**
 * A fake VTOR table so that the bootloader can jump to this module and set up the stack,
 * plus that the module_info resides at the correct offset.
*/
system_part2_vtor_table_size = 0x184;

system_part2_module_info_size = 24;

system_part2_vtor_table_start = system_part2_start;

system_part2_module_info_start = system_part2_start + system_part2_vtor_table_size;

system_part2_module_info_end = system_part2_module_info_start + system_part2_module_info_size;

/**
 * The location of the module export table.
 */
system_part2_module_table = system_part2_module_info_end;


system_part2_reset_handler_location = system_part2_vtor_table_start + 4;

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