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

#include "spark_wiring_ticks.h"
#include "spark_wiring_diagnostics.h"
#include "system_setup.h"
#include "system_network.h"
#include "system_network_internal.h"
#include "system_cloud.h"
#include "system_event.h"
#include "system_threading.h"
#include "watchdog_hal.h"
#include "wlan_hal.h"
#include "delay_hal.h"
#include "rgbled.h"
#include <string.h>
#include "spark_wiring_fixed_point.h"

uint32_t wlan_watchdog_base;
uint32_t wlan_watchdog_duration;

volatile uint8_t SPARK_WLAN_RESET;
volatile uint8_t SPARK_WLAN_SLEEP;
volatile uint8_t SPARK_WLAN_STARTED;


#if Wiring_WiFi
#include "system_network_wifi.h"
#include "spark_wiring_wifi.h"
WiFiNetworkInterface wifi;
ManagedNetworkInterface& network = wifi;
inline NetworkInterface& nif(network_interface_t _nif) { return wifi; }
#define Wiring_Network 1
#endif

#if Wiring_Cellular
#include "system_network_cellular.h"
#include "spark_wiring_cellular.h"
CellularNetworkInterface cellular;
ManagedNetworkInterface& network = cellular;
inline NetworkInterface& nif(network_interface_t _nif) { return cellular; }
#define Wiring_Network 1
#endif

#ifndef Wiring_Network
#define Wiring_Network 0
#endif

#if Wiring_Network

namespace {

using namespace particle;
using namespace spark;

static const system_tick_t SIGNAL_INFO_CACHE_INTERVAL = 1000;

class SignalCache {
public:
    const Signal* getSignal() {
#if Wiring_WiFi || Wiring_Cellular
        system_tick_t m = millis();
        if (ts_ == 0 || (m - ts_) >= SIGNAL_INFO_CACHE_INTERVAL) {
# if Wiring_WiFi
            sig_ = WiFi.RSSI();
# elif Wiring_Cellular
            sig_ = Cellular.RSSI();
# endif
            ts_ = millis();
        }
        return &sig_;
#else
        return nullptr;
#endif
    }
private:
#if Wiring_Cellular
    CellularSignal sig_;
#elif Wiring_WiFi
    WiFiSignal sig_;
#endif
    system_tick_t ts_ = 0;
};

static SignalCache s_signalCache;

class SignalStrengthDiagnosticData: public AbstractIntegerDiagnosticData {
public:
    SignalStrengthDiagnosticData() :
            AbstractIntegerDiagnosticData(DIAG_ID_NETWORK_SIGNAL_STRENGTH, DIAG_NAME_NETWORK_SIGNAL_STRENGTH) {
    }

    virtual int get(IntType& val) {
        const Signal* sig = s_signalCache.getSignal();
        if (sig == nullptr) {
            return SYSTEM_ERROR_NOT_SUPPORTED;
        }

        if (sig->getStrength() < 0) {
            return SYSTEM_ERROR_UNKNOWN;
        }

        // Convert to unsigned Q8.8
        FixedPointUQ<8, 8> str(sig->getStrength());
        val = str;

        return SYSTEM_ERROR_NONE;
    }
};

class NetworkRssiDiagnosticData: public AbstractIntegerDiagnosticData {
public:
    NetworkRssiDiagnosticData() :
            AbstractIntegerDiagnosticData(DIAG_ID_NETWORK_RSSI, DIAG_NAME_NETWORK_RSSI) {
    }

    virtual int get(IntType& val) {
        const Signal* sig = s_signalCache.getSignal();
        if (sig == nullptr) {
            return SYSTEM_ERROR_NOT_SUPPORTED;
        }

        if (sig->getStrength() < 0) {
            return SYSTEM_ERROR_UNKNOWN;
        }

        // Convert to signed Q8.8
        FixedPointSQ<8, 8> str(sig->getStrengthValue());
        val = str;

        return SYSTEM_ERROR_NONE;
    }
};

class SignalStrengthValueDiagnosticData: public AbstractIntegerDiagnosticData {
public:
    SignalStrengthValueDiagnosticData() :
            AbstractIntegerDiagnosticData(DIAG_ID_NETWORK_SIGNAL_STRENGTH_VALUE, DIAG_NAME_NETWORK_SIGNAL_STRENGTH_VALUE) {
    }

