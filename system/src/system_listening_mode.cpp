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

using particle::LEDStatus;

#if HAL_PLATFORM_BLE
#include "ble_hal.h"
#endif /* HAL_PLATFORM_BLE */

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
    // Start advertising
    ble_start_advert(nullptr);
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

#if HAL_PLATFORM_BLE
    // Start advertising
    ble_stop_advert(nullptr);
#endif /* HAL_PLATFORM_BLE */

    LED_SIGNAL_STOP(LISTENING_MODE);

    console_.reset();

    active_ = false;

    system_notify_event(setup_end, HAL_Timer_Get_Milli_Seconds() - timestampStarted_);

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

#endif /* HAL_PLATFORM_IFAPI */
