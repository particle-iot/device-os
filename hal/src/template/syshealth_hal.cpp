
#include "syshealth_hal.h"

eSystemHealth sys_health_cache;

void HAL_Set_Sys_Health(eSystemHealth health) {
    if (health>sys_health_cache)
    {
        sys_health_cache = health;
    }
}

eSystemHealth HAL_Get_Sys_Health() {
    return sys_health_cache;
}



