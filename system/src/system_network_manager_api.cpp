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

#include "system_network_manager.h"

#if HAL_PLATFORM_IFAPI

#include "system_network.h"
#include "system_network_internal.h"
#include "delay_hal.h"
#include "system_update.h"
#include "system_cloud_internal.h"
#include "system_threading.h"
#include <atomic>

using namespace particle::system;

/* FIXME: */
uint32_t wlan_watchdog_base;
uint32_t wlan_watchdog_duration;

/* FIXME */
volatile uint8_t SPARK_WLAN_RESET;
volatile uint8_t SPARK_WLAN_SLEEP;
volatile uint8_t SPARK_WLAN_STARTED;
extern int cfod_count;

namespace {

/* FIXME: This is not how this should be handled */
std::atomic_bool s_forcedDisconnect(false);

} /* anonymous */

void network_setup(network_handle_t network, uint32_t flags, void* reserved) {
    system_set_flag(SYSTEM_FLAG_RESET_NETWORK_ON_CLOUD_ERRORS, 0, nullptr);
    NetworkManager::instance()->init();
}

const void* network_config(network_handle_t network, uint32_t param, void* reserved) {
    return nullptr;
}

void network_connect(network_handle_t network, uint32_t flags, uint32_t param, void* reserved) {
    /* TODO: WIFI_CONNECT_SKIP_LISTEN is unhandled */
    SYSTEM_THREAD_CONTEXT_ASYNC_CALL([]() {
        SPARK_WLAN_STARTED = 1;
        SPARK_WLAN_SLEEP = 0;
        s_forcedDisconnect = false;
        if (!NetworkManager::instance()->isNetworkingEnabled()) {
            NetworkManager::instance()->enableNetworking();
        }
        NetworkManager::instance()->activateConnections();
    }());
}

void network_disconnect(network_handle_t network, uint32_t reason, void* reserved) {
    SYSTEM_THREAD_CONTEXT_ASYNC_CALL([]() {
        cloud_disconnect(true, false, CLOUD_DISCONNECT_REASON_NETWORK_DISCONNECT);
        SPARK_WLAN_STARTED = 0;
        s_forcedDisconnect = true;
        NetworkManager::instance()->deactivateConnections();
        /* FIXME */
        while (NetworkManager::instance()->getState() != NetworkManager::State::IFACE_DOWN) {
            HAL_Delay_Milliseconds(1);
        }
    }());
}

bool network_ready(network_handle_t network, uint32_t param, void* reserved)
{
    return NetworkManager::instance()->isConnectivityAvailable();
}

bool network_connecting(network_handle_t network, uint32_t param, void* reserved) {
    return NetworkManager::instance()->isEstablishingConnections();
}

int network_connect_cancel(network_handle_t network, uint32_t flags, uint32_t param1, void* reserved) {
    return 0;
}

void network_on(network_handle_t network, uint32_t flags, uint32_t param, void* reserved) {
    SYSTEM_THREAD_CONTEXT_ASYNC_CALL([]() {
        NetworkManager::instance()->enableNetworking();
        SPARK_WLAN_STARTED = 1;
        SPARK_WLAN_SLEEP = 0;
    }());
}

bool network_has_credentials(network_handle_t network, uint32_t param, void* reserved) {
    return NetworkManager::instance()->isConfigured();
}

void network_off(network_handle_t network, uint32_t flags, uint32_t param, void* reserved) {
    /* flags & 1 means also disconnect the cloud
     * (so it doesn't autmatically connect when network resumed)
     */
    SYSTEM_THREAD_CONTEXT_ASYNC_CALL([flags]() {
        SPARK_WLAN_STARTED = 0;
        SPARK_WLAN_SLEEP = 1;
        network_disconnect(0, 0, 0);
        /* FIXME */
        while (NetworkManager::instance()->getState() != NetworkManager::State::IFACE_DOWN) {
            HAL_Delay_Milliseconds(1);
        }
        NetworkManager::instance()->disableNetworking();

        if (flags & 1) {
            spark_cloud_flag_disconnect();
        }
    }());
}

void network_listen(network_handle_t network, uint32_t flags, void*) {
    /* TODO: */
}

void network_set_listen_timeout(network_handle_t network, uint16_t timeout, void*) {
    /* TODO */
}

uint16_t network_get_listen_timeout(network_handle_t network, uint32_t flags, void*) {
    /* TODO */
    return 0;
}

bool network_listening(network_handle_t network, uint32_t, void*) {
    /* TODO */
    return false;
}

int network_set_credentials(network_handle_t network, uint32_t, NetworkCredentials* credentials, void*) {
    /* TODO */
    return -1;
}

bool network_clear_credentials(network_handle_t network, uint32_t, NetworkCredentials* creds, void*) {
    /* TODO */
    return false;
}

int network_set_hostname(network_handle_t network, uint32_t flags, const char* hostname, void* reserved) {
    /* TODO */
    return -1;
}

int network_get_hostname(network_handle_t network, uint32_t flags, char* buffer, size_t buffer_len, void* reserved) {
    /* TODO */
    return -1;
}

int network_clear_settings(network_handle_t network, uint32_t flags, void* reserved) {
    /* TODO */
    return -1;
}

/**
 * Reset or initialize the network connection as required.
 */
void manage_network_connection() {
    /* Need to disconnect and disable networking */
    if (SPARK_WLAN_RESET || SPARK_WLAN_SLEEP) {
        auto wasSleeping = SPARK_WLAN_SLEEP;
        cloud_disconnect();
        network_disconnect(0, SPARK_WLAN_RESET ? NETWORK_DISCONNECT_REASON_RESET : NETWORK_DISCONNECT_REASON_NONE, 0);
        network_off(0, 0, 0, 0);
        SPARK_WLAN_RESET = 0;
        SPARK_WLAN_SLEEP = wasSleeping;
        cfod_count = 0;
    } else {
        if (spark_cloud_flag_auto_connect() && !s_forcedDisconnect) {
            if (!NetworkManager::instance()->isNetworkingEnabled()) {
                NetworkManager::instance()->enableNetworking();
            }

            if (!NetworkManager::instance()->isConnectivityAvailable() && !NetworkManager::instance()->isEstablishingConnections()) {
                NetworkManager::instance()->activateConnections();
            }
        }
    }
}

void manage_smart_config() {
}

void manage_ip_config() {
}

#endif /* HAL_PLATFORM_IFAPI */
