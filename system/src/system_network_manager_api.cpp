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
#include "system_update.h"
#include "system_cloud_internal.h"
#include "system_threading.h"
#include "system_listening_mode.h"
#include "system_event.h"
#include "delay_hal.h"

#include "dct.h"

#include <atomic>

#if HAL_PLATFORM_WIFI
#include "wlan_hal.h"
#endif
#if HAL_PLATFORM_CELLULAR
#include "cellular_hal.h"
#endif

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

bool testAndClearListeningModeFlag() {
    static bool check = true;
    if (check) {
        check = false;
        // Check startup flag
        uint8_t val = 0x00;
        system_get_flag(SYSTEM_FLAG_STARTUP_LISTEN_MODE, &val, nullptr);
        if (val) {
            system_set_flag(SYSTEM_FLAG_STARTUP_LISTEN_MODE, 0, nullptr); // Clear startup flag
            return true;
        }
        // Check setup done flag
        val = 0x01;
        dct_read_app_data_copy(DCT_SETUP_DONE_OFFSET, &val, 1);
        if (val == 0x00 || val == 0xff) {
            return true;
        }
    }
    return false;
}

#if HAL_PLATFORM_WIFI
const WLanConfig* wlanConfig() {
    // TODO: Cache current configuration
    static WLanConfig conf;
    memset(&conf, 0, sizeof(conf));
    memset(conf.BSSID, 0xff, sizeof(conf.BSSID));
    memset(conf.nw.uaMacAddr, 0xff, sizeof(conf.nw.uaMacAddr));
    conf.size = sizeof(conf);
    wlan_fetch_ipconfig(&conf);
    return &conf;
}
#endif

#if HAL_PLATFORM_CELLULAR
const CellularConfig* cellularConfig() {
    // TODO: Cache current configuration
    static CellularConfig conf;
    memset(&conf, 0, sizeof(conf));
    memset(conf.nw.uaMacAddr, 0xff, sizeof(conf.nw.uaMacAddr));
    conf.size = sizeof(conf);
    cellular_fetch_ipconfig(&conf, nullptr);
    return &conf;
}
#endif

} /* anonymous */

void network_setup(network_handle_t network, uint32_t flags, void* reserved) {
    system_set_flag(SYSTEM_FLAG_RESET_NETWORK_ON_CLOUD_ERRORS, 0, nullptr);
    NetworkManager::instance()->init();
    // Populate the list
    NetworkManager::instance()->disableInterface();
}

const void* network_config(network_handle_t network, uint32_t param, void* reserved) {
    switch (network) {
#if HAL_PLATFORM_WIFI
    case NETWORK_INTERFACE_WIFI_STA:
        return wlanConfig();
#endif
#if HAL_PLATFORM_CELLULAR
    case NETWORK_INTERFACE_CELLULAR:
        return cellularConfig();
#endif
    default:
        return nullptr;
    }
}

void network_connect(network_handle_t network, uint32_t flags, uint32_t param, void* reserved) {
    /* TODO: WIFI_CONNECT_SKIP_LISTEN is unhandled */
    SYSTEM_THREAD_CONTEXT_ASYNC_CALL([network]() {
        SPARK_WLAN_STARTED = 1;
        SPARK_WLAN_SLEEP = 0;
        s_forcedDisconnect = false;

        if (testAndClearListeningModeFlag() || !NetworkManager::instance()->isConfigured()) {
            /* Enter listening mode */
            network_listen(0, 0, 0);
            return;
        }

        if (network != NETWORK_INTERFACE_ALL) {
            if_t iface;
            if (!if_get_by_index(network, &iface)) {
                NetworkManager::instance()->enableInterface(iface);
                NetworkManager::instance()->syncInterfaceStates();
            }
        } else {
            // Mainly to populate the list
            NetworkManager::instance()->enableInterface();
        }

        if (!NetworkManager::instance()->isNetworkingEnabled()) {
            NetworkManager::instance()->enableNetworking();
        }

        if (NetworkManager::instance()->isConfigured()) {
            NetworkManager::instance()->activateConnections();
        }
    }());
}

