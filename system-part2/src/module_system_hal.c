
#include "dynalib.h"
#include "module_system_wifi_init.h"

DYNALIB_TABLE_EXTERN(services);
DYNALIB_TABLE_EXTERN(hal);
DYNALIB_TABLE_EXTERN(rt);

/**
 * The module export table. This lists the addresses of individual library dynalib jump tables.
 */
const void* const system_part2_module[] = {
    DYNALIB_TABLE_NAME(services),
    DYNALIB_TABLE_NAME(hal),
    DYNALIB_TABLE_NAME(rt),
};


__attribute__((section(".module"))) void system_part2_init(); 

/**
 * Global initialization function. Called after memory has been initialized in this module
 * but before C++ constructors are executed.
 */
void system_part2_init() {
    // initialize dependent modules
    module_system_part1_pre_init();
}

