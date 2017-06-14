#include "module_system_part1.h"
#include "system_part1_loader.c"

DYNALIB_TABLE_EXTERN(communication);
DYNALIB_TABLE_EXTERN(services);
DYNALIB_TABLE_EXTERN(system_module_part1);
DYNALIB_TABLE_EXTERN(crypto);

__attribute__((externally_visible)) const void* const system_part1_module[] = {
    DYNALIB_TABLE_NAME(communication),
    DYNALIB_TABLE_NAME(services),
    DYNALIB_TABLE_NAME(system_module_part1),
    DYNALIB_TABLE_NAME(crypto),
};

