/**
 ******************************************************************************
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************
 */


#include "system_sleep.h"
#include "system_network.h"
#include "system_task.h"
#include "system_cloud.h"
#include "rtc_hal.h"
#include "core_hal.h"
#include "rgbled.h"
#include <stddef.h>
#include "spark_wiring_fuel.h"
#include "spark_wiring_platform.h"

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
        SPARK_WLAN_SLEEP = 0;
#ifndef SPARK_NO_CLOUD
    if (wakeupState.cloud)
        spark_connect();
#endif
}

void sleep_fuel_gauge()
{
    FuelGauge gauge;

    gauge.sleep();
}

void system_sleep(Spark_Sleep_TypeDef sleepMode, long seconds, uint32_t param, void* reserved)
{
    if (seconds)
        HAL_RTC_Set_UnixAlarm((time_t) seconds);

    switch (sleepMode)
    {
        case SLEEP_MODE_WLAN:
            // save the current state so it can be restored on wakeup
            wakeupState.wifi = !SPARK_WLAN_SLEEP;
            wakeupState.wifiConnected = wakeupState.cloud | network_ready(0, 0, NULL) | network_connecting(0, 0, NULL);
#ifndef SPARK_NO_CLOUD
            wakeupState.cloud = spark_connected();
            spark_disconnect();
#endif
            network_off(0, 0, 0, NULL);
            break;

        case SLEEP_MODE_DEEP:
            HAL_Core_Enter_Standby_Mode();
            break;

#if Wiring_SoftPowerOff
        case SLEEP_MODE_SOFTPOWEROFF:
            network_disconnect(0,0,NULL);
            network_off(0, 0, 0, NULL);
            sleep_fuel_gauge();
            HAL_Core_Enter_Standby_Mode();
            break;
#endif
    }
}

void system_sleep_pin(uint16_t wakeUpPin, uint16_t edgeTriggerMode, long seconds, uint32_t param, void* reserved)
{
    if (seconds>0)
        HAL_RTC_Set_UnixAlarm((time_t) seconds);

    LED_Off(LED_RGB);
    HAL_Core_Enter_Stop_Mode(wakeUpPin, edgeTriggerMode);
}
