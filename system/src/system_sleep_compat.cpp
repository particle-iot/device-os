/*
 * Copyright (c) 2020 Particle Industries, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "system_sleep.h"
#include "system_network.h"
#include "system_cloud.h"
#include "system_task.h"
#include "rtc_hal.h"
#include "system_threading.h"
#include "system_cloud_internal.h"
#include "led_service.h"
#include "system_power.h"
#include "system_mode.h"
#if HAL_PLATFORM_CELLULAR
#include "cellular_hal.h"
#endif // HAL_PLATFORM_CELLULAR
#include "spark_wiring_system.h"
#include "system_sleep_configuration.h"
#include "check.h"

using namespace particle;


struct WakeupState
{
    bool wifi;
    bool wifiConnected;
    bool cloud;
};

WakeupState wakeupState;

static void network_suspend() {
    // save the current state so it can be restored on wakeup

    wakeupState.cloud = spark_cloud_flag_auto_connect();
    wakeupState.wifi = !SPARK_WLAN_SLEEP;
    wakeupState.wifiConnected = wakeupState.cloud || network_ready(0, 0, nullptr) || network_connecting(0, 0, nullptr);
    // Disconnect the cloud and the network
    network_disconnect(0, NETWORK_DISCONNECT_REASON_SLEEP, nullptr);
    // Clear the auto connect status
    spark_cloud_flag_disconnect();
    network_off(0, 0, 0, nullptr);
}

static void network_resume() {
    if (wakeupState.wifi) {
        SPARK_WLAN_SLEEP = 0;
    }
    // Outdated - Fix with GCC refactoring
    // Gen2-only: Set the system flags that triggers the wifi/cloud reconnection in the background loop
    // FIXME: Gen3 won't automatically restore the modem state and network connection if cloud auto-connect flag is not set.
    // See manage_network_connection() in system_network_manager_api.cpp.
    if (wakeupState.wifiConnected) {
        SPARK_WLAN_CONNECT_RESTORE = 1;
    }
    if (wakeupState.cloud) {
        spark_cloud_flag_connect();
    }
}

/*******************************************************************************
 * Function Name  : HAL_RTCAlarm_Handler
 * Description    : This function handles additional application requirements.
 * Input          : None.
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
void system_sleep_rtc_alarm_handler(void* context)
{
    /* Wake up from System.sleep mode(SLEEP_MODE_WLAN) */
    network_resume();
}

bool network_sleep_flag(uint32_t flags)
{
    return (flags & SYSTEM_SLEEP_FLAG_NETWORK_STANDBY) == 0;
}

int system_sleep_enter_standby_compat(long seconds, uint32_t param) {
    SystemSleepConfiguration config;
    config.mode(SystemSleepMode::HIBERNATE);

    if (seconds > 0) {
        config.duration(seconds * 1000);
    }

    if (!(param & SYSTEM_SLEEP_FLAG_DISABLE_WKP_PIN)) {
        config.gpio(WKP, RISING);
    }

    SystemSleepConfigurationHelper configHelper(config.halConfig());
    system_power_management_sleep(configHelper.wakeupByFuelGauge() ? false : true);

    return hal_sleep_enter(config.halConfig(), nullptr, nullptr);
}

