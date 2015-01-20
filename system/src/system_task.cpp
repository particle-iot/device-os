/**
 ******************************************************************************
 * @file    spark_wiring_wlan.cpp
 * @author  Satish Nair and Zachary Crockett
 * @version V1.0.0
 * @date    13-March-2013
 * @brief   
 ******************************************************************************
  Copyright (c) 2013 Spark Labs, Inc.  All rights reserved.

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
#include "system_task.h"
#include "spark_macros.h"
#include "string.h"
#include "wifi_credentials_reader.h"
#include "system_tick_hal.h"
#include "watchdog_hal.h"
#include "wlan_hal.h"
#include "delay_hal.h"
#include "timer_hal.h"
#include "rgbled.h"
#include "spark_wiring_system.h"
#include "system_cloud.h"
#include "system_mode.h"

WLanConfig ip_config;

uint32_t wlan_watchdog;

//#define DEBUG_WIFI    // Define to show all the flags in debug output
//#define DEBUG_WAN_WD  // Define to show all SW WD activity in debug output


volatile uint8_t WLAN_DISCONNECT;
volatile uint8_t WLAN_DELETE_PROFILES;
volatile uint8_t WLAN_SMART_CONFIG_START;
volatile uint8_t WLAN_SMART_CONFIG_STOP;
volatile uint8_t WLAN_SMART_CONFIG_FINISHED = 1;
volatile uint8_t WLAN_SERIAL_CONFIG_DONE = 1;
volatile uint8_t WLAN_CONNECTED;
volatile uint8_t WLAN_DHCP;
volatile uint8_t WLAN_CAN_SHUTDOWN;

enum eWanTimings {
  CONNECT_TO_ADDRESS_MAX = S2M(30),
  DISCONNECT_TO_RECONNECT = S2M(30),
};

volatile system_tick_t spark_loop_total_millis = 0;

void (*announce_presence)(void);

// Auth options are WLAN_SEC_UNSEC, WLAN_SEC_WPA, WLAN_SEC_WEP, and WLAN_SEC_WPA2
unsigned char _auth = WLAN_SEC_WPA2;

unsigned char wlan_profile_index;

volatile uint8_t SPARK_WLAN_RESET;
volatile uint8_t SPARK_WLAN_SLEEP;
volatile uint8_t SPARK_WLAN_STARTED;
volatile uint8_t SPARK_LED_FADE = 1;

volatile uint8_t Spark_Error_Count;

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
    if (ssid) {
        NetworkCredentials creds;
        memset(&creds, 0, sizeof(creds));
        creds.len = sizeof(creds);
        creds.ssid = ssid;
        creds.password = password;
        creds.ssid_len = strlen(ssid);
        creds.password_len = strlen(password);
        creds.security = WLanSecurityType(security_type);
        network_set_credentials(&creds);
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
	WLAN_DHCP = 0;
	WLAN_CAN_SHUTDOWN = 0;

	SPARK_CLOUD_SOCKETED = 0;
	SPARK_CLOUD_CONNECTED = 0;
	SPARK_LED_FADE = 0;

	LED_SetRGBColor(RGB_COLOR_BLUE);
	LED_On(LED_RGB);

	/* If WiFi module is connected, disconnect it */
	network_disconnect();

	/* If WiFi module is powered off, turn it on */
	network_on();

	wlan_smart_config_init();

	WiFiCredentialsReader wifi_creds_reader(wifi_add_profile_callback);

	/* Wait for SmartConfig/SerialConfig to finish */
	while (network_listening())
	{
		if(WLAN_DELETE_PROFILES)
		{
			int toggle = 25;
			while(toggle--)
			{
				LED_Toggle(LED_RGB);
				HAL_Delay_Milliseconds(50);
			}
			network_clear_credentials();
			WLAN_DELETE_PROFILES = 0;
		}
		else
		{
			LED_Toggle(LED_RGB);
			HAL_Delay_Milliseconds(250);
			wifi_creds_reader.read();
		}
	}

	LED_On(LED_RGB);

    wlan_smart_config_finalize();
    
	if(WLAN_SMART_CONFIG_FINISHED)
	{
		/* Decrypt configuration information and add profile */
		SPARK_WLAN_SmartConfigProcess();
	}

	network_connect();

	WLAN_SMART_CONFIG_START = 0;
}


