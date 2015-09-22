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

    virtual void on_finalize_listening(bool complete) override
    {
    }
    
    virtual void on_start_listening() override { cellular_on(NULL); }
    virtual bool on_stop_listening() override { /* n/a */ return false; }
    virtual void on_setup_cleanup() override { /* n/a */ }

    virtual void connect_init() override {
        cellular_result_t ok = -1;
        ok = cellular_register(NULL);
        if (ok == 0) {
            CellularCredentials* savedCreds;
            savedCreds = cellular_credentials_get(NULL);
            //DEBUG_D("savedCreds = %s %s %s\r\n", savedCreds->apn, savedCreds->username, savedCreds->password);
            cellular_pdp_activate(savedCreds, NULL);
        }
    }

    virtual void connect_finalize() override {

        CellularCredentials* savedCreds;
        savedCreds = cellular_credentials_get(NULL);
        //DEBUG_D("savedCreds = %s %s %s\r\n", savedCreds->apn, savedCreds->username, savedCreds->password);
        cellular_result_t ok = -1;
        ok = cellular_gprs_attach(savedCreds, NULL);
        if (ok == 0) {
            HAL_WLAN_notify_connected();
            HAL_WLAN_notify_dhcp(true);
        }
    }

    void fetch_ipconfig(WLanConfig* target) override {
        cellular_fetch_ipconfig(target, NULL);
    }

    void on_now() override { cellular_on(NULL); }

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

    void setup() override
    {
        //cellular_init(NULL);
    }

    // todo - associate credentials with presense of SIM card??
    bool clear_credentials() override { /* n/a */ return true; }
    bool has_credentials() override { /* n/a */ return true; }
    int set_credentials(NetworkCredentials* creds) override { return -1; }
    void connect_cancel() override { /* n/a */ }

    void set_error_count(unsigned count) override
    {
    }
};

