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

#pragma once

#include "system_network_internal.h"
#include "wlan_hal.h"


class WiFiNetworkInterface : public ManagedNetworkInterface
{

    static int wifi_add_profile_callback(void* data, const char *ssid, const char *password,
        unsigned long security_type, unsigned long cipher, bool dry_run)
    {
        return ((WiFiNetworkInterface*)data)->add_profile(ssid, password, security_type, cipher, dry_run);
    }

    /**
     * Callback from the wifi credentials reader.
     * @param ssid
     * @param password
     * @param security_type
     */
    int add_profile(const char *ssid, const char *password,
        unsigned long security_type, unsigned long cipher, bool dry_run)
    {
        int result = 0;
        if (ssid)
        {
            NetworkCredentials creds;
            memset(&creds, 0, sizeof (creds));
            creds.size = sizeof (creds);
            creds.ssid = ssid;
            creds.password = password;
            creds.ssid_len = strlen(ssid);
            creds.password_len = strlen(password);
            creds.security = WLanSecurityType(security_type);
            creds.cipher = WLanSecurityCipher(cipher);
            if (dry_run)
                creds.flags |= WLAN_SET_CREDENTIALS_FLAGS_DRY_RUN;
            result = network_set_credentials(0, 0, &creds, NULL);
        }
        if (result==0)
            WLAN_SERIAL_CONFIG_DONE = 1;
        return result;
    }

protected:

    virtual void on_finalize_listening(bool complete) override
    {
        if (complete)
            SPARK_WLAN_SmartConfigProcess();
#if PLATFORM_ID<3 // this is needed to get the CC3000 to retry the wifi connection.
        off();
#endif
    }

    virtual void on_start_listening() override
    {
        notify_cannot_shutdown();
        /* If WiFi module is connected, disconnect it */
        network_disconnect(0, 0, NULL);

        /* If WiFi module is powered off, turn it on */
        network_on(0, 0, 0, NULL);

        wlan_smart_config_init();
    }

    virtual bool on_stop_listening() override
    {
        return wlan_smart_config_finalize();
    }

    void connect_init() override
    {
        wlan_connect_init();

        Set_NetApp_Timeout();
    }

    void connect_finalize() override
    {
        wlan_connect_finalize();
    }

    void disconnect_now() override
    {
        wlan_disconnect_now();
    }

    void on_now() override { wlan_activate(); }
    void off_now() override { wlan_deactivate(); }

    void on_setup_cleanup() override { wlan_smart_config_cleanup(); }



public:


    virtual void start_listening() override
    {
        WiFiSetupConsoleConfig config;
        config.connect_callback = wifi_add_profile_callback;
        config.connect_callback_data = this;
        WiFiSetupConsole console(config);

        ManagedNetworkInterface::start_listening(console);
    }


    void connect_cancel(bool cancel, bool calledFromISR) override
    {
        if (cancel)
            wlan_connect_cancel(calledFromISR);
    }

    bool has_credentials() override
    {
        return wlan_has_credentials()==0;
    }

    int set_credentials(NetworkCredentials* credentials) override
    {
        if (!SPARK_WLAN_STARTED || !credentials)
        {
            return -1;
        }

        WLanSecurityType security = credentials->security;

        if (0 == credentials->password[0])
        {
            security = WLAN_SEC_UNSEC;
        }

        credentials->security = security;

        int result = wlan_set_credentials(credentials);
        if (!result)
            system_notify_event(network_credentials, network_credentials_added, credentials);
        return result;
    }

    bool clear_credentials() override
    {
        return wlan_clear_credentials() == 0;
    }

    void setup() override
    {
        wlan_setup();

        if (wlan_reset_credentials_store_required())
        {
            wlan_reset_credentials_store();
        }
    }

    void fetch_ipconfig(WLanConfig* target) override
    {
        wlan_fetch_ipconfig(target);
    }

    void set_error_count(unsigned count) override
    {
        wlan_set_error_count(count);
    }

};

