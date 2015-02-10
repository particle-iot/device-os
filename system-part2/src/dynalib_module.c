

/**
 * todo - copy initialized data to RAM and zero out uninitialized region
 * 
 */

#include "dynalib.h"


DYNALIB_TABLE_EXTERN(services);
DYNALIB_TABLE_EXTERN(hal);

/**
 * The module export table. This lists the addresses of individual library dynalib jump tables.
 */
void* system_part2_module[] = {
    DYNALIB_TABLE_NAME(services),
    DYNALIB_TABLE_NAME(hal)
};

