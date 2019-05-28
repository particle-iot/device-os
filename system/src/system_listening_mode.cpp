/*
 * Copyright (c) 2018 Particle Industries, Inc.  All rights reserved.
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

#include "logging.h"
LOG_SOURCE_CATEGORY("system.listen")

#include "system_listening_mode.h"

#if HAL_PLATFORM_IFAPI

#include "system_error.h"
#include "system_led_signal.h"
#include "spark_wiring_led.h"
#include "system_network_manager.h"
#include "system_task.h"
#include "system_threading.h"
#include "system_network.h"
#include "delay_hal.h"
#include "system_control_internal.h"
#include "check.h"
#include "system_event.h"
#include "scope_guard.h"

using particle::LEDStatus;

namespace {

using namespace particle::system;

ListeningModeHandler g_listenModeHandler;

const auto SETUP_UPDATE_INTERVAL = 1000;

} // unnamed

ListeningModeHandler::ListeningModeHandler()
        : active_(false) {

}

ListeningModeHandler::~ListeningModeHandler() {
}

ListeningModeHandler* ListeningModeHandler::instance() {
    return &g_listenModeHandler;
}

int ListeningModeHandler::enter(unsigned int timeout) {
    if (active_) {
        return SYSTEM_ERROR_INVALID_STATE;
    }

    active_ = true;
    LOG(INFO, "Entering listening mode");

    /* Disconnect from cloud and network */
    cloud_disconnect(true, false, CLOUD_DISCONNECT_REASON_LISTENING);
    NetworkManager::instance()->deactivateConnections();

    LED_SIGNAL_START(LISTENING_MODE, CRITICAL);
    system_notify_event(setup_begin, 0);
    timestampStarted_ = timestampUpdate_ = HAL_Timer_Get_Milli_Seconds();

#if HAL_PLATFORM_BLE
    bleHandler_.enter();
#endif /* HAL_PLATFORM_BLE */

#if !HAL_PLATFORM_WIFI
    SystemSetupConsoleConfig config;
    console_.reset(new SystemSetupConsole<SystemSetupConsoleConfig>(config));
#else
    WiFiSetupConsoleConfig config = {};
    config.connect_callback2 = [](void*, NetworkCredentials* creds, bool dryRun) -> int {
        // NOTE: dry run is not supported
        if (dryRun) {
            return 0;
        }
        if (creds) {
            CHECK(network_set_credentials(NETWORK_INTERFACE_WIFI_STA, 0, creds, nullptr));
            // Exit listening mode
            instance()->enqueueCommand(NETWORK_LISTEN_COMMAND_EXIT, nullptr);
        }
        return 0;
    };
    console_.reset(new WiFiSetupConsole(config));
#endif // HAL_PLATFORM_WIFI

    return 0;
}

int ListeningModeHandler::exit() {
    if (!active_) {
        return SYSTEM_ERROR_INVALID_STATE;
    }

    LOG(INFO, "Exiting listening mode");

    LED_SIGNAL_STOP(LISTENING_MODE);

    console_.reset();

    active_ = false;

    system_notify_event(setup_end, HAL_Timer_Get_Milli_Seconds() - timestampStarted_);

#if HAL_PLATFORM_BLE
    bleHandler_.exit();
#endif /* HAL_PLATFORM_BLE */

    return 0;
}

bool ListeningModeHandler::isActive() const {
    return active_;
}

int ListeningModeHandler::run() {
    if (!active_) {
        return SYSTEM_ERROR_INVALID_STATE;
    }

    if (console_) {
        console_->loop();
    }

    if ((HAL_Timer_Get_Milli_Seconds() - timestampUpdate_) >= SETUP_UPDATE_INTERVAL) {
        const auto now = HAL_Timer_Get_Milli_Seconds();
        system_notify_event(setup_update, now - timestampStarted_);
        timestampUpdate_ = now;
    }

    return 0;
}