int system_sleep_enter_stop_compat(const uint16_t* pins, size_t pins_count, const InterruptMode* modes, size_t modes_count, long seconds) {
    SystemSleepConfiguration config;
    config.mode(SystemSleepMode::STOP);

    if (seconds > 0) {
        config.duration(seconds * 1000);
    }

    if (pins && pins_count) {
        CHECK_TRUE(modes && modes_count, SYSTEM_ERROR_INVALID_ARGUMENT);
    }

    for (size_t i = 0; i < pins_count; i++) {
        config.gpio(pins[i], ((i < modes_count) ? modes[i] : modes[modes_count - 1]));
    }

    SystemSleepConfigurationHelper configHelper(config.halConfig());
    system_power_management_sleep(configHelper.wakeupByFuelGauge() ? false : true);

    hal_wakeup_source_base_t* wakeupSource = nullptr;
    int ret = CHECK(hal_sleep_enter(config.halConfig(), &wakeupSource, nullptr));

    system_power_management_wakeup();

    ret = 0; // 0 for RTC wakeup reason.
    if (wakeupSource) {
        if (wakeupSource->type == HAL_WAKEUP_SOURCE_TYPE_GPIO) {
            for (size_t i = 0; i < pins_count; i++) {
                if (pins[i] == reinterpret_cast<hal_wakeup_source_gpio_t*>(wakeupSource)->pin) {
                    ret = i + 1;
                    break;
                }
            }
        }
        free(wakeupSource);
    }
    return ret;
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

    if (spark_cloud_flag_connected() && !(param & SYSTEM_SLEEP_FLAG_NO_WAIT)) {
        // Whether the system will actually disconnect from the cloud gracefully or not depends
        // on the disconnection options that can be set via Particle.setDisconnectOptions() or
        // Particle.disconnect()
        cloud_disconnect(CLOUD_DISCONNECT_GRACEFULLY, CLOUD_DISCONNECT_REASON_SLEEP, NETWORK_DISCONNECT_REASON_NONE,
                RESET_REASON_NONE, seconds);
    }

#if HAL_PLATFORM_SETUP_BUTTON_UX
    bool networkTurnedOff = false;
#endif // HAL_PLATFORM_SETUP_BUTTON_UX

    if (network_sleep_flag(param) || SLEEP_MODE_WLAN == sleepMode) {
        network_suspend();
#if HAL_PLATFORM_SETUP_BUTTON_UX
        networkTurnedOff = true;
#endif // HAL_PLATFORM_SETUP_BUTTON_UX
    }

    switch (sleepMode)
    {
        case SLEEP_MODE_WLAN:
            if (seconds)
            {
                struct timeval tv = {
                    .tv_sec = seconds,
                    .tv_usec = 0
                };
                int r = hal_rtc_set_alarm(&tv, HAL_RTC_ALARM_FLAG_IN, system_sleep_rtc_alarm_handler, nullptr, nullptr);
                if (r) {
                    system_sleep_rtc_alarm_handler(nullptr);
                    return r;
                }
            }
            break;

        case SLEEP_MODE_DEEP:
            return system_sleep_enter_standby_compat(seconds, param);

#if HAL_PLATFORM_SETUP_BUTTON_UX
        case SLEEP_MODE_SOFTPOWEROFF:
            if (!networkTurnedOff) {
                network_disconnect(0, NETWORK_DISCONNECT_REASON_SLEEP, nullptr);
                network_off(0, 0, 0, nullptr);
            }
            return system_sleep_enter_standby_compat(seconds, param);
#endif
    }
    return 0;
}

int system_sleep_pin_impl(const uint16_t* pins, size_t pins_count, const InterruptMode* modes, size_t modes_count, long seconds, uint32_t param, void* reserved)
{
    SYSTEM_THREAD_CONTEXT_SYNC(system_sleep_pin_impl(pins, pins_count, modes, modes_count, seconds, param, reserved));

    bool sendCloudPing = false;
    if (spark_cloud_flag_connected()) {
        if (!(param & SYSTEM_SLEEP_FLAG_NO_WAIT)) {
            cloud_disconnect(CLOUD_DISCONNECT_GRACEFULLY, CLOUD_DISCONNECT_REASON_SLEEP, NETWORK_DISCONNECT_REASON_NONE,
                    RESET_REASON_NONE, seconds);
        } else {
            sendCloudPing = true;
        }
    }

    bool network_sleep = network_sleep_flag(param);
    if (network_sleep)
    {
        network_suspend();
        sendCloudPing = false;
    }

#if HAL_PLATFORM_CELLULAR
    if (!network_sleep_flag(param)) {
        // Pause the modem Serial
        cellular_pause(nullptr);
    }
#endif // HAL_PLATFORM_CELLULAR

    led_set_update_enabled(0, nullptr); // Disable background LED updates
    LED_Off(PARTICLE_LED_RGB);

    int ret = system_sleep_enter_stop_compat(pins, pins_count, modes, modes_count, seconds);

    led_set_update_enabled(1, nullptr); // Enable background LED updates
    LED_On(PARTICLE_LED_RGB); // Turn RGB on in case that RGB is controlled by user application before entering sleep mode.

#if HAL_PLATFORM_CELLULAR
    if (!network_sleep_flag(param)) {
        // Pause the modem Serial
        cellular_resume(nullptr);
    }
#endif // HAL_PLATFORM_CELLULAR

    if (network_sleep)
    {
        network_resume();   // asynchronously bring up the network/cloud
    }

    // if single-threaded, managed mode then reconnect to the cloud (for up to 60 seconds)
    auto mode = system_mode();
    if (system_thread_get_state(nullptr)==spark::feature::DISABLED && (mode==AUTOMATIC || mode==SEMI_AUTOMATIC) && spark_cloud_flag_auto_connect()) {
        waitFor(spark_cloud_flag_connected, 60000);
    }

    if (sendCloudPing) {
        spark_protocol_command(system_cloud_protocol_instance(), ProtocolCommands::PING, 0, nullptr);
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
    return system_sleep_pins(&wakeUpPin, 1, &m, 1, seconds, param, reserved);
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
