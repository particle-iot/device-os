
#include "dynalib.h"
#include "module_system_wifi_init.h"
#include <stdint.h>

DYNALIB_TABLE_EXTERN(services);
DYNALIB_TABLE_EXTERN(hal);
DYNALIB_TABLE_EXTERN(rt);
DYNALIB_TABLE_EXTERN(system);

/**
 * The module export table. This lists the addresses of individual library dynalib jump tables.
 */
const void* const system_part2_module[] = {
    DYNALIB_TABLE_NAME(services),
    DYNALIB_TABLE_NAME(hal),
    DYNALIB_TABLE_NAME(rt),
    DYNALIB_TABLE_NAME(system),
};

/**
 * Determines if the user module is present and valid.
 * @return 
 */
uint8_t is_user_module_valid()
{
    return 0;
}

/**
 * Global initialization function. Called after memory has been initialized in this module
 * but before C++ constructors are executed.
 */
void system_part2_pre_init() {
    // initialize dependent modules
    module_system_part1_pre_init();
    
    // check if the user module is valid and if so, call the init function there
    if (is_user_module_valid()) {
        
    }
}

__attribute__((section(".module_pre_init"))) const void* system_part2_pre_init_fn = system_part2_pre_init;