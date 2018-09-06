
__Stack_Size = 2048 ;

PROVIDE ( _Stack_Size = __Stack_Size ) ;

__Stack_Init = _estack - __Stack_Size ;

/* "PROVIDE" allows to easily override these values from an object file or the commmand line.
 */
PROVIDE ( _Stack_Init = __Stack_Init ) ;

/*
There will be a link error if there is not this amount of RAM free at the end.
*/
_Minimum_Stack_Size = 0x1400;

/* include the memory spaces definitions sub-script */
/* Linker subscript for STM32F20x definitions with 1024K Flash and 128K RAM */

/* Memory Spaces Definitions */

/* higher address of the user mode stack */
_estack = 0x20000000 + 200K;


system_part1_ram_start = 0x20000000 + 16K;
system_part1_ram_size = 100K;


system_part1_start = 0x26000;
system_part1_vtor_table_size = 0x200;
system_part1_module_info_size = 24;
system_part1_module_info_start = system_part1_start + system_part1_vtor_table_size;
system_part1_module_info_end = system_part1_module_info_start + system_part1_module_info_size;

/**
 * The location of the module export table.
 */
system_part1_module_table = system_part1_module_info_end;


/** 32K just a guess */
min_heap_size = 32K;

system_heap_end = __Stack_Init;

PROVIDE ( dynalib_location_hal = system_part1_module_table + 0 );
PROVIDE ( dynalib_location_rt = system_part1_module_table + 4 );
PROVIDE ( dynalib_location_system = system_part1_module_table + 8 );
PROVIDE ( dynalib_location_hal_peripherals = system_part1_module_table + 12 );
PROVIDE ( dynalib_location_hal_i2c = system_part1_module_table + 16 );
PROVIDE ( dynalib_location_hal_gpio = system_part1_module_table + 20 );
PROVIDE ( dynalib_location_hal_spi = system_part1_module_table + 24 );
PROVIDE ( dynalib_location_hal_core = system_part1_module_table + 28 );
PROVIDE ( dynalib_location_hal_socket = system_part1_module_table + 32 );
PROVIDE ( dynalib_location_hal_usart = system_part1_module_table + 36 );
PROVIDE ( dynalib_location_system_net = system_part1_module_table + 40 );
PROVIDE ( dynalib_location_system_cloud = system_part1_module_table + 44 );
PROVIDE ( dynalib_location_hal_concurrent = system_part1_module_table + 48 );
PROVIDE ( dynalib_location_hal_usb = system_part1_module_table + 52 );
PROVIDE ( dynalib_location_hal_rgbled = system_part1_module_table + 56 );
PROVIDE ( dynalib_location_hal_bootloader = system_part1_module_table + 60 );
PROVIDE ( dynalib_location_hal_dct = system_part1_module_table + 64 );
PROVIDE ( dynalib_location_communication = system_part1_module_table + 68 );
PROVIDE ( dynalib_location_services = system_part1_module_table + 72 );
