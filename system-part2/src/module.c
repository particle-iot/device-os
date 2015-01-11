

/**
 * todo - copy initialized data to RAM and zero out uninitialized region
 * 
 */

#include "dynalib.h"

DYNALIB_TABLE_EXTERN(services);

void* system_part2_module[] = {
    DYNALIB_TABLE_NAME(services)    
};

void app_setup_and_loop() {
    
}

