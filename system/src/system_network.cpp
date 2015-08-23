#include "particle_wiring_ticks.h"
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

WLanConfig ip_config;

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

volatile uint8_t PARTICLE_WLAN_RESET;
volatile uint8_t PARTICLE_WLAN_SLEEP;
volatile uint8_t PARTICLE_WLAN_STARTED;


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
        network_set_credentials(0, 0, &creds, NULL);
    }
}

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
    PARTICLE_LED_FADE = 0;
    bool signaling = LED_RGB_IsOverRidden();
    LED_SetRGBColor(RGB_COLOR_BLUE);
    LED_Signaling_Stop();
    LED_On(LED_RGB);

    /* If WiFi module is connected, disconnect it */
    network_disconnect(0, 0, NULL);

    /* If WiFi module is powered off, turn it on */
    network_on(0, 0, 0, NULL);

    wlan_smart_config_init();

    WiFiSetupConsoleConfig config;
    config.connect_callback = wifi_add_profile_callback;
    WiFiSetupConsole console(config);

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
        PARTICLE_WLAN_SmartConfigProcess();
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
      PARTICLE_LED_FADE = 1;
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
      PARTICLE_LED_FADE = 0;
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
        PARTICLE_LED_FADE = 1;
        WLAN_LISTEN_ON_FAILED_CONNECT = false;
    }
    else
    {
        WLAN_DHCP = 0;
        PARTICLE_LED_FADE = 0;
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
    return &ip_config;
}

void network_config_clear()
{
    memset(&ip_config, 0, sizeof(ip_config));
}

void network_connect(network_handle_t network, uint32_t flags, uint32_t param, void* reserved)
{
    if (!network_ready(network, flags, reserved) && !WLAN_CONNECTING && !network_listening(network, flags, NULL))
    {
        bool was_sleeping = PARTICLE_WLAN_SLEEP;

        network_on(network, flags, param, NULL);

        WLAN_DISCONNECT = 0;
        wlan_connect_init();
        PARTICLE_WLAN_STARTED = 1;
        PARTICLE_WLAN_SLEEP = 0;

        if (wlan_reset_credentials_store_required())
        {
            wlan_reset_credentials_store();
        }

        if (!network_has_credentials(0, 0, NULL))
        {
            if ((flags && WIFI_CONNECT_SKIP_LISTEN)==0) {
                network_listen(0, 0, NULL);
            }
            else {
                if (was_sleeping) {
                    network_disconnect(network, 0, NULL);
                }
            }
        }
        else
        {
            PARTICLE_LED_FADE = 0;
            WLAN_CONNECTING = 1;
            LED_SetRGBColor(RGB_COLOR_GREEN);
            LED_On(LED_RGB);
            wlan_connect_finalize();
        }

        Set_NetApp_Timeout();
    }
}

void network_disconnect(network_handle_t network, uint32_t param, void* reserved)
{
    if (PARTICLE_WLAN_STARTED)
    {
        WLAN_DISCONNECT = 1; //Do not ARM_WLAN_WD() in WLAN_Async_Callback()
        WLAN_CONNECTING = 0;
        cloud_disconnect();
        wlan_disconnect_now();
        network_config_clear();
    }
}

bool network_ready(network_handle_t network, uint32_t param, void* reserved)
{
    return (PARTICLE_WLAN_STARTED && WLAN_DHCP);
}

bool network_connecting(network_handle_t network, uint32_t param, void* reserved)
{
    return (PARTICLE_WLAN_STARTED && WLAN_CONNECTING);
}

void network_on(network_handle_t network, uint32_t flags, uint32_t param, void* reserved)
{
    if (!PARTICLE_WLAN_STARTED)
    {
        network_config_clear();
        wlan_activate();
        PARTICLE_WLAN_STARTED = 1;
        PARTICLE_WLAN_SLEEP = 0;
        PARTICLE_LED_FADE = 1;
        LED_SetRGBColor(RGB_COLOR_BLUE);
        LED_On(LED_RGB);
    }
}

bool network_has_credentials(network_handle_t network, uint32_t param, void* reserved)
{
    return wlan_has_credentials()==0;
}

void network_off(network_handle_t network, uint32_t flags, uint32_t param, void* reserved)
{
    if (PARTICLE_WLAN_STARTED)
    {
        network_config_clear();
        cloud_disconnect();
        network_disconnect(network, param, reserved);
        wlan_deactivate();

        PARTICLE_WLAN_SLEEP = 1;
#if !PARTICLE_NO_CLOUD
        if (flags & 1) {
            particle_disconnect();
        }
#endif
        PARTICLE_WLAN_STARTED = 0;
        WLAN_DHCP = 0;
        WLAN_CONNECTED = 0;
        WLAN_CONNECTING = 0;
        PARTICLE_LED_FADE = 1;
        LED_SetRGBColor(RGB_COLOR_WHITE);
        LED_On(LED_RGB);
    }

}

void network_listen(network_handle_t, uint32_t flags, void*)
{
    WLAN_SMART_CONFIG_START = !(flags & 1);
    if (!WLAN_SMART_CONFIG_START)
        WLAN_LISTEN_ON_FAILED_CONNECT = 0;  // ensure a failed wifi connection attempt doesn't bring the device back to listening mode
}

bool network_listening(network_handle_t, uint32_t, void*)
{
    return (WLAN_SMART_CONFIG_START && !(WLAN_SMART_CONFIG_FINISHED || WLAN_SERIAL_CONFIG_DONE));
}

void network_set_credentials(network_handle_t, uint32_t, NetworkCredentials* credentials, void*)
{

    if (!PARTICLE_WLAN_STARTED || !credentials)
    {
        return;
    }

    WLanSecurityType security = credentials->security;

    if (0 == credentials->password[0])
    {
        security = WLAN_SEC_UNSEC;
    }

    credentials->security = security;

    wlan_set_credentials(credentials);
    system_notify_event(wifi_credentials_add, 0, credentials);
}

bool network_clear_credentials(network_handle_t, uint32_t, NetworkCredentials* creds, void*)
{
    return wlan_clear_credentials() == 0;
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
    bool fetched_config = ip_config.nw.aucIP.ipv4!=0;
    if (WLAN_DHCP && !PARTICLE_WLAN_SLEEP)
    {
        if (!fetched_config)
        {
            wlan_fetch_ipconfig(&ip_config);
        }
    }
    else if (fetched_config)
    {
        memset(&ip_config, 0, sizeof (ip_config));
    }
}
