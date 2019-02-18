/*
 ******************************************************************************
  Copyright (c) 2014-2015 Particle Industries, Inc.  All rights reserved.

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

#ifndef SPARK_WIRING_CELLULAR_H
#define	SPARK_WIRING_CELLULAR_H

#include "spark_wiring_platform.h"
#include "spark_wiring_network.h"
#include "system_network.h"

#if Wiring_Cellular

#include "cellular_hal.h"
#include "inet_hal.h"
#include "spark_wiring_cellular_printable.h"

namespace spark {

class CellularClass : public NetworkClass
{
    CellularDevice device;

public:
    CellularClass() :
            NetworkClass(NETWORK_INTERFACE_CELLULAR) {
    }

    IPAddress localIP() {
        return IPAddress(((CellularConfig*)network_config(*this, 0, NULL))->nw.aucIP);
    }
    void on() {
        network_on(*this, 0, 0, NULL);
    }
    void off() {
        network_off(*this, 0, 0, NULL);
    }
    void connect(unsigned flags=0) {
        network_connect(*this, flags, 0, NULL);
    }
    bool connecting(void) {
        return network_connecting(*this, 0, NULL);
    }

    void disconnect() {
        network_disconnect(*this, NETWORK_DISCONNECT_REASON_USER, NULL);
    }

    void setCredentials(const char* apn) {
        setCredentials(apn, "", "");
    }
    void setCredentials(const char* username, const char* password) {
        setCredentials("", username, password);
    }
    void setCredentials(const char* apn, const char* username, const char* password) {
        cellular_credentials_set(apn, username, password, nullptr);
    }
#if HAL_PLATFORM_MESH
// FIXME: there should be a separate macro to indicate that this functionality
// is available
    void clearCredentials() {
        cellular_credentials_clear(nullptr);
    }
#endif // HAL_PLATFORM_MESH

    void listen(bool begin=true) {
        network_listen(*this, begin ? 0 : 1, NULL);
    }

    void setListenTimeout(uint16_t timeout) {
        network_set_listen_timeout(*this, timeout, NULL);
    }

    uint16_t getListenTimeout(void) {
        return network_get_listen_timeout(*this, 0, NULL);
    }

    bool listening(void) {
        return network_listening(*this, 0, NULL);
    }

    bool ready()
    {
        return network_ready(*this, 0,  NULL);
    }

    CellularSignal RSSI();

    bool getDataUsage(CellularData &data_get);
    bool setDataUsage(CellularData &data_set);
    bool resetDataUsage(void);

    bool setBandSelect(const char* band);
    bool setBandSelect(CellularBand &data_set);
    bool getBandSelect(CellularBand &data_get);
    bool getBandAvailable(CellularBand &data_get);

    template<typename... Targs>
    inline int command(const char* format, Targs... Fargs)
    {
        return cellular_command(NULL, NULL, 10000, format, Fargs...);
    }

    template<typename... Targs>
    inline int command(system_tick_t timeout_ms, const char* format, Targs... Fargs)
    {
        return cellular_command(NULL, NULL, timeout_ms, format, Fargs...);
    }

    template<typename T, typename... Targs>
    inline int command(int (*cb)(int type, const char* buf, int len, T* param),
            T* param, const char* format, Targs... Fargs)
    {
        return cellular_command((_CALLBACKPTR_MDM)cb, (void*)param, 10000, format, Fargs...);
    }

    template<typename T, typename... Targs>
    inline int command(int (*cb)(int type, const char* buf, int len, T* param),
            T* param, system_tick_t timeout_ms, const char* format, Targs... Fargs)
    {
        return cellular_command((_CALLBACKPTR_MDM)cb, (void*)param, timeout_ms, format, Fargs...);
    }

#if !HAL_USE_INET_HAL_POSIX
    IPAddress resolve(const char* name)
    {
        HAL_IPAddress ip = {0};
        return (inet_gethostbyname(name, strlen(name), &ip, *this, NULL) != 0) ?
                IPAddress(uint32_t(0)) : IPAddress(ip);
    }
#endif // !HAL_USE_INET_HAL_POSIX

#if HAL_PLATFORM_MESH
// FIXME: there should be a separate macro to indicate that this functionality
// is available
    int setActiveSim(SimType sim) {
        return cellular_set_active_sim(sim, nullptr);
    }

    SimType getActiveSim() const {
        int sim = 0;
        const int r = cellular_get_active_sim(&sim, nullptr);
        if (r < 0) {
            return INVALID_SIM;
        }
        return (SimType)sim;
    }
#endif // HAL_PLATFORM_MESH

    void lock()
    {
        cellular_lock(nullptr);
    }

    void unlock()
    {
        cellular_unlock(nullptr);
    }
};


extern CellularClass Cellular;

}   // namespace Spark

#endif  // Wiring_Cellular
#endif	/* SPARK_WIRING_CELLULAR_H */