void SPARK_WLAN_Setup(void (*presence_announcement_callback)(void))
{
    announce_presence = presence_announcement_callback;

    wlan_setup();

    /* Trigger a WLAN device */
    if (system_mode() == AUTOMATIC)
    {
        network_connect();
    }

    //Initialize spark protocol callbacks for all System modes
    Spark_Protocol_Init();
}

static int cfod_count = 0;

/**
 * Reset or initialize the network connection as required.
 */
void manage_network_connection()
{
  if (SPARK_WLAN_RESET || SPARK_WLAN_SLEEP || WLAN_WD_TO())
  {
    if (SPARK_WLAN_STARTED)
    {
      DEBUG("Resetting WLAN!");
      CLR_WLAN_WD();
      WLAN_CONNECTED = 0;
      WLAN_DHCP = 0;
      SPARK_WLAN_RESET = 0;
      SPARK_WLAN_STARTED = 0;
      SPARK_CLOUD_SOCKETED = 0;
      SPARK_CLOUD_CONNECTED = 0;
      Spark_Error_Count = 0;
      cfod_count = 0;

      network_off();
    }
  }
  else
  {
    if (!SPARK_WLAN_STARTED)
    {
      if (!WLAN_DISCONNECT)
      {
        ARM_WLAN_WD(CONNECT_TO_ADDRESS_MAX);
      }
      network_connect();
    }
    }    
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
  if (WLAN_DHCP && !SPARK_WLAN_SLEEP)
  {
    if (ip_config.aucIP[0] == 0)
    {
      HAL_Delay_Milliseconds(100);
      wlan_fetch_ipconfig(&ip_config);
    }
  }
  else if (ip_config.aucIP[0] != 0)
  {
    memset(&ip_config, 0, sizeof(ip_config));
    }    
}

void disconnect_cloud()
{
    if (SPARK_CLOUD_SOCKETED || SPARK_CLOUD_CONNECTED)
    {
      Spark_Disconnect();

      SPARK_FLASH_UPDATE = 0;
      SPARK_CLOUD_CONNECTED = 0;
      SPARK_CLOUD_SOCKETED = 0;

      if(!WLAN_DISCONNECT)
      {
        LED_SetRGBColor(RGB_COLOR_GREEN);
        LED_On(LED_RGB);
      }
    }    
}

void handle_cloud_errors()
{
      LED_SetRGBColor(RGB_COLOR_RED);

      while (Spark_Error_Count != 0)
      {
        LED_On(LED_RGB);
        HAL_Delay_Milliseconds(500);
        LED_Off(LED_RGB);
        HAL_Delay_Milliseconds(500);
        Spark_Error_Count--;
      }

      // TODO Send the Error Count to Cloud: NVMEM_Spark_File_Data[ERROR_COUNT_FILE_OFFSET]

      // Reset Error Count
      wlan_set_error_count(0);
    }

void handle_cfod()
{
      if ((cfod_count += RESET_ON_CFOD) == MAX_FAILED_CONNECTS)
      {
        SPARK_WLAN_RESET = RESET_ON_CFOD;
        ERROR("Resetting CC3000 due to %d failed connect attempts", MAX_FAILED_CONNECTS);
      }

      if (Internet_Test() < 0)
      {
        // No Internet Connection
        if ((cfod_count += RESET_ON_CFOD) == MAX_FAILED_CONNECTS)
        {
          SPARK_WLAN_RESET = RESET_ON_CFOD;
          ERROR("Resetting CC3000 due to %d failed connect attempts", MAX_FAILED_CONNECTS);
        }

        Spark_Error_Count = 2;
      }
      else
      {
        // Cloud not Reachable
        Spark_Error_Count = 3;
    }    
}

/**
 * Establishes a socket connection to the cloud if not already present.
 * - handles previous connection errors by flashing the LED
 * - attempts to open a socket to the cloud
 * - handles the CFOD
 * 
 * On return, SPARK_CLOUD_SOCKETED is set to true if the socket connection was successful.
 */

void establish_cloud_connection()
{
    if (WLAN_DHCP && !SPARK_WLAN_SLEEP && !SPARK_CLOUD_SOCKETED)
    {    
        if (Spark_Error_Count)
            handle_cloud_errors();

        SPARK_LED_FADE = 0;
        LED_SetRGBColor(RGB_COLOR_CYAN);
        LED_On(LED_RGB);

        if (Spark_Connect() >= 0)
        {
            cfod_count  = 0;
            SPARK_CLOUD_SOCKETED = 1;
      }
        else
        {
            SPARK_CLOUD_SOCKETED = 0;
            if (!SPARK_WLAN_RESET)
                handle_cfod();
      wlan_set_error_count(Spark_Error_Count);
    }
    }    
}

