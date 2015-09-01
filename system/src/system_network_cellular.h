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

    virtual void on_start_listening() { }
    virtual bool on_stop_listening() override { return false; }
    virtual void on_setup_cleanup() override { }

    virtual void connect_init() override {

    }

    virtual void connect_finalize() override {

    }

    void fetch_ipconfig(WLanConfig* target) {
        cellular_fetch_ipconfig(target);
    }

    void on_now() override { cellular_on(NULL); }
    void off_now() override { cellular_off(NULL); }
    void disconnect_now() override { cellular_disconnect(NULL); }

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

    // todo - wire credentials through to presense of SIM card??
    bool clear_credentials() override { return true; }
    bool has_credentials() override { return true; }

    void set_credentials(NetworkCredentials* creds) override {}

    void connect_cancel() override {}


};

