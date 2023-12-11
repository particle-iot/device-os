/*
 * Copyright (c) 2019 Particle Industries, Inc.  All rights reserved.
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

#include "platforms.h"

#include <string.h>

#include "cellular_hal.h"
#include "check.h"
#include "spark_wiring_diagnostics.h"
#include "spark_wiring_fixed_point.h"
#include "spark_wiring_platform.h"
#include "spark_wiring_ticks.h"
#include "system_network_diagnostics.h"
#include "system_connection_manager.h"

#if Wiring_WiFi
#include "spark_wiring_wifi.h"
#include "system_network_wifi.h"
#define Wiring_Network 1
#endif

#if Wiring_Cellular
#include "spark_wiring_cellular.h"
#include "system_network_cellular.h"
#define Wiring_Network 1
#endif

#ifndef Wiring_Network
#define Wiring_Network 0
#else

namespace {

using namespace particle;

NetworkDiagnostics g_networkDiagnostics;

} // namespace

particle::NetworkDiagnostics* particle::NetworkDiagnostics::instance() {
    return &g_networkDiagnostics;
}

namespace
{

using namespace particle;
using namespace spark;

class NetworkCache
{
public:
    static const system_tick_t NETWORK_INFO_CACHE_INTERVAL = 1000;

    const Signal* getSignal(bool getPrimarySignal = true)
    {
        bool refreshCachedValues = false;
        system_tick_t m = millis();
        if (ts_ == 0 || (m - ts_) >= NETWORK_INFO_CACHE_INTERVAL) {
            ts_ = millis();
            refreshCachedValues = true;
        }

#if HAL_PLATFORM_AUTOMATIC_CONNECTION_MANAGEMENT
        auto cloudNetwork = Network.from(system::ConnectionManager::instance()->getCloudConnectionNetwork());
        
        if (refreshCachedValues) {
            cellularSig_ = Cellular.RSSI();
            wifiSig_ = WiFi.RSSI();
        } 

        const Signal* primarySignal = &cellularSig_;
        const Signal* alternateSignal = &wifiSig_;

        if (cloudNetwork == WiFi) {
            primarySignal = &wifiSig_;
            alternateSignal = &cellularSig_;
        }

        if (getPrimarySignal) {
            return primarySignal;
        } else {
            return alternateSignal;
        }
#else

#if Wiring_Cellular
        if (refreshCachedValues) {
            cellularSig_ = Cellular.RSSI();    
        }
        return &cellularSig_;
#endif // Wiring_Cellular

#if Wiring_WiFi && !HAL_PLATFORM_WIFI_SCAN_ONLY
        if (refreshCachedValues) {
            wifiSig_ = WiFi.RSSI();
        }
        return &wifiSig_;    
#endif // Wiring_WiFi && !HAL_PLATFORM_WIFI_SCAN_ONLY

#endif // HAL_PLATFORM_AUTOMATIC_CONNECTION_MANAGEMENT
        return nullptr;
    }

#if HAL_PLATFORM_CELLULAR
    cellular_result_t getCGI(CellularGlobalIdentity& cgi)
    {
        cellular_result_t result = SYSTEM_ERROR_NONE;
        system_tick_t ms = millis();

        if (cgi_ts_ == 0 || (ms - cgi_ts_) >= NETWORK_INFO_CACHE_INTERVAL)
        {
            // Update cache
            cgi_.size = sizeof(CellularGlobalIdentity);
            cgi_.version = CGI_VERSION_LATEST;
            result = CHECK(cellular_global_identity(&cgi_, nullptr));
            cgi_ts_ = millis();
        }

        // Return cached values
        cgi = cgi_;
        return result;
    }
#endif

private:
#if Wiring_Cellular
    CellularSignal cellularSig_;
    CellularGlobalIdentity cgi_ = {};
#endif
#if Wiring_WiFi && !HAL_PLATFORM_WIFI_SCAN_ONLY
    WiFiSignal wifiSig_;
#endif
    system_tick_t ts_ = 0;

#if HAL_PLATFORM_CELLULAR
    system_tick_t cgi_ts_ = 0;
#endif
};

static NetworkCache s_networkCache;

class SignalStrengthDiagnosticData : public AbstractIntegerDiagnosticData
{
public:
    SignalStrengthDiagnosticData()
        : AbstractIntegerDiagnosticData(DIAG_ID_NETWORK_SIGNAL_STRENGTH,
                                        DIAG_NAME_NETWORK_SIGNAL_STRENGTH)
    {
    }

    virtual int get(IntType& val)
    {
        const Signal* sig = s_networkCache.getSignal();
        if (sig == nullptr)
        {
            return SYSTEM_ERROR_NOT_SUPPORTED;
        }

        if (sig->getStrength() < 0)
        {
            return SYSTEM_ERROR_UNKNOWN;
        }

        // Convert to unsigned Q8.8
        FixedPointUQ<8, 8> str(sig->getStrength());
        val = str;

        return SYSTEM_ERROR_NONE;
    }
} g_signalStrengthDiagData;

class SignalStrengthValueDiagnosticData : public AbstractIntegerDiagnosticData
{
public:
    SignalStrengthValueDiagnosticData()
        : AbstractIntegerDiagnosticData(DIAG_ID_NETWORK_SIGNAL_STRENGTH_VALUE,
                                        DIAG_NAME_NETWORK_SIGNAL_STRENGTH_VALUE)
    {
    }

    virtual int get(IntType& val)
    {
        const Signal* sig = s_networkCache.getSignal();
        if (sig == nullptr)
        {
            return SYSTEM_ERROR_NOT_SUPPORTED;
        }

        if (sig->getStrength() < 0)
        {
            return SYSTEM_ERROR_UNKNOWN;
        }

        // Convert to signed Q16.16
        FixedPointSQ<16, 16> str(sig->getStrengthValue());
        val = str;

        return SYSTEM_ERROR_NONE;
    }
} g_signalStrengthValueDiagData;

class SignalQualityDiagnosticData : public AbstractIntegerDiagnosticData
{
public:
    SignalQualityDiagnosticData()
        : AbstractIntegerDiagnosticData(DIAG_ID_NETWORK_SIGNAL_QUALITY,
                                        DIAG_NAME_NETWORK_SIGNAL_QUALITY)
    {
    }

    virtual int get(IntType& val)
    {
        const Signal* sig = s_networkCache.getSignal();
        if (sig == nullptr)
        {
            return SYSTEM_ERROR_NOT_SUPPORTED;
        }

        if (sig->getQuality() < 0)
        {
            return SYSTEM_ERROR_UNKNOWN;
        }

        // Convert to unsigned Q8.8
        FixedPointUQ<8, 8> str(sig->getQuality());
        val = str;

        return SYSTEM_ERROR_NONE;
    }
} g_signalQualityDiagData;

class SignalQualityValueDiagnosticData : public AbstractIntegerDiagnosticData
{
public:
    SignalQualityValueDiagnosticData()
        : AbstractIntegerDiagnosticData(DIAG_ID_NETWORK_SIGNAL_QUALITY_VALUE,
                                        DIAG_NAME_NETWORK_SIGNAL_QUALITY_VALUE)
    {
    }

    virtual int get(IntType& val)
    {
        const Signal* sig = s_networkCache.getSignal();
        if (sig == nullptr)
        {
            return SYSTEM_ERROR_NOT_SUPPORTED;
        }

        if (sig->getQuality() < 0)
        {
            return SYSTEM_ERROR_UNKNOWN;
        }

        // Convert to signed Q16.16
        FixedPointSQ<16, 16> str(sig->getQualityValue());
        val = str;

        return SYSTEM_ERROR_NONE;
    }
} g_signalQualityValueDiagData;

class NetworkAccessTechnologyDiagnosticData : public AbstractIntegerDiagnosticData
{
public:
    NetworkAccessTechnologyDiagnosticData()
        : AbstractIntegerDiagnosticData(DIAG_ID_NETWORK_ACCESS_TECNHOLOGY,
                                        DIAG_NAME_NETWORK_ACCESS_TECNHOLOGY)
    {
    }

    virtual int get(IntType& val)
    {
        const Signal* sig = s_networkCache.getSignal();
        if (sig == nullptr)
        {
            return SYSTEM_ERROR_NOT_SUPPORTED;
        }

        val = static_cast<IntType>(sig->getAccessTechnology());

        return SYSTEM_ERROR_NONE;
    }
} g_networkAccessTechnologyDiagData;


#if HAL_PLATFORM_AUTOMATIC_CONNECTION_MANAGEMENT

class CloudConnectionInterfaceDiagnosticData : public EnumDiagnosticData< NetworkDiagnosticsInterface, NoConcurrency>
{
public:
    CloudConnectionInterfaceDiagnosticData()
        : EnumDiagnosticData(DIAG_ID_CLOUD_CONNECTION_INTERFACE,
                            DIAG_NAME_CLOUD_CONNECTION_INTERFACE,
                            NetworkDiagnosticsInterface::ALL)
    {
    }

    virtual int get(IntType& val)
    {
        auto netIf = NetworkDiagnosticsInterface::ALL;
        switch(system::ConnectionManager::instance()->getCloudConnectionNetwork()) {
#if HAL_PLATFORM_ETHERNET
            case NETWORK_INTERFACE_ETHERNET:
                netIf = NetworkDiagnosticsInterface::ETHERNET;
                break;
#endif
#if HAL_PLATFORM_CELLULAR
            case NETWORK_INTERFACE_CELLULAR:
                netIf = NetworkDiagnosticsInterface::CELLULAR;
                break;
#endif
#if HAL_PLATFORM_WIFI
            case NETWORK_INTERFACE_WIFI_STA:
                netIf = NetworkDiagnosticsInterface::WIFI_STA;
                break;
#endif
            default:
                break;
        }

        val = static_cast<IntType>(netIf);
        return SYSTEM_ERROR_NONE;
    }
} g_cloudConnectionInterfaceDiagData;

#endif // HAL_PLATFORM_AUTOMATIC_CONNECTION_MANAGEMENT

#if HAL_PLATFORM_CELLULAR
class NetworkCellularCellGlobalIdentityMobileCountryCodeDiagnosticData
    : public AbstractIntegerDiagnosticData
{
public:
    NetworkCellularCellGlobalIdentityMobileCountryCodeDiagnosticData()
        : AbstractIntegerDiagnosticData(
              DIAG_ID_NETWORK_CELLULAR_CELL_GLOBAL_IDENTITY_MOBILE_COUNTRY_CODE,
              DIAG_NAME_NETWORK_CELLULAR_CELL_GLOBAL_IDENTITY_MOBILE_COUNTRY_CODE)
    {
    }

    virtual int get(IntType& val)
    {
        cellular_result_t result;
        CellularGlobalIdentity cgi; // Intentionally left uninitialized
        result = CHECK(s_networkCache.getCGI(cgi));
        val = static_cast<IntType>(cgi.mobile_country_code);

        return result;
    }
} g_networkCellularCellGlobalIdentityMobileCountryCodeDiagnosticData;

class NetworkCellularCellGlobalIdentityMobileNetworkCodeDiagnosticData
    : public AbstractIntegerDiagnosticData
{
public:
    NetworkCellularCellGlobalIdentityMobileNetworkCodeDiagnosticData()
        : AbstractIntegerDiagnosticData(
              DIAG_ID_NETWORK_CELLULAR_CELL_GLOBAL_IDENTITY_MOBILE_NETWORK_CODE,
              DIAG_NAME_NETWORK_CELLULAR_CELL_GLOBAL_IDENTITY_MOBILE_NETWORK_CODE)
    {
    }

    virtual int get(IntType& val)
    {
        cellular_result_t result;
        CellularGlobalIdentity cgi; // Intentionally left uninitialized
        result = CHECK(s_networkCache.getCGI(cgi));
        if (CGI_FLAG_TWO_DIGIT_MNC & cgi.cgi_flags)
        {
            val = static_cast<IntType>(cgi.mobile_network_code * -1);
        }
        else
        {
            val = static_cast<IntType>(cgi.mobile_network_code);
        }

        return result;
    }
} g_networkCellularCellGlobalIdentityMobileNetworkCodeDiagnosticData;

class NetworkCellularCellGlobalIdentityLocationAreaCodeDiagnosticData
    : public AbstractIntegerDiagnosticData
{
public:
    NetworkCellularCellGlobalIdentityLocationAreaCodeDiagnosticData()
        : AbstractIntegerDiagnosticData(
              DIAG_ID_NETWORK_CELLULAR_CELL_GLOBAL_IDENTITY_LOCATION_AREA_CODE,
              DIAG_NAME_NETWORK_CELLULAR_CELL_GLOBAL_IDENTITY_LOCATION_AREA_CODE)
    {
    }

    virtual int get(IntType& val)
    {
        cellular_result_t result;
        CellularGlobalIdentity cgi; // Intentionally left uninitialized
        result = CHECK(s_networkCache.getCGI(cgi));
        val = static_cast<IntType>(cgi.location_area_code);

        return result;
    }
} g_networkCellularCellGlobalIdentityLocationAreaCodeDiagnosticData;

class NetworkCellularCellGlobalIdentityCellIdDiagnosticData : public AbstractIntegerDiagnosticData
{
public:
    NetworkCellularCellGlobalIdentityCellIdDiagnosticData()
        : AbstractIntegerDiagnosticData(DIAG_ID_NETWORK_CELLULAR_CELL_GLOBAL_IDENTITY_CELL_ID,
                                        DIAG_NAME_NETWORK_CELLULAR_CELL_GLOBAL_IDENTITY_CELL_ID)
    {
    }

    virtual int get(IntType& val)
    {
        cellular_result_t result;
        CellularGlobalIdentity cgi; // Intentionally left uninitialized
        result = CHECK(s_networkCache.getCGI(cgi));
        val = static_cast<IntType>(cgi.cell_id);

        return result;
    }
} g_networkCellularCellGlobalIdentityCellIdDiagnosticData;
#endif // HAL_PLATFORM_CELLULAR

#if HAL_PLATFORM_AUTOMATIC_CONNECTION_MANAGEMENT
// FIXME: Fix the int types when making this recognized data on the server side
class AltSignalStrengthDiagnosticData : public AbstractIntegerDiagnosticData
{
public:
    AltSignalStrengthDiagnosticData()
        : AbstractIntegerDiagnosticData(DIAG_ID_ALT_NETWORK_SIGNAL_STRENGTH,
                                        DIAG_NAME_ALT_NETWORK_SIGNAL_STRENGTH)
    {
    }

    virtual int get(IntType& val)
    {
        const Signal* sig = s_networkCache.getSignal(false);
        if (sig == nullptr)
        {
            return SYSTEM_ERROR_NOT_SUPPORTED;
        }

        if (sig->getStrength() < 0)
        {
            return SYSTEM_ERROR_UNKNOWN;
        }

        // Convert to unsigned Q8.8
        FixedPointUQ<8, 8> str(sig->getStrength());
        val = str;

        return SYSTEM_ERROR_NONE;
    }
} g_altSignalStrengthDiagData;

class AltSignalStrengthValueDiagnosticData : public AbstractIntegerDiagnosticData
{
public:
    AltSignalStrengthValueDiagnosticData()
        : AbstractIntegerDiagnosticData(DIAG_ID_ALT_NETWORK_SIGNAL_STRENGTH_VALUE,
                                        DIAG_NAME_ALT_NETWORK_SIGNAL_STRENGTH_VALUE)
    {
    }

    virtual int get(IntType& val)
    {
        const Signal* sig = s_networkCache.getSignal(false);
        if (sig == nullptr)
        {
            return SYSTEM_ERROR_NOT_SUPPORTED;
        }

        if (sig->getStrength() < 0)
        {
            return SYSTEM_ERROR_UNKNOWN;
        }

        // Convert to signed Q16.16
        FixedPointSQ<16, 16> str(sig->getStrengthValue());
        val = str;

        return SYSTEM_ERROR_NONE;
    }
} g_altSignalStrengthValueDiagData;

class AltSignalQualityDiagnosticData : public AbstractIntegerDiagnosticData
{
public:
    AltSignalQualityDiagnosticData()
        : AbstractIntegerDiagnosticData(DIAG_ID_ALT_NETWORK_SIGNAL_QUALITY,
                                        DIAG_NAME_ALT_NETWORK_SIGNAL_QUALITY)
    {
    }

    virtual int get(IntType& val)
    {
        const Signal* sig = s_networkCache.getSignal(false);
        if (sig == nullptr)
        {
            return SYSTEM_ERROR_NOT_SUPPORTED;
        }

        if (sig->getQuality() < 0)
        {
            return SYSTEM_ERROR_UNKNOWN;
        }

        // Convert to unsigned Q8.8
        FixedPointUQ<8, 8> str(sig->getQuality());
        val = str;

        return SYSTEM_ERROR_NONE;
    }
} g_altSignalQualityDiagData;

class AltSignalQualityValueDiagnosticData : public AbstractIntegerDiagnosticData
{
public:
    AltSignalQualityValueDiagnosticData()
        : AbstractIntegerDiagnosticData(DIAG_ID_ALT_NETWORK_SIGNAL_QUALITY_VALUE,
                                        DIAG_NAME_ALT_NETWORK_SIGNAL_QUALITY_VALUE)
    {
    }

    virtual int get(IntType& val)
    {
        const Signal* sig = s_networkCache.getSignal(false);
        if (sig == nullptr)
        {
            return SYSTEM_ERROR_NOT_SUPPORTED;
        }

        if (sig->getQuality() < 0)
        {
            return SYSTEM_ERROR_UNKNOWN;
        }

        // Convert to signed Q16.16
        FixedPointSQ<16, 16> str(sig->getQualityValue());
        val = str;

        return SYSTEM_ERROR_NONE;
    }
} g_altSignalQualityValueDiagData;

class AltNetworkAccessTechnologyDiagnosticData : public AbstractIntegerDiagnosticData
{
public:
    AltNetworkAccessTechnologyDiagnosticData()
        : AbstractIntegerDiagnosticData(DIAG_ID_ALT_NETWORK_ACCESS_TECNHOLOGY,
                                        DIAG_NAME_ALT_NETWORK_ACCESS_TECNHOLOGY)
    {
    }

    virtual int get(IntType& val)
    {
        const Signal* sig = s_networkCache.getSignal(false);
        if (sig == nullptr)
        {
            return SYSTEM_ERROR_NOT_SUPPORTED;
        }

        val = static_cast<IntType>(sig->getAccessTechnology());

        return SYSTEM_ERROR_NONE;
    }
} g_altNetworkAccessTechnologyDiagData;
#endif // HAL_PLATFORM_AUTOMATIC_CONNECTION_MANAGEMENT

} // namespace

#endif // Wiring_Network

