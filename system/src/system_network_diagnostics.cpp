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

#include "logging.h"
LOG_SOURCE_CATEGORY("system.nd")

#undef LOG_COMPILE_TIME_LEVEL
#define LOG_COMPILE_TIME_LEVEL LOG_LEVEL_ALL

#include "platforms.h"

#include <string.h>

#include "cellular_hal.h"
#include "check.h"
#include "spark_wiring_diagnostics.h"
#include "spark_wiring_fixed_point.h"
#include "spark_wiring_network.h"
#include "spark_wiring_platform.h"
#include "spark_wiring_ticks.h"
#include "system_network_diagnostics.h"
#include "spark_wiring_udp.h"

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

NetIfTester::NetIfTester() {
    network_interface_t interfaceList[] = { 
        NETWORK_INTERFACE_ETHERNET,
#if HAL_PLATFORM_CELLULAR
        NETWORK_INTERFACE_CELLULAR,
#endif
#if HAL_PLATFORM_WIFI 
        NETWORK_INTERFACE_WIFI_STA 
#endif
    };
    
    for (const auto& i: interfaceList) {
        struct NetIfDiagnostics interfaceDiagnostics = {};
        interfaceDiagnostics.interface = i;
        ifDiagnostics_.append(interfaceDiagnostics);
    }
}

NetIfTester* NetIfTester::instance() {
    static NetIfTester* tester = new NetIfTester();
    return tester;
}

void NetIfTester::testInterfaces() {
    // GOAL: To maintain a list of which network interface is "best" at any given time
    for (auto& i: ifDiagnostics_) {
        testInterface(&i);
    }
}

int NetIfTester::testInterface(NetIfDiagnostics* diagnostics) {     
    auto network = spark::Network.from(diagnostics->interface);
    if (!network) {
        return SYSTEM_ERROR_NONE;
    }

    // TODO: Make this more CoAP like test message
    uint8_t udpTxBuffer[128] = {'H', 'e', 'l', 'l', 'o', 0};
    uint8_t udpRxBuffer[128] = {};

    uint8_t beginResult = 0;
    int sendResult, receiveResult = -1;
    IPAddress echoServer;

    diagnostics->dnsResolutionAttempts++;
    echoServer = network.resolve(UDP_ECHO_SERVER_HOSTNAME);
    if (!echoServer) {
        diagnostics->dnsResolutionFailures++;
        LOG(INFO, "IF #%d failed to resolve DNS for %s : %s", diagnostics->interface, UDP_ECHO_SERVER_HOSTNAME, echoServer.toString().c_str());
        return SYSTEM_ERROR_PROTOCOL;
    }

    UDP udpInstance = UDP();
    // TODO: What error conditions are the on UDP socket bind (ie the begin call here)?
    beginResult = udpInstance.begin(NetIfTester::UDP_ECHO_PORT, diagnostics->interface);
    
    LOG(INFO, "Testing IF #%d with %s : %s", diagnostics->interface, UDP_ECHO_SERVER_HOSTNAME, echoServer.toString().c_str());

    // TODO: Error conditions on sock_sendto
    // TODO: start packet rount trip timer
    sendResult = udpInstance.sendPacket(udpTxBuffer, strlen((char*)udpTxBuffer), echoServer, UDP_ECHO_PORT);
    if (sendResult > 0) {
        diagnostics->txBytes += strlen((char*)udpTxBuffer);
    }

    receiveResult = udpInstance.receivePacket(udpRxBuffer, strlen((char*)udpTxBuffer), 5000);
    if (receiveResult > 0) {
        diagnostics->rxBytes += strlen((char*)udpRxBuffer);
    }

    LOG(INFO, "UDP begin %d send: %d receive: %d message: %s", beginResult, sendResult, receiveResult, (char*)udpRxBuffer);
    udpInstance.stop();

    return SYSTEM_ERROR_NONE;
}

const Vector<NetIfDiagnostics>* NetIfTester::getDiagnostics(){
    return &ifDiagnostics_;
}


namespace
{

using namespace particle;
using namespace spark;

class NetworkCache
{
public:
    static const system_tick_t NETWORK_INFO_CACHE_INTERVAL = 1000;

    const Signal* getSignal()
    {
#if Wiring_WiFi || Wiring_Cellular
        system_tick_t m = millis();
        if (ts_ == 0 || (m - ts_) >= NETWORK_INFO_CACHE_INTERVAL)
        {
            ts_ = millis();
#if Wiring_Cellular
            // TODO: Better concept of which Network class is being used for the cloud connection
            // And/or include both cellular + wifi signal strength in diagnostics/vitals
            if (Cellular.ready()) {
                cellularSig_ = Cellular.RSSI();
                return &cellularSig_;    
            }
#endif
#if Wiring_WiFi && !HAL_PLATFORM_WIFI_SCAN_ONLY
            if (WiFi.ready()) {
                wifiSig_ = WiFi.RSSI();
                return &wifiSig_;    
            }
#endif
        }
#endif
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
} // namespace

#endif // Wiring_Network

