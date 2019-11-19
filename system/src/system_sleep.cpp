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
#include "system_network_internal.h"
#include "system_threading.h"
#include "rtc_hal.h"
#include "core_hal.h"
#include "led_service.h"
#include <stddef.h>
#include "spark_wiring_fuel.h"
#include "spark_wiring_system.h"
#include "spark_wiring_platform.h"
#include "system_power.h"
#include "check.h"

#if PLATFORM_ID==PLATFORM_ELECTRON_PRODUCTION
# include "parser.h"
#endif


struct WakeupState
{
    bool wifi;
    bool wifiConnected;
    bool cloud;
};

WakeupState wakeupState;

static void network_suspend() {
    // save the current state so it can be restored on wakeup
#ifndef SPARK_NO_CLOUD
    wakeupState.cloud = spark_cloud_flag_auto_connect();
#endif
    wakeupState.wifi = !SPARK_WLAN_SLEEP;
    wakeupState.wifiConnected = wakeupState.cloud || network_ready(0, 0, NULL) || network_connecting(0, 0, NULL);
    // Disconnect the cloud and the network
    network_disconnect(0, NETWORK_DISCONNECT_REASON_SLEEP, NULL);
#ifndef SPARK_NO_CLOUD
    // Clear the auto connect status
    spark_cloud_flag_disconnect();
#endif
    network_off(0, 0, 0, NULL);
}

