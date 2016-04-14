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

#ifndef SYSTEM_NETWORK_INTERNAL_H
#define	SYSTEM_NETWORK_INTERNAL_H

#include "system_setup.h"
#include "rgbled.h"
#include "spark_wiring_ticks.h"
#include "system_event.h"
#include "system_cloud_internal.h"
#include "system_network.h"
#include "system_threading.h"
#include "system_rgbled.h"


enum eWanTimings
{
    CONNECT_TO_ADDRESS_MAX = S2M(30),
    DISCONNECT_TO_RECONNECT = S2M(2),
};

extern volatile uint8_t SPARK_WLAN_RESET;
extern volatile uint8_t SPARK_WLAN_SLEEP;
extern volatile uint8_t SPARK_WLAN_STARTED;

extern volatile uint8_t SPARK_LED_FADE;
void manage_smart_config();
void manage_ip_config();

extern uint32_t wlan_watchdog_duration;
extern uint32_t wlan_watchdog_base;

#if defined(DEBUG_WAN_WD)
#define WAN_WD_DEBUG(x,...) DEBUG(x,__VA_ARGS__)
#else
#define WAN_WD_DEBUG(x,...)
#endif


inline void ARM_WLAN_WD(uint32_t x) {
    wlan_watchdog_base = HAL_Timer_Get_Milli_Seconds();
    wlan_watchdog_duration = x;
    WAN_WD_DEBUG("WD Set %d",(x));
}
inline bool WLAN_WD_TO() {
    return wlan_watchdog_duration && ((HAL_Timer_Get_Milli_Seconds()-wlan_watchdog_base)>wlan_watchdog_duration);
}

inline void CLR_WLAN_WD() {
    wlan_watchdog_duration = 0;
    WAN_WD_DEBUG("WD Cleared, was %d",wlan_watchdog_duration);
}

/**
 * Internal network interface class to provide polymorphic behavior for each
 * network type.  This is not part of the dynalib so functions can freely evolve.
 */
struct NetworkInterface
{

    virtual network_interface_t network_interface()=0;
    virtual void setup()=0;

    virtual void on(bool update_led)=0;
    virtual void off(bool disconnect_cloud=false)=0;
    virtual void connect(bool listen_enabled=true)=0;
    virtual bool connecting()=0;
    virtual bool connected()=0;
    virtual void connect_cancel(bool cancel, bool calledFromISR)=0;
    /**
     * Force a manual disconnct.
     */
    virtual void disconnect()=0;

    /**
     * @return {@code true} if this connection was manually taken down.
     */
    virtual bool manual_disconnect()=0;
    virtual void listen(bool stop=false)=0;
    virtual void listen_loop()=0;
    virtual bool listening()=0;
    /**
     * Perform the 10sec press command, e.g. clear credentials.
     */
    virtual void listen_command()=0;
    virtual bool ready()=0;

    virtual bool clear_credentials()=0;
    virtual bool has_credentials()=0;
    virtual int set_credentials(NetworkCredentials* creds)=0;

    virtual void config_clear()=0;
    virtual void update_config(bool force=false)=0;
    virtual void* config()=0;       // not really happy about lack of type

};


class ManagedNetworkInterface : public NetworkInterface
{
    volatile uint8_t WLAN_DISCONNECT;
    volatile uint8_t WLAN_DELETE_PROFILES;
    volatile uint8_t WLAN_SMART_CONFIG_START;
    volatile uint8_t WLAN_SMART_CONFIG_STOP;
    volatile uint8_t WLAN_SMART_CONFIG_FINISHED;
    volatile uint8_t WLAN_CONNECTED;
    volatile uint8_t WLAN_CONNECTING;
    volatile uint8_t WLAN_DHCP;
    volatile uint8_t WLAN_CAN_SHUTDOWN;
    volatile uint8_t WLAN_LISTEN_ON_FAILED_CONNECT;

protected:
    volatile uint8_t WLAN_SERIAL_CONFIG_DONE;

    virtual network_interface_t network_interface() override { return 0; }

    virtual void start_listening()=0;