int ListeningModeHandler::command(network_listen_command_t com, void* arg) {
    switch (com) {
        case NETWORK_LISTEN_COMMAND_ENTER: {
            return enter();
        }
        case NETWORK_LISTEN_COMMAND_EXIT: {
            return exit();
        }
        case NETWORK_LISTEN_COMMAND_CLEAR_CREDENTIALS: {
            /* TODO: LED indication */
            return clearNetworkConfiguration();
        }
    }

    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int ListeningModeHandler::enqueueCommand(network_listen_command_t com, void* arg) {
    auto task = static_cast<Task*>(system_pool_alloc(sizeof(Task), nullptr));
    if (!task) {
        return SYSTEM_ERROR_NO_MEMORY;
    }

    memset(task, 0, sizeof(Task));
    task->command = com;
    task->arg = arg;
    task->func = reinterpret_cast<ISRTaskQueue::TaskFunc>(&executeEnqueuedCommand);

    SystemISRTaskQueue.enqueue(task);

    return 0;
}

int ListeningModeHandler::setTimeout(unsigned int timeout) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

unsigned int ListeningModeHandler::getTimeout() const {
    return 0;
}

void ListeningModeHandler::executeEnqueuedCommand(Task* task) {
    auto com = task->command;
    auto arg = task->arg;

    system_pool_free(task, nullptr);

    instance()->command(com, arg);
}

int ListeningModeHandler::clearNetworkConfiguration() const {
    if (!active_) {
        return SYSTEM_ERROR_INVALID_STATE;
    }

    LOG(INFO, "Clearing network settings");

    /* FIXME: this needs to be refactored */

    // Get base color used for the listening mode indication
    const LEDStatusData* status = led_signal_status(LED_SIGNAL_LISTENING_MODE, nullptr);
    LEDStatus led(status ? status->color : RGB_COLOR_BLUE, LED_PRIORITY_CRITICAL);
    led.setActive();
    int toggle = 25;
    while (toggle--) {
        led.toggle();
        HAL_Delay_Milliseconds(50);
    }

    int r = NetworkManager::instance()->clearConfiguration();
    if (r) {
        led.setColor(RGB_COLOR_RED);
        led.on();

        int toggle = 25;
        while (toggle--) {
            led.toggle();
            HAL_Delay_Milliseconds(50);
        }
    }

    return r;
}

#if HAL_PLATFORM_BLE
BleListeningModeHandler::BleListeningModeHandler()
        : preAdvData_(nullptr),
          preAdvDataLen_(0),
          preSrData_(nullptr),
          preSrDataLen_(0),
          preAdvParams_(),
          prePpcp_(),
          preAdvertising_(false),
          preConnected_(false),
          preAutoAdv_(BLE_AUTO_ADV_ALWAYS) {
}

BleListeningModeHandler::~BleListeningModeHandler() {
    if (!preAdvData_) {
        free(preAdvData_);
        preAdvData_ = nullptr;
    }
    if (!preSrData_) {
        free(preSrData_);
        preSrData_ = nullptr;
    }
}

int BleListeningModeHandler::enter() {
    // TODO: Would it be better to call the method of BLE Control Request Channel to isolate the logic?
    // Cache the application specific advertising data, advertising parameters and connection parameters.
    preAdvDataLen_ = hal_ble_gap_get_advertising_data(nullptr, 0, nullptr);
    CHECK(preAdvDataLen_);
    if (preAdvDataLen_ > 0) {
        preAdvData_ = (uint8_t*)malloc(preAdvDataLen_);
        CHECK_TRUE(preAdvData_, SYSTEM_ERROR_NO_MEMORY);
        hal_ble_gap_get_advertising_data(preAdvData_, preAdvDataLen_, nullptr);
    }
    preSrDataLen_ = hal_ble_gap_get_scan_response_data(nullptr, 0, nullptr);
    CHECK(preSrDataLen_);
    if (preSrDataLen_ > 0) {
        preSrData_ = (uint8_t*)malloc(preSrDataLen_);
        CHECK_TRUE(preSrData_, SYSTEM_ERROR_NO_MEMORY);
        hal_ble_gap_get_scan_response_data(preSrData_, preSrDataLen_, nullptr);
    }
    preAdvParams_.size = sizeof(hal_ble_adv_params_t);
    CHECK(hal_ble_gap_get_advertising_parameters(&preAdvParams_, nullptr));
    CHECK(hal_ble_gap_get_ppcp(&prePpcp_, nullptr));
    CHECK(hal_ble_gap_get_auto_advertise(&preAutoAdv_, nullptr));
    preAdvertising_ = hal_ble_gap_is_advertising(nullptr);
    preConnected_ = hal_ble_gap_is_connected(nullptr, nullptr);

    // Set PPCP (Peripheral Preferred Connection Parameters)
    hal_ble_conn_params_t ppcp = {};
    ppcp.min_conn_interval = BLE_DEFAULT_MIN_CONN_INTERVAL;
    ppcp.max_conn_interval = BLE_DEFAULT_MAX_CONN_INTERVAL;
    ppcp.conn_sup_timeout = BLE_DEFAULT_CONN_SUP_TIMEOUT;
    ppcp.slave_latency = BLE_DEFAULT_SLAVE_LATENCY;
    CHECK(hal_ble_gap_set_ppcp(&ppcp, nullptr));

    // Complete local name
    char devName[32] = {};
    CHECK(hal_ble_gap_get_device_name(devName, sizeof(devName), nullptr));

    // Particle specific Manufacture data
    uint8_t mfgData[BLE_MAX_ADV_DATA_LEN];
    size_t mfgDataLen = 0;
    uint16_t platformID = PLATFORM_ID;
    uint16_t companyID = PARTICLE_COMPANY_ID;
    memcpy(&mfgData[mfgDataLen], (uint8_t*)&companyID, 2);
    mfgDataLen += 2;
    memcpy(&mfgData[mfgDataLen], (uint8_t*)&platformID, sizeof(platformID));
    mfgDataLen += sizeof(platformID);

    uint8_t advData[BLE_MAX_ADV_DATA_LEN] = {};
    size_t advDataLen = 0;
    advData[advDataLen++] = 0x02;
    advData[advDataLen++] = BLE_SIG_AD_TYPE_FLAGS;
    advData[advDataLen++] = BLE_SIG_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;
    advData[advDataLen++] = strlen(devName) + 1;
    advData[advDataLen++] = BLE_SIG_AD_TYPE_COMPLETE_LOCAL_NAME;
    memcpy(&advData[advDataLen], devName, strlen(devName));
    advDataLen += strlen(devName);
    advData[advDataLen++] = mfgDataLen + 1;
    advData[advDataLen++] = BLE_SIG_AD_TYPE_MANUFACTURER_SPECIFIC_DATA;
    memcpy(&advData[advDataLen], mfgData, mfgDataLen);
    advDataLen += mfgDataLen;
    CHECK(hal_ble_gap_set_advertising_data(advData, advDataLen, nullptr));

    // Particle Control Request Service 128-bits UUID
    uint8_t srData[BLE_MAX_ADV_DATA_LEN] = {};
    const uint8_t CTRL_SERVICE_UUID[] = {0xfc,0x36,0x6f,0x54,0x30,0x80,0xf4,0x94,0xa8,0x48,0x4e,0x5c,0x01,0x00,0xa9,0x6f};
    size_t srDataLen = 0;
    srData[srDataLen++] = 0x11;
    srData[srDataLen++] = BLE_SIG_AD_TYPE_128BIT_SERVICE_UUID_COMPLETE;
    memcpy(&srData[srDataLen], CTRL_SERVICE_UUID, sizeof(CTRL_SERVICE_UUID));
    srDataLen += sizeof(CTRL_SERVICE_UUID);
    CHECK(hal_ble_gap_set_scan_response_data(srData, srDataLen, nullptr));

    // Advertising parameters
    hal_ble_adv_params_t advParams = {};
    advParams.type = BLE_ADV_CONNECTABLE_SCANNABLE_UNDIRECRED_EVT;
    advParams.filter_policy = BLE_ADV_FP_ANY;
    advParams.interval = BLE_DEFAULT_ADVERTISING_INTERVAL;
    advParams.timeout = BLE_DEFAULT_ADVERTISING_TIMEOUT;
    advParams.inc_tx_power = false;
    CHECK(hal_ble_gap_set_advertising_parameters(&advParams, nullptr));

    CHECK(hal_ble_gap_set_auto_advertise(BLE_AUTO_ADV_ALWAYS, nullptr));

    if (!preAdvertising_ && !preConnected_) {
        // Start advertising if it not connected as BLE Peripheral
        CHECK(hal_ble_gap_start_advertising(nullptr));
    }

    return SYSTEM_ERROR_NONE;
}

int BleListeningModeHandler::exit() {
    /*
     * TODO: Would it be better to call the method of BLE Control Request Channel to isolate the logic?
     *
     * FIXME: It's not possible to restore the final state that is changed by user application
     * during in the listening mode when threading is enabled.
     */
    SCOPE_GUARD ({
        if (preAdvData_) {
            free(preAdvData_);
            preAdvData_ = nullptr;
        }
        if (preSrData_) {
            free(preSrData_);
            preSrData_ = nullptr;
        }
    });

    // Restore the advertising data, advertising parameters and connection parameters.
    CHECK(hal_ble_gap_set_advertising_data(preAdvData_, preAdvDataLen_, nullptr));
    CHECK(hal_ble_gap_set_scan_response_data(preSrData_, preSrDataLen_, nullptr));
    CHECK(hal_ble_gap_set_ppcp(&prePpcp_, nullptr));
    CHECK(hal_ble_gap_set_advertising_parameters(&preAdvParams_, nullptr));
    CHECK(hal_ble_gap_set_auto_advertise(preAutoAdv_, nullptr));

    // It is advertising when entering listening mode.
    if (preAdvertising_) {
        if (!hal_ble_gap_is_connected(nullptr, nullptr)) {
            CHECK(hal_ble_gap_start_advertising(nullptr));
        }
        // Currently it is connected. When disconnected, use previous automatic advertising scheme.
        return SYSTEM_ERROR_NONE;
    }
    // It is connected but not advertising when entering listening mode.
    if (preConnected_) {
        if (!hal_ble_gap_is_connected(nullptr, nullptr)) {
            if (preAutoAdv_ == BLE_AUTO_ADV_ALWAYS) {
                CHECK(hal_ble_gap_start_advertising(nullptr));
            } else {
                CHECK(hal_ble_gap_stop_advertising(nullptr));
            }
        }
        return SYSTEM_ERROR_NONE;
    }
    // It is neither connected nor advertising when entering listening mode.
    if (hal_ble_gap_is_connected(nullptr, nullptr)) {
        if (preAutoAdv_ == BLE_AUTO_ADV_ALWAYS) {
            // Do not automatically start advertising when disconnected.
            preAutoAdv_ = BLE_AUTO_ADV_SINCE_NEXT_CONN;
            CHECK(hal_ble_gap_set_auto_advertise(preAutoAdv_, nullptr));
        }
        return SYSTEM_ERROR_NONE;
    }
    // Stop advertising anyway.
    CHECK(hal_ble_gap_stop_advertising(nullptr));
    return SYSTEM_ERROR_NONE;
}
#endif

#endif /* HAL_PLATFORM_IFAPI */