static void network_resume() {
    // Set the system flags that triggers the wifi/cloud reconnection in the background loop
    if (wakeupState.wifiConnected || wakeupState.wifi)  // at present, no way to get the background loop to only turn on wifi.
        SPARK_WLAN_SLEEP = 0;
    if (wakeupState.cloud)
        spark_cloud_flag_connect();
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

bool network_sleep_flag(uint32_t flags)
{
    return (flags & SYSTEM_SLEEP_FLAG_NETWORK_STANDBY) == 0;
}

int system_sleep_impl(Spark_Sleep_TypeDef sleepMode, long seconds, uint32_t param, void* reserved)
{
    SYSTEM_THREAD_CONTEXT_SYNC(system_sleep_impl(sleepMode, seconds, param, reserved));
    // TODO - determine if these are valuable:
    // - Currently publishes will get through with or without #1.
    // - More data is consumed with #1.
    // - Session is not resuming after waking from DEEP sleep,
    //   so a full handshake currently outweighs leaving the
    //   modem on for #2.
    //
    //---- #1
    // If we're connected to the cloud, make sure all
    // confirmable UDP messages are sent before sleeping
    // if (spark_cloud_flag_connected()) {
    //     Spark_Sleep();
    // }
    //---- #2
    // SLEEP_NETWORK_STANDBY can keep the modem on during DEEP sleep
    // System.sleep(10) always powers down the network, even if SLEEP_NETWORK_STANDBY flag is used.

    // Make sure all confirmable UDP messages are sent and acknowledged before sleeping
    if (spark_cloud_flag_connected() && !(param & SYSTEM_SLEEP_FLAG_NO_WAIT)) {
        Spark_Sleep();
    }

    if (network_sleep_flag(param) || SLEEP_MODE_WLAN == sleepMode) {
        network_suspend();
    }

    switch (sleepMode)
    {
        case SLEEP_MODE_WLAN:
            if (seconds)
            {
                HAL_RTC_Set_UnixAlarm((time_t) seconds);
            }
            break;

        case SLEEP_MODE_DEEP:
            if (network_sleep_flag(param))
            {
                network_disconnect(0, NETWORK_DISCONNECT_REASON_SLEEP, NULL);
                network_off(0, 0, 0, NULL);
            }

            system_power_management_sleep();
            return HAL_Core_Enter_Standby_Mode(seconds,
                    (param & SLEEP_DISABLE_WKP_PIN.value()) ? HAL_STANDBY_MODE_FLAG_DISABLE_WKP_PIN : 0);
            break;

#if HAL_PLATFORM_SETUP_BUTTON_UX
        case SLEEP_MODE_SOFTPOWEROFF:
            network_disconnect(0, NETWORK_DISCONNECT_REASON_SLEEP, NULL);
            network_off(0, 0, 0, NULL);
            sleep_fuel_gauge();
            system_power_management_sleep();
            return HAL_Core_Enter_Standby_Mode(seconds,
                    (param & SLEEP_DISABLE_WKP_PIN.value()) ? HAL_STANDBY_MODE_FLAG_DISABLE_WKP_PIN : 0);
            break;
#endif
    }
    return 0;
}

int system_sleep_pin_impl(const uint16_t* pins, size_t pins_count, const InterruptMode* modes, size_t modes_count, long seconds, uint32_t param, void* reserved)
{
    SYSTEM_THREAD_CONTEXT_SYNC(system_sleep_pin_impl(pins, pins_count, modes, modes_count, seconds, param, reserved));

    // Make sure all confirmable UDP messages are sent and acknowledged before sleeping
    if (spark_cloud_flag_connected() && !(param & SYSTEM_SLEEP_FLAG_NO_WAIT)) {
        Spark_Sleep();
    }

    bool network_sleep = network_sleep_flag(param);
    if (network_sleep)
    {
        network_suspend();
    }

#if HAL_PLATFORM_CELLULAR
    if (!network_sleep_flag(param)) {
        // Pause the modem Serial
        cellular_pause(nullptr);
    }
#endif // HAL_PLATFORM_CELLULAR

#if HAL_PLATFORM_MESH
    // FIXME: We are still going to turn off OpenThread with SLEEP_NETWORK_STANDBY, otherwise
    // there are various issues with sleep
    if (!network_sleep_flag(param)) {
        network_off(NETWORK_INTERFACE_MESH, 0, 0, nullptr);
    }
#endif // HAL_PLATFORM_MESH

    led_set_update_enabled(0, nullptr); // Disable background LED updates
    LED_Off(LED_RGB);
	system_power_management_sleep();
    int ret = HAL_Core_Enter_Stop_Mode_Ext(pins, pins_count, modes, modes_count, seconds, nullptr);
    led_set_update_enabled(1, nullptr); // Enable background LED updates

#if HAL_PLATFORM_CELLULAR
    if (!network_sleep_flag(param)) {
        // Pause the modem Serial
        cellular_resume(nullptr);
    }
#endif // HAL_PLATFORM_CELLULAR

#if HAL_PLATFORM_MESH
    // FIXME: we need to bring Mesh interface back up because we've turned it off
    // despite SLEEP_NETWORK_STANDBY
    if (!network_sleep_flag(param)) {
        network_on(NETWORK_INTERFACE_MESH, 0, 0, nullptr);
    }
#endif // HAL_PLATFORM_MESH

    if (network_sleep)
    {
        network_resume();   // asynchronously bring up the network/cloud
    }

    // if single-threaded, managed mode then reconnect to the cloud (for up to 60 seconds)
    auto mode = system_mode();
    if (system_thread_get_state(nullptr)==spark::feature::DISABLED && (mode==AUTOMATIC || mode==SEMI_AUTOMATIC) && spark_cloud_flag_auto_connect()) {
        waitFor(spark_cloud_flag_connected, 60000);
    }

    if (spark_cloud_flag_connected()) {
        Spark_Wake();
    }
    return ret;
}

/**
 * Wraps the actual implementation, which has to return a value as part of the threaded implementation.
 */
int system_sleep_pin(uint16_t wakeUpPin, uint16_t edgeTriggerMode, long seconds, uint32_t param, void* reserved)
{
    // Cancel current connection attempt to unblock the system thread
    network_connect_cancel(0, 1, 0, 0);
    InterruptMode m = (InterruptMode)edgeTriggerMode;
    return system_sleep_pin_impl(&wakeUpPin, 1, &m, 1, seconds, param, reserved);
}

int system_sleep(Spark_Sleep_TypeDef sleepMode, long seconds, uint32_t param, void* reserved)
{
    network_connect_cancel(0, 1, 0, 0);
    return system_sleep_impl(sleepMode, seconds, param, reserved);
}

int system_sleep_pins(const uint16_t* pins, size_t pins_count, const InterruptMode* modes, size_t modes_count, long seconds, uint32_t param, void* reserved)
{
    network_connect_cancel(0, 1, 0, 0);
    return system_sleep_pin_impl(pins, pins_count, modes, modes_count, seconds, param, reserved);
}


static bool system_sleep_network_suspend(network_interface_index index) {
    bool resume = false;
    // Disconnect from network
    if (network_connecting(index, 0, NULL) || network_ready(index, 0, NULL)) {
        if (network_connecting(index, 0, NULL)) {
            network_connect_cancel(index, 1, 0, 0);
        }
        network_disconnect(index, NETWORK_DISCONNECT_REASON_SLEEP, NULL);
        resume = true;
    }
    // Turn off the modem
    network_off(index, 0, 0, NULL);
    return resume;
}

static int system_sleep_network_resume(network_interface_index index) {
    network_on(NETWORK_INTERFACE_ETHERNET, 0, 0, nullptr);
    network_connect(NETWORK_INTERFACE_ETHERNET, 0, 0, nullptr);
    return SYSTEM_ERROR_NONE;
}

int system_sleep_ext(hal_sleep_config_t* config, WakeupReason* reason, void* reserved) {
    SYSTEM_THREAD_CONTEXT_SYNC(system_sleep_ext(config, reason, reserved));

    SystemSleepConfigurationHelper configHelper(config);
    int ret;

#ifndef SPARK_NO_CLOUD
    bool cloudResume = false;
    // Disconnect from cloud is necessary.
    // Make sure all confirmable UDP messages are sent and acknowledged before sleeping
    if (configHelper.cloudDisconnectRequested() && spark_cloud_flag_connected()) {
        if (configHelper.sleepWait() == SystemSleepWait::CLOUD) {
            Spark_Sleep();
        }
        cloudResume = spark_cloud_flag_auto_connect();
        // Clear the auto connect status
        spark_cloud_flag_disconnect();
    }
#endif

    // Network disconnect
#if HAL_PLATFORM_CELLULAR
    bool cellularResume = false;
    if (configHelper.networkDisconnectRequested(NETWORK_INTERFACE_CELLULAR)) {
        if (system_sleep_network_suspend(NETWORK_INTERFACE_CELLULAR)) {
            cellularResume = true;
        }
    } else {
        // Pause the modem Serial, while leaving the modem keeps running.
        cellular_pause(nullptr);
    }
#endif
#if HAL_PLATFORM_WIFI
    bool wifiResume = false;
    if (configHelper.networkDisconnectRequested(NETWORK_INTERFACE_WIFI_STA)) {
        if (system_sleep_network_suspend(NETWORK_INTERFACE_WIFI_STA)) {
            wifiResume = true;
        }
    }
#endif
#if HAL_PLATFORM_MESH
    bool meshResume = false;
    if (configHelper.networkDisconnectRequested(NETWORK_INTERFACE_MESH)) {
        if (system_sleep_network_suspend(NETWORK_INTERFACE_MESH)) {
            meshResume = true;
        }
    }
#endif
#if HAL_PLATFORM_ETHERNET
    bool ethernetResume = false;
    if (configHelper.networkDisconnectRequested(NETWORK_INTERFACE_ETHERNET)) {
        if (system_sleep_network_suspend(NETWORK_INTERFACE_ETHERNET)) {
            ethernetResume = true;
        }
    }
#endif
    // Let the sleep HAL layer to turn off the NCP interface if necessary.

    // Stop RGB signaling
    led_set_update_enabled(0, nullptr); // Disable background LED updates
    LED_Off(LED_RGB);

	system_power_management_sleep();

    // Now enter sleep mode
    hal_wakeup_source_base_t* wakeupSource = nullptr;
    ret = hal_sleep(configHelper.halConfig(), &wakeupSource, nullptr);
    if (wakeupSource) {
        // FIXME: wakeup source type to wakeup reason
        *reason = static_cast<WakeupReason>(wakeupSource->type);
    }

    // Start RGB signaling.
    led_set_update_enabled(1, nullptr); // Enable background LED updates

    // Network resume
#if HAL_PLATFORM_CELLULAR
    if (cellularResume) {
        system_sleep_network_resume(NETWORK_INTERFACE_CELLULAR);
    } else {
        cellular_resume(nullptr);
    }
#endif
#if HAL_PLATFORM_WIFI
    if (wifiResume) {
        SPARK_WLAN_SLEEP = 0;
        system_sleep_network_resume(NETWORK_INTERFACE_WIFI_STA);
    }
#endif
#if HAL_PLATFORM_MESH
    if (meshResume) {
        // FIXME: we need to bring Mesh interface back up because we've turned it off
        // despite SLEEP_NETWORK_STANDBY
        system_sleep_network_resume(NETWORK_INTERFACE_MESH);
    }
#endif
#if HAL_PLATFORM_ETHERNET
    if (ethernetResume) {
        system_sleep_network_resume(NETWORK_INTERFACE_ETHERNET);
    }
#endif

#ifndef SPARK_NO_CLOUD
    if (cloudResume) {
        // Resume cloud connection.
        spark_cloud_flag_connect();

        // if single-threaded, managed mode then reconnect to the cloud (for up to 60 seconds)
        auto mode = system_mode();
        if (system_thread_get_state(nullptr)==spark::feature::DISABLED && (mode==AUTOMATIC || mode==SEMI_AUTOMATIC) && spark_cloud_flag_auto_connect()) {
            waitFor(spark_cloud_flag_connected, 60000);
        }
        if (spark_cloud_flag_connected()) {
            Spark_Wake();
        }
    }
#endif

    return ret;
}