void network_disconnect(network_handle_t network, uint32_t reason, void* reserved) {
    SYSTEM_THREAD_CONTEXT_ASYNC_CALL([network]() {
        if (network == NETWORK_INTERFACE_ALL) {
            cloud_disconnect(true, false, CLOUD_DISCONNECT_REASON_NETWORK_DISCONNECT);
            SPARK_WLAN_STARTED = 0;
            s_forcedDisconnect = true;
        }

        if (network != NETWORK_INTERFACE_ALL) {
            if_t iface;
            if (!if_get_by_index(network, &iface)) {
                NetworkManager::instance()->disableInterface(iface);
                NetworkManager::instance()->syncInterfaceStates();
            }
        } else {
            // Mainly to populate the list
            NetworkManager::instance()->disableInterface();
        }

        if ((network == NETWORK_INTERFACE_ALL ||
                NetworkManager::instance()->countEnabledInterfaces() == 0) &&
                !NetworkManager::instance()->deactivateConnections()) {
            /* FIXME: should not loop in here */
            while (NetworkManager::instance()->getState() != NetworkManager::State::IFACE_DOWN) {
                HAL_Delay_Milliseconds(1);
            }
        }
    }());
}

bool network_ready(network_handle_t network, uint32_t type, void* reserved) {
    if (network == NETWORK_INTERFACE_ALL) {
        if ((network_ready_type)type == NETWORK_READY_TYPE_ANY) {
            return NetworkManager::instance()->isConnectivityAvailable();
        } else {
           if ((network_ready_type)type == NETWORK_READY_TYPE_IPV4) {
                return NetworkManager::instance()->isIp4ConnectivityAvailable();
            } else if ((network_ready_type)type == NETWORK_READY_TYPE_IPV6) {
                return NetworkManager::instance()->isIp6ConnectivityAvailable();
            }
        }
    } else {
        if_t iface;
        if (!if_get_by_index(network, &iface)) {
            if (NetworkManager::instance()->isInterfaceEnabled(iface)) {
                auto ip4 = NetworkManager::instance()->getInterfaceIp4State(iface);
                auto ip6 = NetworkManager::instance()->getInterfaceIp6State(iface);
                if ((network_ready_type)type == NETWORK_READY_TYPE_ANY) {
                    if (network != NETWORK_INTERFACE_MESH) {
                        if (ip4 == NetworkManager::ProtocolState::CONFIGURED || ip6 == NetworkManager::ProtocolState::CONFIGURED) {
                            return true;
                        }
                    } else {
                        if (ip4 != NetworkManager::ProtocolState::UNCONFIGURED || ip6 != NetworkManager::ProtocolState::UNCONFIGURED) {
                            return true;
                        }
                    }
                } else {
                    if ((network_ready_type)type == NETWORK_READY_TYPE_IPV4) {
                        return ip4 == NetworkManager::ProtocolState::CONFIGURED;
                    } else if ((network_ready_type)type == NETWORK_READY_TYPE_IPV6) {
                        return ip6 == NetworkManager::ProtocolState::CONFIGURED;
                    }
                }
            }
        }
    }

    return false;
}

bool network_connecting(network_handle_t network, uint32_t param, void* reserved) {
    if (network == NETWORK_INTERFACE_ALL) {
        return NetworkManager::instance()->isEstablishingConnections();
    } else {
        if_t iface;
        if (!if_get_by_index(network, &iface)) {
            if (NetworkManager::instance()->isConnectivityAvailable() ||
                    NetworkManager::instance()->isEstablishingConnections()) {
                return !network_ready(network, param, reserved);
            }
        }
    }
    return false;
}

int network_connect_cancel(network_handle_t network, uint32_t flags, uint32_t param1, void* reserved) {
    return 0;
}

void network_on(network_handle_t network, uint32_t flags, uint32_t param, void* reserved) {
    SYSTEM_THREAD_CONTEXT_ASYNC_CALL([&]() {
        if (network != NETWORK_INTERFACE_ALL) {
            if_t iface;
            if (!if_get_by_index(network, &iface)) {
                if_req_power req = {};
                req.state = IF_POWER_STATE_UP;
                if_request(iface, IF_REQ_POWER_STATE, &req, sizeof(req), nullptr);
            }
        }

        NetworkManager::instance()->enableNetworking();
        SPARK_WLAN_STARTED = 1;
        SPARK_WLAN_SLEEP = 0;
    }());
}

bool network_has_credentials(network_handle_t network, uint32_t param, void* reserved) {
    if (network == NETWORK_INTERFACE_ALL) {
        return NetworkManager::instance()->isConfigured();
    } else {
        if_t iface;
        if (!if_get_by_index(network, &iface)) {
            return NetworkManager::instance()->isConfigured(iface);
        }
    }

    return false;
}

