
#include "dynalib.h"
#include "module_system_wifi.h"

DYNALIB_TABLE_EXTERN(communication);
DYNALIB_TABLE_EXTERN(wifi_resource);

const void* const system_part1_module[] = {
    DYNALIB_TABLE_NAME(communication),
    DYNALIB_TABLE_NAME(wifi_resource),    
};