/**
 * Manages the handshake and cloud events when the cloud has a socket connected.
 * @param force_events
 */
void handle_cloud_connection(bool force_events)
{
  if (SPARK_CLOUD_SOCKETED)
  {
    if (!SPARK_CLOUD_CONNECTED)
    {
      int err = Spark_Handshake();
      if (err)
      {
        if (0 > err)
        {
          // Wrong key error, red
          LED_SetRGBColor(RGB_COLOR_RED);
        }
        else if (1 == err)
        {
          // RSA decryption error, orange
          LED_SetRGBColor(RGB_COLOR_ORANGE);
        }
        else if (2 == err)
        {
          // RSA signature verification error, magenta
          LED_SetRGBColor(RGB_COLOR_MAGENTA);
        }

        LED_On(LED_RGB);
      }
      else
      {
        SPARK_CLOUD_CONNECTED = 1;
      }
    }

    if(SPARK_FLASH_UPDATE || force_events || System.mode() != MANUAL)
    {
      Spark_Process_Events();
    }
  }
}

void Spark_Idle_Events(bool force_events/*=false*/)
{  
    HAL_Notify_WDT();

    ON_EVENT_DELTA();
    spark_loop_total_millis = 0;

    manage_network_connection();

    manage_smart_config();

    manage_ip_config();

    if (SPARK_CLOUD_CONNECT == 0)
    {
        disconnect_cloud();        
    }
    else // cloud connection is wanted
    {
        establish_cloud_connection();

        handle_cloud_connection(force_events);
    }
}

void HAL_WLAN_notify_simple_config_done() 
{       
    WLAN_SMART_CONFIG_FINISHED = 1;
    WLAN_SMART_CONFIG_STOP = 1;    
}

void HAL_WLAN_notify_connected()
{
    WLAN_CONNECTED = 1;
    if(!WLAN_DISCONNECT)
    {
        ARM_WLAN_WD(CONNECT_TO_ADDRESS_MAX);
    }
}
    
void HAL_WLAN_notify_disconnected()
{
    if (WLAN_CONNECTED && !WLAN_DISCONNECT)
    {
        ARM_WLAN_WD(DISCONNECT_TO_RECONNECT);
    }
    WLAN_CONNECTED = 0;
    WLAN_DHCP = 0;
    SPARK_CLOUD_SOCKETED = 0;
    SPARK_CLOUD_CONNECTED = 0;
    SPARK_LED_FADE = 1;
    LED_SetRGBColor(RGB_COLOR_BLUE);
    LED_On(LED_RGB);
    Spark_Error_Count = 0;
}

void HAL_WLAN_notify_dhcp(bool dhcp) 
{
    if (dhcp) {
        CLR_WLAN_WD();
        WLAN_DHCP = 1;
        SPARK_LED_FADE = 1;
        LED_SetRGBColor(RGB_COLOR_GREEN);
        LED_On(LED_RGB);
    }
    else {
        WLAN_DHCP = 0;
    }    
}

void HAL_WLAN_notify_can_shutdown()
{
    WLAN_CAN_SHUTDOWN = 1;
}

void HAL_WLAN_notify_socket_closed(sock_handle_t socket)
{
    if (socket==sparkSocket) {
        SPARK_CLOUD_CONNECTED = 0;
        SPARK_CLOUD_SOCKETED = 0;
    }    
}

void network_connect()
{
    if (!network_ready()) {
        WLAN_DISCONNECT = 0;
        wlan_connect_init();
        SPARK_WLAN_STARTED = 1;
        SPARK_WLAN_SLEEP = 0;

        if (wlan_reset_credentials_store_required()) {
            wlan_reset_credentials_store();
        }

        if (!network_has_credentials()) {
            network_listen();
        }
        else {
            SPARK_LED_FADE = 0;
            LED_SetRGBColor(RGB_COLOR_GREEN);
            LED_On(LED_RGB);
            wlan_connect_finalize();
        }

        Set_NetApp_Timeout();
    }    
}

void network_disconnect()
{
    if (network_ready()) {
        WLAN_DISCONNECT = 1;//Do not ARM_WLAN_WD() in WLAN_Async_Callback()
        SPARK_CLOUD_CONNECT = 0;
        wlan_disconnect_now();
    }
}

