
#include "dynalib.h"
#include "module-system-wifi.h"

DYNALIB_TABLE_EXTERN(communication);
DYNALIB_TABLE_EXTERN(wifi_resource);

void* system_part1_module[] = {
    DYNALIB_TABLE_NAME(communication),
    DYNALIB_TABLE_NAME(wifi_resource),    
};

const void* dynalib_location_communication = &system_part1_module[SYSTEM_WIFI_MODULE_JUMP_TABLE_INDEX_COMMUNICATION];
const void* dynalib_location_wifi_resource = &system_part1_module[SYSTEM_WIFI_MODULE_JUMP_TABLE_INDEX_WIFI_RESOURCE];

