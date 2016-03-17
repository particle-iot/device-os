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
#include "system_cloud_internal.h"
#include "system_threading.h"
#include "rtc_hal.h"
#include "core_hal.h"
#include "rgbled.h"
#include <stddef.h>
#include "spark_wiring_fuel.h"
#include "spark_wiring_system.h"
#include "spark_wiring_platform.h"

struct WakeupState
{
    bool wifi;
    bool wifiConnected;
    bool cloud;
};

WakeupState wakeupState;

static void network_suspend() {
    // save the current state so it can be restored on wakeup
    wakeupState.wifi = !SPARK_WLAN_SLEEP;
    wakeupState.wifiConnected = wakeupState.cloud | network_ready(0, 0, NULL) | network_connecting(0, 0, NULL);
#ifndef SPARK_NO_CLOUD
    wakeupState.cloud = spark_connected();
    Spark_Sleep();
    Spark_Disconnect();	// actually disconnect the cloud
    spark_disconnect();	// flag the system to not automatically connect the cloud
#endif
    network_off(0, 0, 0, NULL);
}

static void network_resume() {
	// Set the system flags that triggers the wifi/cloud reconnection in the background loop
    if (wakeupState.wifiConnected || wakeupState.wifi)  // at present, no way to get the background loop to only turn on wifi.
        SPARK_WLAN_SLEEP = 0;
#ifndef SPARK_NO_CLOUD
    if (wakeupState.cloud)
        spark_connect();
#endif
}

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
    network_resume();
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

    network_suspend();

    switch (sleepMode)
    {
        case SLEEP_MODE_WLAN:
            break;

        case SLEEP_MODE_DEEP:
            HAL_Core_Enter_Standby_Mode();
            break;

#if Wiring_SetupButtonUX
        case SLEEP_MODE_SOFTPOWEROFF:
            network_disconnect(0,0,NULL);
            network_off(0, 0, 0, NULL);
            sleep_fuel_gauge();
            HAL_Core_Enter_Standby_Mode();
            break;
#endif
    }
}


int system_sleep_pin_impl(uint16_t wakeUpPin, uint16_t edgeTriggerMode, long seconds, uint32_t param, void* reserved)
{
	SYSTEM_THREAD_CONTEXT_SYNC_CALL(system_sleep_pin_impl(wakeUpPin, edgeTriggerMode, seconds, param, reserved));
    network_suspend();
    LED_Off(LED_RGB);
    HAL_Core_Enter_Stop_Mode(wakeUpPin, edgeTriggerMode, seconds);
    network_resume();		// asynchronously bring up the network/cloud

    // if single-threaded, managed mode then reconnect to the cloud (for up to 30 seconds)
    auto mode = system_mode();
    if (system_thread_get_state(nullptr)==spark::feature::DISABLED && (mode==AUTOMATIC || mode==SEMI_AUTOMATIC) && SPARK_CLOUD_CONNECT) {
    		waitFor(spark_connected, 60000);
    }

    if (spark_connected()) {
    		Spark_Wake();
    }
    return 0;
}

/**
 * Wraps the actual implementation, which has to return a value as part of the threaded implementation.
 */
void system_sleep_pin(uint16_t wakeUpPin, uint16_t edgeTriggerMode, long seconds, uint32_t param, void* reserved)
{
	system_sleep_pin_impl(wakeUpPin, edgeTriggerMode, seconds, param, reserved);
}
