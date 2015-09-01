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

#include "spark_wiring_ticks.h"
#include "system_setup.h"
#include "system_network.h"
#include "system_network_internal.h"
#include "system_cloud.h"
#include "system_event.h"
#include "watchdog_hal.h"
#include "wlan_hal.h"
#include "delay_hal.h"
#include "rgbled.h"
#include <string.h>

uint32_t wlan_watchdog_base;
uint32_t wlan_watchdog_duration;

volatile uint8_t WLAN_DISCONNECT;
volatile uint8_t WLAN_DELETE_PROFILES;
volatile uint8_t WLAN_SMART_CONFIG_START;
volatile uint8_t WLAN_SMART_CONFIG_STOP;
volatile uint8_t WLAN_SMART_CONFIG_FINISHED = 1;
volatile uint8_t WLAN_SERIAL_CONFIG_DONE = 1;
volatile uint8_t WLAN_CONNECTED;
volatile uint8_t WLAN_CONNECTING;
volatile uint8_t WLAN_DHCP;
volatile uint8_t WLAN_CAN_SHUTDOWN;
volatile uint8_t WLAN_LISTEN_ON_FAILED_CONNECT;

volatile uint8_t SPARK_WLAN_RESET;
volatile uint8_t SPARK_WLAN_SLEEP;
volatile uint8_t SPARK_WLAN_STARTED;

#if Wiring_WiFi
#include "system_network_wifi.h"
WiFiNetworkInterface wifi;

inline NetworkInterface& nif(network_interface_t _nif) { return wifi; }

/**
 * Callback from the wifi credentials reader.
 * @param ssid
 * @param password
 * @param security_type
 */
void wifi_add_profile_callback(const char *ssid,
    const char *password,
    unsigned long security_type)
{
    WLAN_SERIAL_CONFIG_DONE = 1;
    if (ssid)
    {
        NetworkCredentials creds;
        memset(&creds, 0, sizeof (creds));
        creds.len = sizeof (creds);
        creds.ssid = ssid;
        creds.password = password;
        creds.ssid_len = strlen(ssid);
        creds.password_len = strlen(password);
        creds.security = WLanSecurityType(security_type);
        wifi.set_credentials(&creds);
    }
}

#endif

/*******************************************************************************
 * Function Name  : Start_Smart_Config.
 * Description    : The function triggers a smart configuration process on CC3000.
 * Input          : None.
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
void Start_Smart_Config(void)
{
    WLAN_SMART_CONFIG_FINISHED = 0;
    WLAN_SMART_CONFIG_STOP = 0;
    WLAN_SERIAL_CONFIG_DONE = 0;
    WLAN_CONNECTED = 0;
    WLAN_CONNECTING = 0;
    WLAN_DHCP = 0;
    WLAN_CAN_SHUTDOWN = 0;

    cloud_disconnect();
    SPARK_LED_FADE = 0;
    bool signaling = LED_RGB_IsOverRidden();
    LED_SetRGBColor(RGB_COLOR_BLUE);
    LED_Signaling_Stop();
    LED_On(LED_RGB);

    // TODO: refactor this for non-WiFi networks
    // Rename function to Start_Setup())
#if Wiring_WiFi
    /* If WiFi module is connected, disconnect it */
    network_disconnect(0, 0, NULL);

    /* If WiFi module is powered off, turn it on */
    network_on(0, 0, 0, NULL);

    wlan_smart_config_init();

    WiFiSetupConsoleConfig config;
    config.connect_callback = wifi_add_profile_callback;
    WiFiSetupConsole console(config);
#elif Wiring_Cellular

    CellularSetupConsoleConfig config;
    CellularSetupConsole console(config);
#endif

    const uint32_t start = millis();
    uint32_t loop = start;
    system_notify_event(wifi_listen_begin, start);

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
            system_notify_event(wifi_credentials_cleared);
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
    }

    LED_On(LED_RGB);
    if (signaling)
        LED_Signaling_Start();

    WLAN_LISTEN_ON_FAILED_CONNECT = wlan_smart_config_finalize();

    if (WLAN_SMART_CONFIG_FINISHED)
    {
        /* Decrypt configuration information and add profile */
        SPARK_WLAN_SmartConfigProcess();
    }

    system_notify_event(wifi_listen_end, millis()-start);

    WLAN_SMART_CONFIG_START = 0;
    network_connect(0, 0, 0, NULL);
}


