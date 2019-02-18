/**
 ******************************************************************************
  Copyright (c) 2015 Particle Industries, Inc.  All rights reserved.

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

#pragma once

#include "system_network_internal.h"

/* FIXME: there should be a define that tells whether there is NetworkManager available
 * or not */
#if !HAL_PLATFORM_IFAPI

#include "cellular_hal.h"
#include "interrupts_hal.h"
#include "spark_wiring_interrupts.h"

class CellularNetworkInterface : public ManagedIPNetworkInterface<CellularConfig, CellularNetworkInterface>
{
    volatile bool connect_cancelled = false;
    volatile bool connecting = false;

protected:

    virtual void on_finalize_listening(bool complete) override { /* n/a */ }

    virtual void on_start_listening() override {
        cellular_cancel(false, true, NULL);  // resume
    }

    virtual bool on_stop_listening() override {
        /* in case we interrupted during connecting(), force system to stop WLAN_CONNECTING */
        if (ManagedNetworkInterface::connecting()) ManagedNetworkInterface::disconnect();
        CLR_WLAN_WD(); // keep system from power cycling modem in manage_network_connection()
        return false;
    }

    virtual void on_setup_cleanup() override { /* n/a */ }

    virtual void connect_init() override { /* n/a */ }

    int connect_finalize() override {
        ATOMIC_BLOCK() { connecting = true; }

        int ret = cellular_connect(nullptr);

        bool require_resume = false;

        ATOMIC_BLOCK() {
            // ensure after connection exits the cancel flag is cleared if it was set during connection
            if (connect_cancelled) {
                require_resume = true;
                // This flag needs to be reset, otherwise the next connect_cancel() will do nothing
                connect_cancelled = false;
            }
            connecting = false;
        }
        if (require_resume) {
            cellular_cancel(false, HAL_IsISR(), NULL);
            ret = SYSTEM_ERROR_ABORTED; // FIXME: Return a HAL-specific error code
        }

        return ret;
    }

    int on_now() override {
        cellular_result_t ret = cellular_on(nullptr);
        if (ret != 0) {
            return ret;
        }
        ret = cellular_init(nullptr);
        if (ret != 0) {
            return ret;
        }
        return 0;
    }

    void off_now() override {
        cellular_disconnect(nullptr);
        cellular_off(nullptr);
    }

    void disconnect_now() override {
        cellular_disconnect(nullptr);
    }

public:

    CellularNetworkInterface() {
        HAL_NET_Callbacks cb;
        cb.size = sizeof(HAL_NET_Callbacks);
        cb.notify_connected = HAL_NET_notify_connected;
        cb.notify_disconnected = HAL_NET_notify_disconnected;
        cb.notify_dhcp = HAL_NET_notify_dhcp;
        cb.notify_can_shutdown = HAL_NET_notify_can_shutdown;
        HAL_NET_SetCallbacks(&cb, nullptr);
    }

    void fetch_ipconfig(CellularConfig* target)  {
        cellular_fetch_ipconfig(target, NULL);
    }

    void start_listening() override
    {
        CellularSetupConsoleConfig config;
        CellularSetupConsole console(config);
        ManagedNetworkInterface::start_listening(console);
    }

    bool listening() override
    {
        return ManagedNetworkInterface::listening();
    }

    void setup() override { /* n/a */ }

    // todo - associate credentials with presense of SIM card??
    bool clear_credentials() override { /* n/a */ return true; }
    bool has_credentials() override
    {
        bool rv = cellular_sim_ready(NULL);
        LOG(INFO,"%s", (rv)?"Sim Ready":"Sim not inserted? Detecting...");
        if (!rv) {
            cellular_on(NULL);
            rv = cellular_sim_ready(NULL);
            LOG(INFO,"%s", (rv)?"Sim Ready":"Sim not inserted.");
        }
        return rv;
    }
    int set_credentials(NetworkCredentials* creds) override { /* n/a */ return -1; }

    void connect_cancel(bool cancel) override {
        // only cancel if presently connecting
        bool require_cancel = false;
        ATOMIC_BLOCK() {
            if (connecting)
            {
                if (cancel!=connect_cancelled) {
                    require_cancel = true;
                    connect_cancelled = cancel;
                }
            }
        }
        if (require_cancel) {
            cellular_cancel(cancel, HAL_IsISR(), NULL);
        }
    }

    void set_error_count(unsigned count) override { /* n/a */ }

    virtual int set_hostname(const char* hostname) override
    {
        return 1;
    }

    virtual int get_hostname(char* buf, size_t buf_len, bool noDefault) override
    {
        if (buf) {
            buf[0] = '\0';
        }
        return 1;
    }
};

#endif /* !HAL_PLATFORM_IFAPI */