    template<typename T> void start_listening(SystemSetupConsole<T>& console)
    {
        bool started = SPARK_WLAN_STARTED;
        WLAN_SMART_CONFIG_FINISHED = 0;
        WLAN_SMART_CONFIG_STOP = 0;
        WLAN_SERIAL_CONFIG_DONE = 0;

        cloud_disconnect();
        RGBLEDState led_state;
        led_state.save();
        SPARK_LED_FADE = 0;
        LED_SetRGBColor(RGB_COLOR_BLUE);
        LED_Signaling_Stop();
        LED_On(LED_RGB);

        on_start_listening();

        const uint32_t start = millis();
        uint32_t loop = start;
        system_notify_event(wifi_listen_begin, 0);

        /* Wait for SmartConfig/SerialConfig to finish */
        while (network_listening(0, 0, NULL))
        {
            if (WLAN_DELETE_PROFILES)
            {
                int toggle = 25;
                while (toggle--)
                {
                    LED_Toggle(LED_RGB);
                    HAL_Delay_Milliseconds(50);
                }
                if (!network_clear_credentials(0, 0, NULL, NULL) || network_has_credentials(0, 0, NULL)) {
                    LED_SetRGBColor(RGB_COLOR_RED);
                    LED_On(LED_RGB);

                    int toggle = 25;
                    while (toggle--)
                    {
                        LED_Toggle(LED_RGB);
                        HAL_Delay_Milliseconds(50);
                    }
                    LED_SetRGBColor(RGB_COLOR_BLUE);
                    LED_On(LED_RGB);
                }
                system_notify_event(network_credentials, network_credentials_cleared);
                WLAN_DELETE_PROFILES = 0;
            }
            else
            {
                uint32_t now = millis();
                if ((now-loop)>250) {
                    LED_Toggle(LED_RGB);
                    loop = now;
                    system_notify_event(wifi_listen_update, now-start);
                }
                console.loop();
            }
#if PLATFORM_THREADING
            if (!APPLICATION_THREAD_CURRENT()) {
                SystemThread.process();
            }
#endif
        }

        LED_On(LED_RGB);
        led_state.restore();

        WLAN_LISTEN_ON_FAILED_CONNECT = started && on_stop_listening();

        on_finalize_listening(WLAN_SMART_CONFIG_FINISHED);

        system_notify_event(wifi_listen_end, millis()-start);

        WLAN_SMART_CONFIG_START = 0;
        if (has_credentials())
            connect();
        else if (!started)
            off();
    }

    virtual void on_start_listening()=0;
    /**
     *
     * @param setup
     * @return true if network credentials were configured.
     */
    virtual bool on_stop_listening()=0;

    virtual void on_setup_cleanup()=0;

    virtual void connect_init()=0;
    virtual void connect_finalize()=0;
    virtual void disconnect_now()=0;

    virtual void on_now()=0;
    virtual void off_now()=0;

    /**
     *
     * @param external_process_complete If some external process triggered exit of listen mode.
     */
    virtual void on_finalize_listening(bool external_process_complete)=0;

public:

    virtual void get_ipconfig(IPConfig* config)=0;

    virtual void set_error_count(unsigned count)=0;

    bool manual_disconnect() override
    {
        return WLAN_DISCONNECT;
    }

    void set_manual_disconnect(bool disconnect)
    {
        WLAN_DISCONNECT = disconnect;
    }

    bool connected() override
    {
        return WLAN_CONNECTED;
    }


    void listen(bool stop=false) override
    {
        WLAN_SMART_CONFIG_START = !stop;
        if (!WLAN_SMART_CONFIG_START)
            WLAN_LISTEN_ON_FAILED_CONNECT = 0;  // ensure a failed wifi connection attempt doesn't bring the device back to listening mode
    }

    void listen_command() override
    {
        WLAN_DELETE_PROFILES = 1;
    }

    bool listening() override
    {
        return (WLAN_SMART_CONFIG_START && !(WLAN_SMART_CONFIG_FINISHED || WLAN_SERIAL_CONFIG_DONE));
    }


    void connect(bool listen_enabled=true) override
    {
        INFO("ready():%s,connecting():%s,listening():%s",(ready())?"true":"false",(connecting())?"true":"false",(listening())?"true":"false");
        if (!ready() && !connecting() && !listening())
        {
            bool was_sleeping = SPARK_WLAN_SLEEP;

            // activate WiFi, don't set LED since that happens later.
            on(false);

            WLAN_DISCONNECT = 0;
            connect_init();
            SPARK_WLAN_STARTED = 1;
            SPARK_WLAN_SLEEP = 0;

            if (!has_credentials())
            {
                if (listen_enabled) {
                    listen();
                }
                else {
                    if (was_sleeping) {
                        disconnect();
                    }
                }
            }
            else
            {
                SPARK_LED_FADE = 0;
                WLAN_CONNECTING = 1;
                LED_SetRGBColor(RGB_COLOR_GREEN);
                ARM_WLAN_WD(CONNECT_TO_ADDRESS_MAX);    // reset the network if it doesn't connect within the timeout
                connect_finalize();
            }
        }
    }

    void disconnect() override
    {
        if (SPARK_WLAN_STARTED)
        {
            WLAN_DISCONNECT = 1; //Do not ARM_WLAN_WD() in WLAN_Async_Callback()
            WLAN_CONNECTING = 0;
            WLAN_CONNECTED = 0;
            WLAN_DHCP = 0;

            cloud_disconnect();
            disconnect_now();
            config_clear();
        }
    }

    bool ready() override
    {
        return (SPARK_WLAN_STARTED && WLAN_DHCP);
    }

    bool connecting() override
    {
        return (SPARK_WLAN_STARTED && WLAN_CONNECTING);
    }

    void on(bool update_led=true) override
    {
        if (!SPARK_WLAN_STARTED)
        {
            config_clear();
            on_now();
            SPARK_WLAN_STARTED = 1;
            SPARK_WLAN_SLEEP = 0;
            SPARK_LED_FADE = 1;
            if (update_led) {
                LED_SetRGBColor(RGB_COLOR_BLUE);
                LED_On(LED_RGB);
            }
        }
    }

