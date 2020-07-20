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

#undef LOG_COMPILE_TIME_LEVEL
#define LOG_COMPILE_TIME_LEVEL LOG_LEVEL_ALL

#include "logging.h"
LOG_SOURCE_CATEGORY("system.sleep");

#include "system_sleep.h"
#include "system_sleep_configuration.h"
#include "system_network.h"
#include "system_power.h"
#include "system_threading.h"
#include "system_cloud.h"
#include "system_cloud_internal.h"
#include "system_mode.h"
#include "spark_wiring_system.h"
#include "led_service.h"
#include "system_task.h"
#if HAL_PLATFORM_CELLULAR
#include "cellular_hal.h"
#endif // HAL_PLATFORM_CELLULAR
#include "check.h"

using namespace particle;

#undef LOG_COMPILE_TIME_LEVEL
#define LOG_COMPILE_TIME_LEVEL LOG_LEVEL_ALL

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
    LOG(TRACE, "Waiting interface to be off...");
    // There might be up to 30s delay to turn off the modem for particular platforms.
    network_wait_off(index, 120000/*ms*/, nullptr);
    return resume;
}

static int system_sleep_network_resume(network_interface_index index) {
    network_on(index, 0, 0, nullptr);
    network_connect(index, 0, 0, nullptr);
    return SYSTEM_ERROR_NONE;
}

int system_sleep_ext(const hal_sleep_config_t* config, hal_wakeup_source_base_t** reason, void* reserved) {
    SYSTEM_THREAD_CONTEXT_SYNC(system_sleep_ext(config, reason, reserved));

    LOG(TRACE, "Entering system_sleep_ext()");

    // Validates the sleep configuration previous to disconnecting network,
    // so that the network status remains if the configuration is invalid.
    CHECK(hal_sleep_validate_config(config, nullptr));

    SystemSleepConfigurationHelper configHelper(config);

    bool cloudResume = false;
    // Disconnect from cloud is necessary.
    // Make sure all confirmable UDP messages are sent and acknowledged before sleeping
    if (configHelper.cloudDisconnectRequested() && spark_cloud_flag_connected()) {
        if (configHelper.sleepFlags().isSet(SystemSleepFlag::WAIT_CLOUD)) {
            unsigned duration = 0; // Sleep duration in seconds
            const auto src = (const hal_wakeup_source_rtc_t*)configHelper.wakeupSourceFeatured(HAL_WAKEUP_SOURCE_TYPE_RTC);
            if (src) {
                duration = src->ms / 1000;
            }
            Spark_Sleep(duration);
        }
        cloudResume = spark_cloud_flag_auto_connect();
        // Clear the auto connect status
        spark_cloud_flag_disconnect();
    }

    // TODO: restore network state if network is disconnected but it failed to enter sleep mode.

    // Network disconnect.
    // FIXME: if_get_list() can be potentially used, instead of using pre-processor.
#if HAL_PLATFORM_CELLULAR
    bool cellularResume = false;
    if (!configHelper.wakeupByNetworkInterface(NETWORK_INTERFACE_CELLULAR)) {
        if (configHelper.networkFlags(NETWORK_INTERFACE_CELLULAR).isSet(SystemSleepNetworkFlag::INACTIVE_STANDBY)) {
            // Pause the modem Serial, while leaving the modem keeps running.
            cellular_pause(nullptr);
        } else {
            if (system_sleep_network_suspend(NETWORK_INTERFACE_CELLULAR)) {
                cellularResume = true;
            }
        }
    } else {
        // Pause the modem Serial, while leaving the modem keeps running.
        cellular_pause(nullptr);
    }
#endif // HAL_PLATFORM_CELLULAR

#if HAL_PLATFORM_WIFI
    bool wifiResume = false;
    if (!configHelper.wakeupByNetworkInterface(NETWORK_INTERFACE_WIFI_STA)) {
        if (system_sleep_network_suspend(NETWORK_INTERFACE_WIFI_STA)) {
            wifiResume = true;
        }
    }
#endif // HAL_PLATFORM_WIFI

#if HAL_PLATFORM_ETHERNET
    // NOTE: wake-up by Ethernet interfaces is not implemented.
    // For now we are always powering it off
    bool ethernetResume = false;
    // if (!configHelper.wakeupByNetworkInterface(NETWORK_INTERFACE_ETHERNET)) {
        if (system_sleep_network_suspend(NETWORK_INTERFACE_ETHERNET)) {
            ethernetResume = true;
        }
    // }
#endif // HAL_PLATFORM_ETHERNET

    // Let the sleep HAL layer to turn off the NCP interface if necessary.

    // Stop RGB signaling
    led_set_update_enabled(0, nullptr); // Disable background LED updates
    LED_Off(LED_RGB);

    system_power_management_sleep();

    // Now enter sleep mode
    int ret = hal_sleep_enter(config, reason, nullptr);

    system_power_management_sleep(false);

    led_set_update_enabled(1, nullptr); // Enable background LED updates
    LED_On(LED_RGB); // Turn RGB on in case that RGB is controlled by user application before entering sleep mode.

    // Network resume
    // FIXME: if_get_list() can be potentially used, instead of using pre-processor.
#if HAL_PLATFORM_CELLULAR
    if (cellularResume) {
        system_sleep_network_resume(NETWORK_INTERFACE_CELLULAR);
    } else {
        cellular_resume(nullptr);
    }
#endif // HAL_PLATFORM_CELLULAR

#if HAL_PLATFORM_WIFI
    if (wifiResume) {
        SPARK_WLAN_SLEEP = 0;
        system_sleep_network_resume(NETWORK_INTERFACE_WIFI_STA);
    }
#endif // HAL_PLATFORM_WIFI

#if HAL_PLATFORM_ETHERNET
    if (ethernetResume) {
        system_sleep_network_resume(NETWORK_INTERFACE_ETHERNET);
    }
#endif // HAL_PLATFORM_ETHERNET

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

    return ret;
}