void network_off(network_handle_t network, uint32_t flags, uint32_t param, void* reserved) {
    /* flags & 1 means also disconnect the cloud
     * (so it doesn't autmatically connect when network resumed)
     */
    SYSTEM_THREAD_CONTEXT_ASYNC_CALL([&]() {
        if (network != NETWORK_INTERFACE_ALL) {
            network_disconnect(network, 0, 0);
            if_t iface;
            if (!if_get_by_index(network, &iface)) {
                if_req_power req = {};
                req.state = IF_POWER_STATE_DOWN;
                if_request(iface, IF_REQ_POWER_STATE, &req, sizeof(req), nullptr);
            }
        }

        if (network == NETWORK_INTERFACE_ALL || NetworkManager::instance()->countEnabledInterfaces() == 0) {
            SPARK_WLAN_STARTED = 0;
            SPARK_WLAN_SLEEP = 1;
            network_disconnect(0, 0, 0);
            /* FIXME: should not loop in here */
            NetworkManager::instance()->disableNetworking();

            if (flags & 1) {
                spark_cloud_flag_disconnect();
            }
        }
    }());
}

void network_listen(network_handle_t network, uint32_t flags, void*) {
    /* May be called from an ISR */
    if (!HAL_IsISR()) {
        SYSTEM_THREAD_CONTEXT_ASYNC_CALL([&]() {
            if (!(flags & NETWORK_LISTEN_EXIT)) {
                ListeningModeHandler::instance()->enter();
            } else {
                ListeningModeHandler::instance()->exit();
            }
        }());
    } else {
        if (!(flags & NETWORK_LISTEN_EXIT)) {
            ListeningModeHandler::instance()->enqueueCommand(NETWORK_LISTEN_COMMAND_ENTER, nullptr);
        } else {
            ListeningModeHandler::instance()->enqueueCommand(NETWORK_LISTEN_COMMAND_EXIT, nullptr);
        }
    }
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
    return ListeningModeHandler::instance()->isActive();
}

int network_listen_command(network_handle_t network, network_listen_command_t command, void* arg) {
    if (!HAL_IsISR()) {
        return ListeningModeHandler::instance()->command(command, arg);
    }

    return ListeningModeHandler::instance()->enqueueCommand(command, arg);
}

int network_set_credentials(network_handle_t network, uint32_t, NetworkCredentials* credentials, void*) {
    switch (network) {
#if HAL_PLATFORM_WIFI
    case NETWORK_INTERFACE_WIFI_STA: {
        auto r = wlan_set_credentials(credentials);
        if (!r) {
            system_notify_event(network_credentials, network_credentials_added, credentials);
        }
        return r;
    }
#endif
    default:
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
}

bool network_clear_credentials(network_handle_t network, uint32_t, NetworkCredentials* creds, void*) {
    if (network == NETWORK_INTERFACE_ALL) {
        return NetworkManager::instance()->clearConfiguration() == 0;
    } else {
        if_t iface;
        if (!if_get_by_index(network, &iface)) {
            return NetworkManager::instance()->clearConfiguration(iface);
        }
    }
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

/**
 * Reset or initialize the network connection as required.
 */
void manage_network_connection() {
    if (ListeningModeHandler::instance()->isActive()) {
        /* Nothing to do, we are in listening mode */
        return;
    }

    /* Need to disconnect and disable networking */
    /* FIXME: refactor */
    if (SPARK_WLAN_RESET || SPARK_WLAN_SLEEP) {
        auto wasSleeping = SPARK_WLAN_SLEEP;
        cloud_disconnect();
        network_disconnect(0, SPARK_WLAN_RESET ? NETWORK_DISCONNECT_REASON_RESET : NETWORK_DISCONNECT_REASON_NONE, 0);
        network_off(0, 0, 0, 0);
        SPARK_WLAN_RESET = 0;
        SPARK_WLAN_SLEEP = wasSleeping;
        cfod_count = 0;
    } else {
        if (spark_cloud_flag_auto_connect() && (!s_forcedDisconnect || !SPARK_WLAN_SLEEP)) {
            if (!NetworkManager::instance()->isConnectivityAvailable() && !NetworkManager::instance()->isEstablishingConnections()) {
                network_connect(0, 0, 0, 0);
            }
        }
    }
}

void manage_smart_config() {
    ListeningModeHandler::instance()->run();
}

void manage_ip_config() {
}

#endif /* HAL_PLATFORM_IFAPI */
