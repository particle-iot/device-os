#include "syshealth_hal.h"

#include "hw_config.h"

eSystemHealth HAL_Get_Sys_Health() {
    return RTC_ReadBackupRegister(RTC_BKP_DR1);
}

void HAL_Set_Sys_Health(eSystemHealth health) {
    static eSystemHealth cache;
    if (health>cache) {
        cache = health;
        RTC_WriteBackupRegister(RTC_BKP_DR1, (health));
    }
}

