
#include "syshealth_hal.h"

#include "hw_config.h"

eSystemHealth sys_health_cache;

void HAL_Set_Sys_Health(eSystemHealth health) {
    if (health>sys_health_cache)
    {
        sys_health_cache = health;
        BKP_WriteBackupRegister(BKP_DR1, (health));
    }
}

eSystemHealth HAL_Get_Sys_Health() {
    return BKP_ReadBackupRegister(BKP_DR1);
}