void HAL_WLAN_notify_simple_config_done()
{
    WLAN_SMART_CONFIG_FINISHED = 1;
    WLAN_SMART_CONFIG_STOP = 1;
}

void HAL_WLAN_notify_connected()
{
    WLAN_CONNECTED = 1;
    WLAN_CONNECTING = 0;
    if (!WLAN_DISCONNECT)
    {
        ARM_WLAN_WD(CONNECT_TO_ADDRESS_MAX);
    }
}


void HAL_WLAN_notify_disconnected()
{
    cloud_disconnect();
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

void HAL_WLAN_notify_dhcp(bool dhcp)
{
    WLAN_CONNECTING = 0;
    if (!WLAN_SMART_CONFIG_START)
    {
        LED_SetRGBColor(RGB_COLOR_GREEN);
        LED_On(LED_RGB);
    }
    if (dhcp)
    {
        CLR_WLAN_WD();
        WLAN_DHCP = 1;
        SPARK_LED_FADE = 1;
        WLAN_LISTEN_ON_FAILED_CONNECT = false;
    }
    else
    {
        WLAN_DHCP = 0;
        SPARK_LED_FADE = 0;
        if (WLAN_LISTEN_ON_FAILED_CONNECT)
            network_listen(0, 0, NULL);
        else
             ARM_WLAN_WD(DISCONNECT_TO_RECONNECT);
    }
}

void HAL_WLAN_notify_can_shutdown()
{
    WLAN_CAN_SHUTDOWN = 1;
}

const WLanConfig* network_config(network_handle_t network, uint32_t param, void* reserved)
{
    return nif(network).config();
}

void network_connect(network_handle_t network, uint32_t flags, uint32_t param, void* reserved)
{
    nif(network).connect(!(flags & WIFI_CONNECT_SKIP_LISTEN));
}

void network_disconnect(network_handle_t network, uint32_t param, void* reserved)
{
    nif(network).disconnect();
}

bool network_ready(network_handle_t network, uint32_t param, void* reserved)
{
    return nif(network).ready();
}

bool network_connecting(network_handle_t network, uint32_t param, void* reserved)
{
    return nif(network).connecting();
}

/**
 *
 * @param network
 * @param flags    1 - don't change the LED color
 * @param param
 * @param reserved
 */
void network_on(network_handle_t network, uint32_t flags, uint32_t param, void* reserved)
{
    return nif(network).on(!(flags & 1));
}

bool network_has_credentials(network_handle_t network, uint32_t param, void* reserved)
{
    return nif(network).has_credentials();
}

void network_off(network_handle_t network, uint32_t flags, uint32_t param, void* reserved)
{
    // flags & 1 means also disconnect the cloud (so it doesn't autmatically connect when network resumed.)
    return nif(network).off(flags & 1);
}

/**
 *
 * @param network
 * @param flags  bit 0 set means to stop listening.
 * @param
 */
void network_listen(network_handle_t network, uint32_t flags, void*)
{
    nif(network).listen(flags & 1);
}

bool network_listening(network_handle_t network, uint32_t, void*)
{
    return nif(network).listening();
}

void network_set_credentials(network_handle_t network, uint32_t, NetworkCredentials* credentials, void*)
{
    return nif(network).set_credentials(credentials);
}

bool network_clear_credentials(network_handle_t network, uint32_t, NetworkCredentials* creds, void*)
{
    return nif(network).clear_credentials();
}

void network_setup(network_handle_t network, uint32_t flags, void* reserved)
{
    nif(network).setup();
}

void manage_smart_config()
{
    if (WLAN_SMART_CONFIG_START)
    {
        Start_Smart_Config();
    }

    // Complete Smart Config Process:
    // 1. if smart config is done
    // 2. CC3000 established AP connection
    // 3. DHCP IP is configured
    // then send mDNS packet to stop external SmartConfig application
    if ((WLAN_SMART_CONFIG_STOP == 1) && (WLAN_DHCP == 1) && (WLAN_CONNECTED == 1))
    {
        wlan_smart_config_cleanup();
        WLAN_SMART_CONFIG_STOP = 0;
    }
}

void manage_ip_config()
{
    nif(0).update_config();
}
