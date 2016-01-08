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
#include "cellular_hal.h"


class CellularNetworkInterface : public ManagedNetworkInterface
{

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

    virtual void connect_finalize() override {
        cellular_result_t result = -1;
        result = cellular_init(NULL);
        if (result) return;

        result = cellular_register(NULL);
        if (result) return;

        CellularCredentials* savedCreds;
        savedCreds = cellular_credentials_get(NULL);
        result = cellular_pdp_activate(savedCreds, NULL);
        if (result) return;

        //DEBUG_D("savedCreds = %s %s %s\r\n", savedCreds->apn, savedCreds->username, savedCreds->password);
        result = cellular_gprs_attach(savedCreds, NULL);
        if (result) return;

        HAL_NET_notify_connected();
        HAL_NET_notify_dhcp(true);
    }

    void fetch_ipconfig(WLanConfig* target) override {
        cellular_fetch_ipconfig(target, NULL);
    }

    void on_now() override {
        cellular_on(NULL);
    }

    void off_now() override {
        cellular_pdp_deactivate(NULL);
        cellular_gprs_detach(NULL);
        cellular_off(NULL);
    }

    void disconnect_now() override {
        cellular_pdp_deactivate(NULL);
        cellular_gprs_detach(NULL);
    }

public:

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
        return cellular_sim_ready(NULL);
    }
    int set_credentials(NetworkCredentials* creds) override { /* n/a */ return -1; }
    void connect_cancel(bool cancel, bool calledFromISR) override { cellular_cancel(cancel, calledFromISR, NULL);  }

    void set_error_count(unsigned count) override { /* n/a */ }
};

