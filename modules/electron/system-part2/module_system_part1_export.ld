INCLUDE "../../shared/stm32f2xx/part1_vtor_module.ld"

/**
 * The dyanmic libraries exported by this module.
 */
PROVIDE ( dynalib_location_communication = system_part1_module_table + 0 );
PROVIDE ( dynalib_location_services = system_part1_module_table + 4 );
PROVIDE ( dynalib_location_system_module_part1 = system_part1_module_table + 8 );
PROVIDE ( dynalib_location_crypto = system_part1_module_table + 12 );