    virtual int get(IntType& val) {
        const Signal* sig = s_signalCache.getSignal();
        if (sig == nullptr) {
            return SYSTEM_ERROR_NOT_SUPPORTED;
        }

        if (sig->getStrength() < 0) {
            return SYSTEM_ERROR_UNKNOWN;
        }

        // Convert to signed Q16.16
        FixedPointSQ<16, 16> str(sig->getStrengthValue());
        val = str;

        return SYSTEM_ERROR_NONE;
    }
};

class SignalQualityDiagnosticData: public AbstractIntegerDiagnosticData {
public:
    SignalQualityDiagnosticData() :
            AbstractIntegerDiagnosticData(DIAG_ID_NETWORK_SIGNAL_QUALITY, DIAG_NAME_NETWORK_SIGNAL_QUALITY) {
    }

    virtual int get(IntType& val) {
        const Signal* sig = s_signalCache.getSignal();
        if (sig == nullptr) {
            return SYSTEM_ERROR_NOT_SUPPORTED;
        }

        if (sig->getQuality() < 0) {
            return SYSTEM_ERROR_UNKNOWN;
        }

        // Convert to unsigned Q8.8
        FixedPointUQ<8, 8> str(sig->getQuality());
        val = str;

        return SYSTEM_ERROR_NONE;
    }
};

class SignalQualityValueDiagnosticData: public AbstractIntegerDiagnosticData {
public:
    SignalQualityValueDiagnosticData() :
            AbstractIntegerDiagnosticData(DIAG_ID_NETWORK_SIGNAL_QUALITY_VALUE, DIAG_NAME_NETWORK_SIGNAL_QUALITY_VALUE) {
    }

    virtual int get(IntType& val) {
        const Signal* sig = s_signalCache.getSignal();
        if (sig == nullptr) {
            return SYSTEM_ERROR_NOT_SUPPORTED;
        }

        if (sig->getQuality() < 0) {
            return SYSTEM_ERROR_UNKNOWN;
        }

        // Convert to signed Q16.16
        FixedPointSQ<16, 16> str(sig->getQualityValue());
        val = str;

        return SYSTEM_ERROR_NONE;
    }
};

class NetworkAccessTechnologyDiagnosticData: public AbstractIntegerDiagnosticData {
public:
    NetworkAccessTechnologyDiagnosticData() :
            AbstractIntegerDiagnosticData(DIAG_ID_NETWORK_ACCESS_TECNHOLOGY, DIAG_NAME_NETWORK_ACCESS_TECNHOLOGY) {
    }

    virtual int get(IntType& val) {
        const Signal* sig = s_signalCache.getSignal();
        if (sig == nullptr) {
            return SYSTEM_ERROR_NOT_SUPPORTED;
        }

        val = static_cast<IntType>(sig->getAccessTechnology());

        return SYSTEM_ERROR_NONE;
    }
};

SignalStrengthDiagnosticData g_signalStrengthDiagData;
SignalStrengthValueDiagnosticData g_signalStrengthValueDiagData;
SignalQualityDiagnosticData g_signalQualityDiagData;
SignalQualityValueDiagnosticData g_signalQualityValueDiagData;
NetworkAccessTechnologyDiagnosticData g_networkAccessTechnologyDiagData;
//
NetworkRssiDiagnosticData g_networkRssiDiagData;
//

} // namespace

void HAL_WLAN_notify_simple_config_done()
{
    network.notify_listening_complete();
}

void HAL_NET_notify_connected()
{
    network.notify_connected();
}

void HAL_NET_notify_disconnected()
{
    network.notify_disconnected();
}

void HAL_NET_notify_can_shutdown()
{
    network.notify_can_shutdown();
}

