

#include "system_sleep.h"
#include "system_network.h"
#include "system_task.h"
#include "system_cloud.h"
#include "rtc_hal.h"
#include "core_hal.h"
#include "rgbled.h"
#include <stddef.h>

struct WakeupState
{
    bool wifi;
    bool wifiConnected;
    bool cloud;
};

WakeupState wakeupState;

/*******************************************************************************
 * Function Name  : HAL_RTCAlarm_Handler
 * Description    : This function handles additional application requirements.
 * Input          : None.
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
extern "C" void HAL_RTCAlarm_Handler(void)
{
    /* Wake up from System.sleep mode(SLEEP_MODE_WLAN) */
    if (wakeupState.wifiConnected || wakeupState.wifi)  // at present, no way to get the background loop to only turn on wifi.
        PARTICLE_WLAN_SLEEP = 0;
    if (wakeupState.cloud)
        particle_connect();
}


void system_sleep(Particle_Sleep_TypeDef sleepMode, long seconds, uint32_t param, void* reserved)
{
    if (seconds)
        HAL_RTC_Set_UnixAlarm((time_t) seconds);

    switch (sleepMode)
    {
        case SLEEP_MODE_WLAN:
            // save the current state so it can be restored on wakeup
            wakeupState.wifi = !PARTICLE_WLAN_SLEEP;
            wakeupState.cloud = particle_connected();
            wakeupState.wifiConnected = wakeupState.cloud | network_ready(0, 0, NULL) | network_connecting(0, 0, NULL);
            particle_disconnect();
            network_off(0, 0, 0, NULL);
            break;

        case SLEEP_MODE_DEEP:
            HAL_Core_Enter_Standby_Mode();
            break;
    }
}

void system_sleep_pin(uint16_t wakeUpPin, uint16_t edgeTriggerMode, long seconds, uint32_t param, void* reserved)
{
    if (seconds>0)
        HAL_RTC_Set_UnixAlarm((time_t) seconds);

    LED_Off(LED_RGB);
    HAL_Core_Enter_Stop_Mode(wakeUpPin, edgeTriggerMode);
}