    void off(bool disconnect_cloud=false) override
    {
        if (SPARK_WLAN_STARTED)
        {
            disconnect();
            off_now();

            SPARK_WLAN_SLEEP = 1;
#if !SPARK_NO_CLOUD
            if (disconnect_cloud) {
                spark_cloud_flag_disconnect();
            }
#endif
            SPARK_WLAN_STARTED = 0;
            WLAN_DHCP = 0;
            WLAN_CONNECTED = 0;
            WLAN_CONNECTING = 0;
            WLAN_SERIAL_CONFIG_DONE = 1;
            SPARK_LED_FADE = 1;
            LED_SetRGBColor(RGB_COLOR_WHITE);
            LED_On(LED_RGB);
        }
    }

    void notify_listening_complete()
    {
        WLAN_SMART_CONFIG_FINISHED = 1;
        WLAN_SMART_CONFIG_STOP = 1;
    }

    void notify_connected()
    {
        WLAN_CONNECTED = 1;
        WLAN_CONNECTING = 0;
        if (!WLAN_DISCONNECT)
        {
            ARM_WLAN_WD(CONNECT_TO_ADDRESS_MAX);
        }
    }

    void notify_disconnected()
    {
        cloud_disconnect(false); // don't close the socket on the callback since this causes a lockup on the Core
        if (WLAN_CONNECTED)     /// unsolicited disconnect
        {
          //Breathe blue if established connection gets disconnected
          if(!WLAN_DISCONNECT)
          {
            //if WiFi.disconnect called, do not enable wlan watchdog
            ARM_WLAN_WD(DISCONNECT_TO_RECONNECT);
          }
          SPARK_LED_FADE = 1;
          LED_SetRGBColor(RGB_COLOR_BLUE);
          LED_On(LED_RGB);
        }
        else if (!WLAN_SMART_CONFIG_START)
        {
          //Do not enter if smart config related disconnection happens
          //Blink green if connection fails because of wrong password
            if (!WLAN_DISCONNECT) {
                ARM_WLAN_WD(DISCONNECT_TO_RECONNECT);
            }
          SPARK_LED_FADE = 0;
          LED_SetRGBColor(RGB_COLOR_GREEN);
          LED_On(LED_RGB);
        }
        WLAN_CONNECTED = 0;
        WLAN_CONNECTING = 0;
        WLAN_DHCP = 0;
    }

    void notify_dhcp(bool dhcp)
    {
        WLAN_CONNECTING = 0;
        if (!WLAN_SMART_CONFIG_START)
        {
            LED_SetRGBColor(RGB_COLOR_GREEN);
        }
        if (dhcp)
        {
            LED_On(LED_RGB);
            CLR_WLAN_WD();
            WLAN_DHCP = 1;
            SPARK_LED_FADE = 1;
            WLAN_LISTEN_ON_FAILED_CONNECT = false;
        }
        else
        {
            config_clear();
            WLAN_DHCP = 0;
            SPARK_LED_FADE = 0;
            if (WLAN_LISTEN_ON_FAILED_CONNECT)
                listen();
            else
                ARM_WLAN_WD(DISCONNECT_TO_RECONNECT);
        }
    }

    void notify_can_shutdown()
    {
        WLAN_CAN_SHUTDOWN = 1;
    }

    void notify_cannot_shutdown()
    {
        WLAN_CAN_SHUTDOWN = 0;
    }


    void listen_loop() override
    {
        if (WLAN_SMART_CONFIG_START)
        {
            start_listening();
        }

        // Complete Smart Config Process:
        // 1. if smart config is done
        // 2. CC3000 established AP connection
        // 3. DHCP IP is configured
        // then send mDNS packet to stop external SmartConfig application
        if ((WLAN_SMART_CONFIG_STOP == 1) && (WLAN_DHCP == 1) && (WLAN_CONNECTED == 1))
        {
            on_setup_cleanup();
            WLAN_SMART_CONFIG_STOP = 0;
        }
    }

    inline bool hasDHCP()
    {
    		return WLAN_DHCP && !SPARK_WLAN_SLEEP;
    }

};

extern ManagedNetworkInterface& network;

template <typename Config, typename C>
class ManagedIPNetworkInterface : public ManagedNetworkInterface
{
	Config ip_config;

public:

    void get_ipconfig(IPConfig* config) override
    {
    		update_config(true);
    		memcpy(config, this->config(), config->size);
    }

    void update_config(bool force=false) override
    {
    		// todo - IPv6 may not set this field.
        bool fetched_config = ip_config.nw.aucIP.ipv4!=0;
        if (hasDHCP() || force)
        {
            if (!fetched_config || force)
            {
            		memset(&ip_config, 0, sizeof(ip_config));
            		ip_config.size = sizeof(ip_config);
            		reinterpret_cast<C*>(this)->fetch_ipconfig(&ip_config);
            }
        }
        else if (fetched_config)
        {
            config_clear();
        }
    }

    void config_clear() override
    {
        memset(&ip_config, 0, sizeof(ip_config));
    }

    void* config() override  { return &ip_config; }

};



#endif	/* SYSTEM_NETWORK_INTERNAL_H */

