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
#include "firmware_update.h"
#include "spark_wiring_system.h"
#include "led_service.h"
#include "system_task.h"
#if HAL_PLATFORM_CELLULAR
#include "cellular_hal.h"
#endif // HAL_PLATFORM_CELLULAR
#include "check.h"
#include "system_network_manager.h"

using namespace particle;
using namespace particle::system;

#undef LOG_COMPILE_TIME_LEVEL
#define LOG_COMPILE_TIME_LEVEL LOG_LEVEL_ALL

namespace {

typedef struct {
    bool suspended;
    bool on;
    bool connected;
} network_status_t;

#if PLATFORM_ID != PLATFORM_GCC
network_status_t system_sleep_network_suspend(network_interface_index index) {
    network_status_t status = {};
    status.suspended = true;

    // Disconnect from network
    if (network_connecting(index, 0, nullptr) || network_ready(index, 0, nullptr)) {
        if (network_connecting(index, 0, nullptr)) {
            network_connect_cancel(index, 1, 0, 0);
        }
        network_disconnect(index, NETWORK_DISCONNECT_REASON_SLEEP, nullptr);
        status.connected = true;
    }

    // Turn off the modem
    if (!network_is_off(index, nullptr)) {
#if HAL_PLATFORM_IFAPI
        if_t iface;
        if (!if_get_by_index(index, &iface)) {
            if (NetworkManager::instance()->isInterfacePowerState(iface, IF_POWER_STATE_UP) ||
                    NetworkManager::instance()->isInterfacePowerState(iface, IF_POWER_STATE_POWERING_UP)) {
                status.on = true;
            }
        }
#endif
        network_off(index, 0, 0, NULL);
        LOG(TRACE, "Waiting interface %d to be off...", (int)index);
        // There might be up to 30s delay to turn off the modem for particular platforms.
        network_wait_off(index, 120000/*ms*/, nullptr);
    } else {
        LOG(TRACE, "Interface %d is off already", (int)index);
    }

    return status;
}

int system_sleep_network_resume(network_interface_index index, network_status_t status) {
    if (status.on && network_is_off(index, nullptr)) {
        network_on(index, 0, 0, nullptr);
    }
    if (status.connected) {
        network_connect(index, 0, 0, nullptr);
    }
    return SYSTEM_ERROR_NONE;
}
#endif // PLATFORM_ID != PLATFORM_GCC

} // anonymous namespacee

