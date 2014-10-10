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
#include "spark_wlan.h"
#include "spark_macros.h"
#include "string.h"
#include "wifi_credentials_reader.h"
#include "system_tick_hal.h"
#include "watchdog_hal.h"
#include "wlan_hal.h"
#include "delay_hal.h"
#include "rgbled.h"
#include "spark_wiring_wifi.h"

using namespace spark;

WLanConfig ip_config;

uint32_t wlan_watchdog;

//#define DEBUG_WIFI    // Define to show all the flags in debug output
//#define DEBUG_WAN_WD  // Define to show all SW WD activity in debug output


volatile uint8_t WLAN_DISCONNECT;
volatile uint8_t WLAN_MANUAL_CONNECT = 0; //For Manual connection, set this to 1
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
volatile uint8_t SPARK_CLOUD_CONNECT = 1; //default is AUTOMATIC mode
volatile uint8_t SPARK_CLOUD_SOCKETED;
volatile uint8_t SPARK_CLOUD_CONNECTED;
volatile uint8_t SPARK_FLASH_UPDATE;
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
    WiFi.setCredentials(ssid, strlen(ssid), password, strlen(password), security_type);
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
	SPARK_FLASH_UPDATE = 0;
	SPARK_LED_FADE = 0;

	LED_SetRGBColor(RGB_COLOR_BLUE);
	LED_On(LED_RGB);

	/* If WiFi module is connected, disconnect it */
	WiFi.disconnect();

	/* If WiFi module is powered off, turn it on */
	WiFi.on();

	wlan_smart_config_init();

	WiFiCredentialsReader wifi_creds_reader(wifi_add_profile_callback);

	/* Wait for SmartConfig/SerialConfig to finish */
	while (WiFi.listening())
	{
		if(WLAN_DELETE_PROFILES)
		{
			int toggle = 25;
			while(toggle--)
			{
				LED_Toggle(LED_RGB);
				HAL_Delay_Milliseconds(50);
			}
			WiFi.clearCredentials();
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

	WiFi.connect();

	WLAN_SMART_CONFIG_START = 0;
}


void SPARK_WLAN_Setup(void (*presence_announcement_callback)(void))
{
    announce_presence = presence_announcement_callback;

    wlan_setup();

    /* Trigger a WLAN device */
    if (System.mode() == AUTOMATIC)
    {
        WiFi.connect();
    }

    //Initialize spark protocol callbacks for all System modes
    Spark_Protocol_Init();
}

void SPARK_WLAN_Loop(void)
{
  static int cfod_count = 0;
  HAL_Notify_WDT();

  ON_EVENT_DELTA();
  spark_loop_total_millis = 0;

  if (SPARK_WLAN_RESET || SPARK_WLAN_SLEEP || WLAN_WD_TO())
  {
    if (SPARK_WLAN_STARTED)
    {
      DEBUG("Resetting CC3000!");
      CLR_WLAN_WD();
      WLAN_CONNECTED = 0;
      WLAN_DHCP = 0;
      SPARK_WLAN_RESET = 0;
      SPARK_WLAN_STARTED = 0;
      SPARK_CLOUD_SOCKETED = 0;
      SPARK_CLOUD_CONNECTED = 0;
      SPARK_FLASH_UPDATE = 0;
      Spark_Error_Count = 0;
      cfod_count = 0;

      WiFi.off();
    }
  }
  else
  {
    if (!SPARK_WLAN_STARTED)
    {
      if (!WLAN_MANUAL_CONNECT && !WLAN_DISCONNECT)
      {
        ARM_WLAN_WD(CONNECT_TO_ADDRESS_MAX);
      }
      WiFi.connect();
    }
  }

  if (WLAN_SMART_CONFIG_START)
  {
    Start_Smart_Config();
  }
  else if (WLAN_MANUAL_CONNECT && !WLAN_DHCP)
  {
    CLR_WLAN_WD();
    WiFi.disconnect();
    wlan_manual_connect();
    WLAN_MANUAL_CONNECT = 0;
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

  if (SPARK_CLOUD_CONNECT == 0)
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

    return;
  }

  if (WLAN_DHCP && !SPARK_WLAN_SLEEP && !SPARK_CLOUD_SOCKETED)
  {
    if (Spark_Error_Count)
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
      wlan_clear_error_count();
    }

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
      if (SPARK_WLAN_RESET)
      {
        return;
      }

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

      wlan_set_error_count(Spark_Error_Count);
      SPARK_CLOUD_SOCKETED = 0;
    }
  }

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

    if(SPARK_FLASH_UPDATE || System.mode() != MANUAL)
    {
      Spark.process();
    }
  }
}

void HAL_WLAN_notify_simple_config_done() 
{       
    WLAN_SMART_CONFIG_FINISHED = 1;
    WLAN_SMART_CONFIG_STOP = 1;
    WLAN_MANUAL_CONNECT = 0;
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
    SPARK_FLASH_UPDATE = 0;
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
        SPARK_FLASH_UPDATE = 0;
        SPARK_CLOUD_CONNECTED = 0;
        SPARK_CLOUD_SOCKETED = 0;
    }    
}
