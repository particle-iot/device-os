
#include "core_hal.h"
#include "rtc_hal.h"
#include "rgbled.h"
#include "spark_wiring_cloud.h"
#include "system_task.h"
#include "system_network.h"

SystemClass System;

void SystemClass::factoryReset(void)
{
    //This method will work only if the Core is supplied
    //with the latest version of Bootloader
    HAL_Core_Factory_Reset();
}

void SystemClass::bootloader(void)
{
    //DFU mode persist if firmware upgrade is not completed
    HAL_Core_Enter_Bootloader(true);
}

void SystemClass::modeDFU(void)
{
    //Briefly enter DFU bootloader mode (works with latest bootloader only )
    //Subsequent reset or power off-on will execute normal firmware
    HAL_Core_Enter_Bootloader(false);
}

void SystemClass::reset(void)
{
    HAL_Core_System_Reset();
}

void SystemClass::sleep(Spark_Sleep_TypeDef sleepMode, long seconds)
{
    HAL_RTC_Set_UnixAlarm((time_t) seconds);

    switch (sleepMode)
    {
        case SLEEP_MODE_WLAN:
            network_off(false);
            break;

        case SLEEP_MODE_DEEP:
            HAL_Core_Enter_Standby_Mode();
            break;
    }
}

void SystemClass::sleep(uint16_t wakeUpPin, uint16_t edgeTriggerMode, long seconds)
{
    if (seconds>0)
        HAL_RTC_Set_UnixAlarm((time_t) seconds);

    LED_Off(LED_RGB);
    HAL_Core_Enter_Stop_Mode(wakeUpPin, edgeTriggerMode);
}