int system_sleep_ext_impl(const hal_sleep_config_t* config, hal_wakeup_source_base_t** reason, void* reserved) {
    SYSTEM_THREAD_CONTEXT_SYNC(system_sleep_ext_impl(config, reason, reserved));

    // Validates the sleep configuration previous to disconnecting network,
    // so that the network status remains if the configuration is invalid.
    CHECK(hal_sleep_validate_config(config, nullptr));

    // Cancel the firmware update
    system::FirmwareUpdate::instance()->finishUpdate(FirmwareUpdateFlag::CANCEL);

    SystemSleepConfigurationHelper configHelper(config);

    bool reconnectCloud = false;
    bool sendCloudPing = false;
    // Disconnect from cloud is necessary.
    if (configHelper.cloudDisconnectRequested()) {
        unsigned duration = 0; // Sleep duration in seconds
        const auto src = (const hal_wakeup_source_rtc_t*)configHelper.wakeupSourceFeatured(HAL_WAKEUP_SOURCE_TYPE_RTC);
        if (src) {
            duration = src->ms / 1000;
        }
        // This flag seems to be redundant in the presence of Particle.setDisconnectOptions() and
        // Particle.disconnect()
        if (configHelper.sleepFlags().isSet(SystemSleepFlag::WAIT_CLOUD) && spark_cloud_flag_connected()) {
            auto opts = CloudConnectionSettings::instance()->takePendingDisconnectOptions();
            opts.graceful(true);
            CloudConnectionSettings::instance()->setPendingDisconnectOptions(std::move(opts));
        }
        // The CLOUD_DISCONNECT_GRACEFULLY flag doesn't really enable graceful disconnection mode,
        // it merely indicates that it is ok to disconnect gracefully in this specific case.
        // Whether the system will actually disconnect from the cloud gracefully or not depends
        // on the disconnection options that can be set via Particle.setDisconnectOptions() or
        // Particle.disconnect().
        //
        // TODO: Rename CLOUD_DISCONNECT_GRACEFULLY to CLOUD_DISCONNECT_IMMEDIATELY and invert its
        // meaning to avoid confusion
        cloud_disconnect(CLOUD_DISCONNECT_GRACEFULLY, CLOUD_DISCONNECT_REASON_SLEEP, NETWORK_DISCONNECT_REASON_NONE,
                RESET_REASON_NONE, duration);
        reconnectCloud = spark_cloud_flag_auto_connect();
        // Clear the auto connect status
        spark_cloud_flag_disconnect();
    } else if (spark_cloud_flag_connected()) {
        sendCloudPing = true;
    }

    // TODO: restore network state if network is disconnected but it failed to enter sleep mode.

    // Network disconnect.
    // FIXME: if_get_list() can be potentially used, instead of using pre-processor.
#if HAL_PLATFORM_CELLULAR
    network_status_t cellularStatus = {};
    if (!configHelper.wakeupByNetworkInterface(NETWORK_INTERFACE_CELLULAR)) {
        if (configHelper.networkFlags(NETWORK_INTERFACE_CELLULAR).isSet(SystemSleepNetworkFlag::INACTIVE_STANDBY)) {
            // Pause the modem Serial, while leaving the modem keeps running.
            cellular_pause(nullptr);
        } else {
            cellularStatus = system_sleep_network_suspend(NETWORK_INTERFACE_CELLULAR);
        }
    } else {
        cellular_urcs(false, nullptr);
    }
#endif // HAL_PLATFORM_CELLULAR

#if HAL_PLATFORM_WIFI
    network_status_t wifiStatus = {};
    if (!configHelper.wakeupByNetworkInterface(NETWORK_INTERFACE_WIFI_STA) &&
          !configHelper.networkFlags(NETWORK_INTERFACE_CELLULAR).isSet(SystemSleepNetworkFlag::INACTIVE_STANDBY)) {
        wifiStatus = system_sleep_network_suspend(NETWORK_INTERFACE_WIFI_STA);
    }
#endif // HAL_PLATFORM_WIFI

#if HAL_PLATFORM_ETHERNET
    // NOTE: wake-up by Ethernet interfaces is not implemented.
    // For now we are always powering it off
    network_status_t ethernetStatus = {};
    // if (!configHelper.wakeupByNetworkInterface(NETWORK_INTERFACE_ETHERNET)) {
        ethernetStatus = system_sleep_network_suspend(NETWORK_INTERFACE_ETHERNET);
    // }
#endif // HAL_PLATFORM_ETHERNET

    // Let the sleep HAL layer to turn off the NCP interface if necessary.

    // Stop RGB signaling
    led_set_update_enabled(0, nullptr); // Disable background LED updates
    LED_Off(PARTICLE_LED_RGB);

    system_power_management_sleep(configHelper.wakeupByFuelGauge() ? false : true);

    // Now enter sleep mode
    int ret = hal_sleep_enter(config, reason, nullptr);

    system_power_management_wakeup();

    led_set_update_enabled(1, nullptr); // Enable background LED updates
    LED_On(PARTICLE_LED_RGB); // Turn RGB on in case that RGB is controlled by user application before entering sleep mode.

    // Network resume
    // FIXME: if_get_list() can be potentially used, instead of using pre-processor.
#if HAL_PLATFORM_CELLULAR
    if (cellularStatus.suspended) {
        system_sleep_network_resume(NETWORK_INTERFACE_CELLULAR, cellularStatus);
    } else {
        cellular_resume(nullptr);
        cellular_urcs(true, nullptr);
    }
#endif // HAL_PLATFORM_CELLULAR

#if HAL_PLATFORM_WIFI
    if (wifiStatus.suspended) {
        system_sleep_network_resume(NETWORK_INTERFACE_WIFI_STA, wifiStatus);
    }
#endif // HAL_PLATFORM_WIFI

#if HAL_PLATFORM_ETHERNET
    if (ethernetStatus.suspended) {
        system_sleep_network_resume(NETWORK_INTERFACE_ETHERNET, ethernetStatus);
    }
#endif // HAL_PLATFORM_ETHERNET

    if (reconnectCloud) {
        // Resume cloud connection.
        spark_cloud_flag_connect();

        // if single-threaded, managed mode then reconnect to the cloud (for up to 60 seconds)
        auto mode = system_mode();
        if (system_thread_get_state(nullptr)==spark::feature::DISABLED && (mode==AUTOMATIC || mode==SEMI_AUTOMATIC) && spark_cloud_flag_auto_connect()) {
            waitFor(spark_cloud_flag_connected, 60000);
        }
    } else if (sendCloudPing) {
        spark_protocol_command(system_cloud_protocol_instance(), ProtocolCommands::PING, 0, nullptr);
    }

    return ret;
}

int system_sleep_ext(const hal_sleep_config_t* config, hal_wakeup_source_base_t** reason, void* reserved) {
    LOG(TRACE, "Entering system_sleep_ext()");
    return system_sleep_ext_impl(config, reason, reserved);
}