void HAL_NET_notify_dhcp(bool dhcp)
{
    network.notify_dhcp(dhcp);
}


const void* network_config(network_handle_t network, uint32_t param, void* reserved)
{
    return nif(network).config();
}

void network_connect(network_handle_t network, uint32_t flags, uint32_t param, void* reserved)
{
    SYSTEM_THREAD_CONTEXT_ASYNC_CALL(nif(network).connect(!(flags & WIFI_CONNECT_SKIP_LISTEN)));
}

void network_disconnect(network_handle_t network, uint32_t reason, void* reserved)
{
	nif(network).connect_cancel(true);
    SYSTEM_THREAD_CONTEXT_ASYNC_CALL(nif(network).disconnect((network_disconnect_reason)reason));
}

bool network_ready(network_handle_t network, uint32_t param, void* reserved)
{
    return nif(network).ready();
}

bool network_connecting(network_handle_t network, uint32_t param, void* reserved)
{
    return nif(network).connecting();
}

/**
 *
 * @param network
 * @param flags    1 - don't change the LED color
 * @param param
 * @param reserved
 */
void network_on(network_handle_t network, uint32_t flags, uint32_t param, void* reserved)
{
    SYSTEM_THREAD_CONTEXT_ASYNC_CALL(nif(network).on());
}

bool network_has_credentials(network_handle_t network, uint32_t param, void* reserved)
{
    SYSTEM_THREAD_CONTEXT_SYNC_CALL_RESULT(nif(network).has_credentials());
}

void network_off(network_handle_t network, uint32_t flags, uint32_t param, void* reserved)
{
    nif(network).connect_cancel(true);
    // flags & 1 means also disconnect the cloud (so it doesn't autmatically connect when network resumed.)
    SYSTEM_THREAD_CONTEXT_ASYNC_CALL(nif(network).off(flags & 1));
}

/**
 *
 * @param network
 * @param flags  bit 0 set means to stop listening.
 * @param
 */
void network_listen(network_handle_t network, uint32_t flags, void*)
{
    const bool stop = flags & NETWORK_LISTEN_EXIT;
    // Set/clear listening mode flag
    nif(network).listen(stop);
    if (!stop) {
        // Cancel current connection attempt
        nif(network).connect_cancel(true);
    }
}

void network_set_listen_timeout(network_handle_t network, uint16_t timeout, void*)
{
    return nif(network).set_listen_timeout(timeout);
}

uint16_t network_get_listen_timeout(network_handle_t network, uint32_t flags, void*)
{
    return nif(network).get_listen_timeout();
}

bool network_listening(network_handle_t network, uint32_t, void*)
{
    return nif(network).listening();
}

int network_set_credentials(network_handle_t network, uint32_t, NetworkCredentials* credentials, void*)
{
    SYSTEM_THREAD_CONTEXT_SYNC_CALL_RESULT(nif(network).set_credentials(credentials));
}

bool network_clear_credentials(network_handle_t network, uint32_t, NetworkCredentials* creds, void*)
{
    SYSTEM_THREAD_CONTEXT_SYNC_CALL_RESULT(nif(network).clear_credentials());
}

void network_setup(network_handle_t network, uint32_t flags, void* reserved)
{
    SYSTEM_THREAD_CONTEXT_ASYNC_CALL(nif(network).setup());
}


// These are internal methods
void manage_smart_config()
{
    network.listen_loop();
}

void manage_ip_config()
{
    nif(0).update_config();
}

int network_set_hostname(network_handle_t network, uint32_t flags, const char* hostname, void* reserved)
{
    SYSTEM_THREAD_CONTEXT_SYNC_CALL_RESULT(nif(network).set_hostname(hostname));
}

int network_get_hostname(network_handle_t network, uint32_t flags, char* buffer, size_t buffer_len, void* reserved)
{
    SYSTEM_THREAD_CONTEXT_SYNC_CALL_RESULT(nif(network).get_hostname(buffer, buffer_len));
}

#endif
