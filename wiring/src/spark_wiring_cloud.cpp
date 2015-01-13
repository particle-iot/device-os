
#include "spark_wiring_cloud.h"
#include "rtc_hal.h"
#include "core_hal.h"
#include "system_task.h"

void SparkClass::sleep(Spark_Sleep_TypeDef sleepMode, long seconds)
{
    HAL_RTC_Set_UnixAlarm((time_t)seconds);

    switch(sleepMode)
    {
        case SLEEP_MODE_WLAN:
            SPARK_WLAN_SLEEP = 1;//Flag Wifi.off() not to disable cloud connection
            network_off();
            break;

        case SLEEP_MODE_DEEP:
            HAL_Core_Enter_Standby_Mode();
            break;
    }
}

void SparkClass::sleep(long seconds)
{
    SparkClass::sleep(SLEEP_MODE_WLAN, seconds);
}

void SparkClass::sleep(uint16_t wakeUpPin, uint16_t edgeTriggerMode)
{
    HAL_Core_Enter_Stop_Mode(wakeUpPin, edgeTriggerMode);
}

void SparkClass::sleep(uint16_t wakeUpPin, uint16_t edgeTriggerMode, long seconds)
{
    HAL_RTC_Set_UnixAlarm((time_t)seconds);

    sleep(wakeUpPin, edgeTriggerMode);
}

