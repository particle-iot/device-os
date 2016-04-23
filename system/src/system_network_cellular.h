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

    void connect_finalize_impl() {
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

    void connect_finalize() override {
		ATOMIC_BLOCK() { connecting = true; }

		connect_finalize_impl();

		bool require_resume = false;

        ATOMIC_BLOCK() {
        		// ensure after connection exits the cancel flag is cleared if it was set during connection
        		if (connect_cancelled) {
        			require_resume = true;
        		}
        		connecting = false;
        }
        if (require_resume)
        		cellular_cancel(false, HAL_IsISR(), NULL);
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
        return cellular_sim_ready(NULL);
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
    		if (require_cancel)
    			cellular_cancel(cancel, HAL_IsISR(), NULL);
    }

    void set_error_count(unsigned count) override { /* n/a */ }
};