bool network_ready()
{
    return (SPARK_WLAN_STARTED && WLAN_DHCP);    
}    

bool network_connecting() 
{
    return (SPARK_WLAN_STARTED && !WLAN_DHCP);
}

void network_on()
{
    if (!SPARK_WLAN_STARTED) {
        wlan_activate();
        SPARK_WLAN_STARTED = 1;
        SPARK_WLAN_SLEEP = 0;
        SPARK_LED_FADE = 1;
        LED_SetRGBColor(RGB_COLOR_BLUE);
        LED_On(LED_RGB);
    }    
}

inline bool network_has_credentials() 
{
    return wlan_has_credentials() == 0;
}

void network_off() 
{
    if (SPARK_WLAN_STARTED) {
        wlan_deactivate();

        if(!SPARK_WLAN_SLEEP)//if Spark.sleep() is not called
        {
            // Reset remaining state variables in Spark_Idle()
            SPARK_WLAN_SLEEP = 1;

            // Do not automatically connect to the cloud
            // the next time we connect to a Wi-Fi network
            SPARK_CLOUD_CONNECT = 0;
        }

        SPARK_WLAN_STARTED = 0;
        WLAN_DHCP = 0;
        SPARK_LED_FADE = 1;
        LED_SetRGBColor(RGB_COLOR_WHITE);
        LED_On(LED_RGB);
    }
    
}

void network_listen()
{
    WLAN_SMART_CONFIG_START = 1;    
}

bool network_listening()
{
    if (WLAN_SMART_CONFIG_START && !(WLAN_SMART_CONFIG_FINISHED || WLAN_SERIAL_CONFIG_DONE)) {
        return true;
    }
    return false;
}

void network_set_credentials(NetworkCredentials* credentials) {
    
    if (!SPARK_WLAN_STARTED || !credentials) {
        return;
    }

    WLanSecurityType security = credentials->security;
    
    if (0 == credentials->password[0]) {
        security = WLAN_SEC_UNSEC;
    }

    char buf[14];
    if (security == WLAN_SEC_WEP) {
        // Get WEP key from string, needs converting
        credentials->password_len = (strlen(credentials->password) / 2); // WEP key length in bytes
        char byteStr[3];
        byteStr[2] = '\0';
        memset(buf, 0, sizeof(buf));
        for (uint32_t i = 0; i < credentials->password_len; i++) { // Basic loop to convert text-based WEP key to byte array, can definitely be improved
            byteStr[0] = credentials->password[2 * i];
            byteStr[1] = credentials->password[(2 * i) + 1];
            buf[i] = strtoul(byteStr, NULL, 16);
        }
        credentials->password = buf;
    }
    credentials->security = security;

    wlan_set_credentials(credentials);
}


bool network_clear_credentials(void) {
    return wlan_clear_credentials() == 0;
}



/*
 * @brief This should block for a certain number of milliseconds and also execute spark_wlan_loop
 */
void spark_delay_ms(unsigned long ms)
{
  volatile system_tick_t spark_loop_elapsed_millis = SPARK_LOOP_DELAY_MILLIS;
  spark_loop_total_millis += ms;

  volatile system_tick_t last_millis = HAL_Timer_Get_Milli_Seconds();

  while (1)
  {
    HAL_Notify_WDT();

    volatile system_tick_t current_millis = HAL_Timer_Get_Milli_Seconds();
    volatile system_tick_t elapsed_millis = current_millis - last_millis;

    //Check for wrapping
    if (elapsed_millis >= 0x80000000)
    {
      elapsed_millis = last_millis + current_millis;
    }

    if (elapsed_millis >= ms)
    {
      break;
    }

    if (SPARK_WLAN_SLEEP)
    {
      //Do not yield for Spark_Idle()
    }
    else if ((elapsed_millis >= spark_loop_elapsed_millis) || (spark_loop_total_millis >= SPARK_LOOP_DELAY_MILLIS))
    {
      spark_loop_elapsed_millis = elapsed_millis + SPARK_LOOP_DELAY_MILLIS;
      //spark_loop_total_millis is reset to 0 in Spark_Idle()
      do
      {
        //Run once if the above condition passes
        Spark_Idle();
      }
      while (SPARK_FLASH_UPDATE);//loop during OTA update
    }
  }
}
