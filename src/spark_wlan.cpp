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

//#define DEBUG_WIFI    // Define to show all the flags in debug output
//#define DEBUG_WAN_WD  // Define to show all SW WD activity in debug output

#if defined(DEBUG_WIFI)
static uint32_t lastEvent = 0;
#define SET_LAST_EVENT(x) do {lastEvent = (x);} while(0)
#define GET_LAST_EVENT(x) do { x = lastEvent; lastEvent = 0;} while(0)
#define DUMP_STATE() do { \
    DEBUG("\r\nWLAN_MANUAL_CONNECT=%d\r\nWLAN_DELETE_PROFILES=%d\r\nWLAN_SMART_CONFIG_START=%d\r\nWLAN_SMART_CONFIG_STOP=%d", \
          WLAN_MANUAL_CONNECT,WLAN_DELETE_PROFILES,WLAN_SMART_CONFIG_START, WLAN_SMART_CONFIG_STOP); \
    DEBUG("\r\nWLAN_SMART_CONFIG_FINISHED=%d\r\nWLAN_SERIAL_CONFIG_DONE=%d\r\nWLAN_CONNECTED=%d\r\nWLAN_DHCP=%d\r\nWLAN_CAN_SHUTDOWN=%d", \
          WLAN_SMART_CONFIG_FINISHED,WLAN_SERIAL_CONFIG_DONE,WLAN_CONNECTED,WLAN_DHCP,WLAN_CAN_SHUTDOWN); \
    DEBUG("\r\nSPARK_WLAN_RESET=%d\r\nSPARK_WLAN_SLEEP=%d\r\nSPARK_WLAN_STARTED=%d\r\nSPARK_SOCKET_HANDSHAKE=%d", \
           SPARK_WLAN_RESET,SPARK_WLAN_SLEEP,SPARK_WLAN_STARTED,SPARK_SOCKET_HANDSHAKE); \
    DEBUG("\r\nSPARK_SOCKET_CONNECTED=%d\r\nSPARK_HANDSHAKE_COMPLETED=%d\r\nSPARK_FLASH_UPDATE=%d\r\nSPARK_LED_FADE=%d\r\n", \
           SPARK_SOCKET_CONNECTED,SPARK_HANDSHAKE_COMPLETED,SPARK_FLASH_UPDATE,SPARK_LED_FADE); \
 } while(0)

#define ON_EVENT_DELTA()  do { if (lastEvent != 0) { uint32_t l; GET_LAST_EVENT(l); DEBUG("\r\nAsyncEvent 0x%04x", l); DUMP_STATE();}} while(0)
#else
#define SET_LAST_EVENT(x)
#define GET_LAST_EVENT(x)
#define DUMP_STATE()
#define ON_EVENT_DELTA()
#endif
tNetappIpconfigRetArgs ip_config;

volatile int8_t  WLAN_MANUAL_CONNECT = 0; //For Manual connection, set this to 1
volatile uint8_t WLAN_DELETE_PROFILES;
volatile uint8_t WLAN_SMART_CONFIG_START;
volatile uint8_t WLAN_SMART_CONFIG_STOP;
volatile uint8_t WLAN_SMART_CONFIG_FINISHED;
volatile uint8_t WLAN_SERIAL_CONFIG_DONE;
volatile uint8_t WLAN_CONNECTED;
volatile uint8_t WLAN_DHCP;
volatile uint8_t WLAN_CAN_SHUTDOWN;

enum eWanTimings {
  CONNECT_TO_ADDRESS_MAX = S2M(30),
  DISCONNECT_TO_RECONNECT = S2M(30),
};

#if defined(DEBUG_WAN_WD)
#define WAN_WD_DEBUG(x,...) DEBUG(x,__VA_ARGS__)
#else
#define WAN_WD_DEBUG(x,...)
#endif
uint32_t wlan_watchdog = 0;
#define ARM_WLAN_WD(x) do { wlan_watchdog = millis()+(x); WAN_WD_DEBUG("WD Set "#x" %d",(x));}while(0)
#define WLAN_WD_TO() (wlan_watchdog && (millis() >= wlan_watchdog))
#define CLR_WLAN_WD() do { wlan_watchdog = 0; WAN_WD_DEBUG("WD Cleared, was %d",wlan_watchdog);;}while(0)


void (*announce_presence)(void);

unsigned char patchVer[2];

/* Smart Config Prefix */
char aucCC3000_prefix[] = {'T', 'T', 'T'};
/* AES key "sparkdevices2013" */
const unsigned char smartconfigkey[] = "sparkdevices2013";	//16 bytes
/* device name used by smart config response */
char device_name[] = "CC3000";

/* Manual connect credentials; only used if WLAN_MANUAL_CONNECT == 1 */
char _ssid[] = "ssid";
char _password[] = "password";
// Auth options are WLAN_SEC_UNSEC, WLAN_SEC_WPA, WLAN_SEC_WEP, and WLAN_SEC_WPA2
unsigned char _auth = WLAN_SEC_WPA2;

unsigned char wlan_profile_index;

unsigned char NVMEM_Spark_File_Data[NVMEM_SPARK_FILE_SIZE];

volatile uint8_t SPARK_WLAN_RESET;
volatile uint8_t SPARK_WLAN_SLEEP;
volatile uint8_t SPARK_WLAN_STARTED;
volatile uint8_t SPARK_SOCKET_HANDSHAKE;
volatile uint8_t SPARK_SOCKET_CONNECTED;
volatile uint8_t SPARK_HANDSHAKE_COMPLETED;
volatile uint8_t SPARK_FLASH_UPDATE;
volatile uint8_t SPARK_LED_FADE;

volatile uint8_t Spark_Error_Count;


void Set_NetApp_Timeout(void)
{
	unsigned long aucDHCP = 14400;
	unsigned long aucARP = 3600;
	unsigned long aucKeepalive = 10;
	unsigned long aucInactivity = DEFAULT_SEC_INACTIVITY;
	SPARK_WLAN_SetNetWatchDog(S2M(DEFAULT_SEC_NETOPS)+ (DEFAULT_SEC_INACTIVITY ? 250 : 0) );
	netapp_timeout_values(&aucDHCP, &aucARP, &aucKeepalive, &aucInactivity);
}

void Clear_NetApp_Dhcp(void)
{
	// Clear out the DHCP settings
	unsigned long pucSubnetMask = 0;
	unsigned long pucIP_Addr = 0;
	unsigned long pucIP_DefaultGWAddr = 0;
	unsigned long pucDNS = 0;

	netapp_dhcp(&pucIP_Addr, &pucSubnetMask, &pucIP_DefaultGWAddr, &pucDNS);
}

void wifi_add_profile_callback(const char *ssid,
                               const char *password,
                               unsigned long security_type)
{
  if (0 == password[0]) {
    security_type = WLAN_SEC_UNSEC;
  }

  wlan_profile_index = wlan_add_profile(security_type, (unsigned char *)ssid, strlen(ssid), NULL, 1, 0x18, 0x1e, 2, (unsigned char *)password, strlen(password));

  WLAN_SERIAL_CONFIG_DONE = 1;
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

	SPARK_SOCKET_CONNECTED = 0;
	SPARK_HANDSHAKE_COMPLETED = 0;
	SPARK_FLASH_UPDATE = 0;
	SPARK_LED_FADE = 0;

	LED_SetRGBColor(RGB_COLOR_BLUE);
	LED_On(LED_RGB);

	/* Reset all the previous configuration */
	wlan_ioctl_set_connection_policy(DISABLE, DISABLE, DISABLE);

	NVMEM_Spark_File_Data[WLAN_POLICY_FILE_OFFSET] = 0;
	nvmem_write(NVMEM_SPARK_FILE_ID, 1, WLAN_POLICY_FILE_OFFSET, &NVMEM_Spark_File_Data[WLAN_POLICY_FILE_OFFSET]);

	/* Wait until CC3000 is disconnected */
	while (WLAN_CONNECTED == 1)
	{
		//Delay 100ms
		Delay(100);
		hci_unsolicited_event_handler();
	}

	/* Create new entry for AES encryption key */
	nvmem_create_entry(NVMEM_AES128_KEY_FILEID,16);

	/* Write AES key to NVMEM */
	aes_write_key((unsigned char *)(&smartconfigkey[0]));

	wlan_smart_config_set_prefix((char*)aucCC3000_prefix);

	/* Start the SmartConfig start process */
	wlan_smart_config_start(1);

	WiFiCredentialsReader wifi_creds_reader(wifi_add_profile_callback);

	/* Wait for SmartConfig/SerialConfig to finish */
	while (!(WLAN_SMART_CONFIG_FINISHED | WLAN_SERIAL_CONFIG_DONE))
	{
		if(WLAN_DELETE_PROFILES && wlan_ioctl_del_profile(255) == 0)
		{
			int toggle = 25;
			while(toggle--)
			{
				LED_Toggle(LED_RGB);
				Delay(50);
			}
			NVMEM_Spark_File_Data[WLAN_PROFILE_FILE_OFFSET] = 0;
			nvmem_write(NVMEM_SPARK_FILE_ID, 1, WLAN_PROFILE_FILE_OFFSET, &NVMEM_Spark_File_Data[WLAN_PROFILE_FILE_OFFSET]);
			WLAN_DELETE_PROFILES = 0;
		}
		else
		{
			LED_Toggle(LED_RGB);
			Delay(250);
			wifi_creds_reader.read();
		}
	}

	LED_On(LED_RGB);

	/* read count of wlan profiles stored */
	nvmem_read(NVMEM_SPARK_FILE_ID, 1, WLAN_PROFILE_FILE_OFFSET, &NVMEM_Spark_File_Data[WLAN_PROFILE_FILE_OFFSET]);

//	if(NVMEM_Spark_File_Data[WLAN_PROFILE_FILE_OFFSET] >= 7)
//	{
//		if(wlan_ioctl_del_profile(255) == 0)
//			NVMEM_Spark_File_Data[WLAN_PROFILE_FILE_OFFSET] = 0;
//	}

	if(WLAN_SMART_CONFIG_FINISHED)
	{
		/* Decrypt configuration information and add profile */
		wlan_profile_index = wlan_smart_config_process();
	}

	if(wlan_profile_index != -1)
	{
		NVMEM_Spark_File_Data[WLAN_PROFILE_FILE_OFFSET] = wlan_profile_index + 1;
	}

	/* write count of wlan profiles stored */
	nvmem_write(NVMEM_SPARK_FILE_ID, 1, WLAN_PROFILE_FILE_OFFSET, &NVMEM_Spark_File_Data[WLAN_PROFILE_FILE_OFFSET]);

	/* Configure to connect automatically to the AP retrieved in the Smart config process */
	wlan_ioctl_set_connection_policy(DISABLE, DISABLE, ENABLE);

	NVMEM_Spark_File_Data[WLAN_POLICY_FILE_OFFSET] = 1;
	nvmem_write(NVMEM_SPARK_FILE_ID, 1, WLAN_POLICY_FILE_OFFSET, &NVMEM_Spark_File_Data[WLAN_POLICY_FILE_OFFSET]);

	/* Reset the CC3000 */
	wlan_stop();

	Delay(100);

	wlan_start(0);

	SPARK_WLAN_STARTED = 1;

	/* Mask out all non-required events */
	wlan_set_event_mask(HCI_EVNT_WLAN_KEEPALIVE | HCI_EVNT_WLAN_UNSOL_INIT | HCI_EVNT_WLAN_ASYNC_PING_REPORT);

    LED_SetRGBColor(RGB_COLOR_GREEN);
	LED_On(LED_RGB);

	Set_NetApp_Timeout();

	WLAN_SMART_CONFIG_START = 0;
}

/* WLAN Application related callbacks passed to wlan_init */
void WLAN_Async_Callback(long lEventType, char *data, unsigned char length)
{
        SET_LAST_EVENT(lEventType);
	switch (lEventType)
	{
	        default:
	          break;

		case HCI_EVNT_WLAN_ASYNC_SIMPLE_CONFIG_DONE:
			WLAN_SMART_CONFIG_FINISHED = 1;
			WLAN_SMART_CONFIG_STOP = 1;
			WLAN_MANUAL_CONNECT = 0;
			break;

		case HCI_EVNT_WLAN_UNSOL_CONNECT:
			WLAN_CONNECTED = 1;
  		        ARM_WLAN_WD(CONNECT_TO_ADDRESS_MAX);
			break;

		case HCI_EVNT_WLAN_UNSOL_DISCONNECT:
			if(WLAN_CONNECTED)
			{
	                        ARM_WLAN_WD(DISCONNECT_TO_RECONNECT);
				LED_RGB_OVERRIDE = 0;
				LED_SetRGBColor(RGB_COLOR_GREEN);
				LED_On(LED_RGB);
			}
			else
			{
				if(NVMEM_Spark_File_Data[WLAN_PROFILE_FILE_OFFSET] != 0)
				{
					NVMEM_Spark_File_Data[WLAN_PROFILE_FILE_OFFSET] -= 1;
				}
				else
				{
					WLAN_SMART_CONFIG_START = 1;
				}
			}
			if (WLAN_MANUAL_CONNECT == -1) {
			    WLAN_MANUAL_CONNECT = 1;
			}
			WLAN_CONNECTED = 0;
			WLAN_DHCP = 0;
			SPARK_SOCKET_CONNECTED = 0;
			SPARK_HANDSHAKE_COMPLETED = 0;
			SPARK_FLASH_UPDATE = 0;
			SPARK_LED_FADE = 0;
			Spark_Error_Count = 0;
			break;

		case HCI_EVNT_WLAN_UNSOL_DHCP:
			if (*(data + 20) == 0)
			{
				WLAN_DHCP = 1;
				CLR_WLAN_WD();
				LED_SetRGBColor(RGB_COLOR_GREEN);
				LED_On(LED_RGB);
			}
			else
			{
				WLAN_DHCP = 0;
			}
			break;

		case HCI_EVENT_CC3000_CAN_SHUT_DOWN:
			WLAN_CAN_SHUTDOWN = 1;
			break;

		case HCI_EVNT_BSD_TCP_CLOSE_WAIT:
                      long socket = -1;
		      STREAM_TO_UINT32(data,0,socket);
		      set_socket_active_status(socket, SOCKET_STATUS_INACTIVE);
  		      if(socket == sparkSocket)
		      {
			SPARK_FLASH_UPDATE = 0;
			SPARK_LED_FADE = 0;
			SPARK_HANDSHAKE_COMPLETED = 0;
			SPARK_SOCKET_CONNECTED = 0;
 		      }
		    break;
	}
}

char *WLAN_Firmware_Patch(unsigned long *length)
{
	*length = 0;
	return NULL;
}

char *WLAN_Driver_Patch(unsigned long *length)
{
	*length = 0;
	return NULL;
}

char *WLAN_BootLoader_Patch(unsigned long *length)
{
	*length = 0;
	return NULL;
}

int SPARK_WLAN_hasAddress(void)
{
  return WLAN_DHCP || WLAN_MANUAL_CONNECT != 0;
}

uint32_t SPARK_WLAN_SetNetWatchDog(uint32_t timeOutInMS)
{
  uint32_t rv = cc3000__event_timeout_ms;
  cc3000__event_timeout_ms = timeOutInMS;
  return rv;
}


void SPARK_WLAN_Setup(void (*presence_announcement_callback)(void))
{
	announce_presence = presence_announcement_callback;

	/* Initialize CC3000's CS, EN and INT pins to their default states */
	CC3000_WIFI_Init();

	/* Configure & initialize CC3000 SPI_DMA Interface */
	CC3000_SPI_DMA_Init();

	/* WLAN On API Implementation */
	wlan_init(WLAN_Async_Callback, WLAN_Firmware_Patch, WLAN_Driver_Patch, WLAN_BootLoader_Patch,
				CC3000_Read_Interrupt_Pin, CC3000_Interrupt_Enable, CC3000_Interrupt_Disable, CC3000_Write_Enable_Pin);

	Delay(100);

	/* Trigger a WLAN device */
	wlan_start(0);

	SPARK_LED_FADE = 0;

	SPARK_WLAN_STARTED = 1;

	/* Mask out all non-required events from CC3000 */
	wlan_set_event_mask(HCI_EVNT_WLAN_KEEPALIVE | HCI_EVNT_WLAN_UNSOL_INIT | HCI_EVNT_WLAN_ASYNC_PING_REPORT);

	if(NVMEM_SPARK_Reset_SysFlag == 0x0001 || nvmem_read(NVMEM_SPARK_FILE_ID, NVMEM_SPARK_FILE_SIZE, 0, NVMEM_Spark_File_Data) != NVMEM_SPARK_FILE_SIZE)
	{
		/* Delete all previously stored wlan profiles */
		wlan_ioctl_del_profile(255);

		/* Create new entry for Spark File in CC3000 EEPROM */
		nvmem_create_entry(NVMEM_SPARK_FILE_ID, NVMEM_SPARK_FILE_SIZE);

		memset(NVMEM_Spark_File_Data,0, arraySize(NVMEM_Spark_File_Data));

		nvmem_write(NVMEM_SPARK_FILE_ID, NVMEM_SPARK_FILE_SIZE, 0, NVMEM_Spark_File_Data);

		NVMEM_SPARK_Reset_SysFlag = 0x0000;
		Save_SystemFlags();
	}

	if(WLAN_MANUAL_CONNECT == 0)
	{
		if(NVMEM_Spark_File_Data[WLAN_PROFILE_FILE_OFFSET] == 0)
		{
			WLAN_SMART_CONFIG_START = 1;
		}
		else if(NVMEM_Spark_File_Data[WLAN_POLICY_FILE_OFFSET] == 0)
		{
			wlan_ioctl_set_connection_policy(DISABLE, DISABLE, ENABLE);

			NVMEM_Spark_File_Data[WLAN_POLICY_FILE_OFFSET] = 1;
			nvmem_write(NVMEM_SPARK_FILE_ID, 1, WLAN_POLICY_FILE_OFFSET, &NVMEM_Spark_File_Data[WLAN_POLICY_FILE_OFFSET]);
		}
	}

	if((WLAN_MANUAL_CONNECT > 0) || !WLAN_SMART_CONFIG_START)
	{
		LED_SetRGBColor(RGB_COLOR_GREEN);
		LED_On(LED_RGB);
	}

	nvmem_read_sp_version(patchVer);
	if (patchVer[1] == 24)//19 for old patch
	{
		/* Latest Patch Available after flashing "cc3000-patch-programmer.bin" */
	}

	Clear_NetApp_Dhcp();

	Set_NetApp_Timeout();
}

void SPARK_WLAN_Loop(void)
{
        static int cofd_count = 0;
        ON_EVENT_DELTA();

        if(SPARK_WLAN_RESET || SPARK_WLAN_SLEEP || WLAN_WD_TO())
	{
		if(SPARK_WLAN_STARTED)
		{
			if (LED_RGB_OVERRIDE)
			{
				LED_Signaling_Stop();
			}
			DEBUG("Resetting CC3000!");
			CLR_WLAN_WD();
			WLAN_CONNECTED = 0;
			WLAN_DHCP = 0;
			SPARK_WLAN_RESET = 0;
			SPARK_WLAN_STARTED = 0;
			SPARK_SOCKET_CONNECTED = 0;
			SPARK_HANDSHAKE_COMPLETED = 0;
			SPARK_FLASH_UPDATE = 0;
			SPARK_LED_FADE = 0;
			Spark_Error_Count = 0;
			cofd_count = 0;

			wlan_stop();
			Delay(100);

			if(WLAN_SMART_CONFIG_START)
			{
				//Workaround to enter smart config when socket connect had blocked
				wlan_start(0);

				SPARK_WLAN_STARTED = 1;

				/* Start CC3000 Smart Config Process */
				Start_Smart_Config();
			}

			LED_SetRGBColor(RGB_COLOR_GREEN);
			LED_On(LED_RGB);
		}
	}
	else
	{
		if(!SPARK_WLAN_STARTED)
		{
                      if (WLAN_MANUAL_CONNECT == 0) {
                          ARM_WLAN_WD(CONNECT_TO_ADDRESS_MAX);
                      }
			wlan_start(0);

			SPARK_WLAN_STARTED = 1;
		}
	}

	if(WLAN_SMART_CONFIG_START)
	{
		/* Start CC3000 Smart Config Process */
		Start_Smart_Config();
	}
	else if (WLAN_MANUAL_CONNECT > 0 && !WLAN_DHCP)
	{
	    CLR_WLAN_WD();
	    wlan_ioctl_set_connection_policy(DISABLE, DISABLE, DISABLE);
	    /* Edit the below line before use*/
	    wlan_connect(WLAN_SEC_WPA2, _ssid, strlen(_ssid), NULL, (unsigned char*)_password, strlen(_password));
	    WLAN_MANUAL_CONNECT = -1;
	}

	// Complete Smart Config Process:
	// 1. if smart config is done
	// 2. CC3000 established AP connection
	// 3. DHCP IP is configured
	// then send mDNS packet to stop external SmartConfig application
	if ((WLAN_SMART_CONFIG_STOP == 1) && (WLAN_DHCP == 1) && (WLAN_CONNECTED == 1))
	{
		unsigned char loop_index = 0;

		while (loop_index < 3)
		{
			mdnsAdvertiser(1,device_name,strlen(device_name));
			loop_index++;
		}

		WLAN_SMART_CONFIG_STOP = 0;
	}

	if(SPARK_SOCKET_HANDSHAKE == 0)
	{
		if(SPARK_SOCKET_CONNECTED || SPARK_HANDSHAKE_COMPLETED)
		{
			Spark_Disconnect();

			SPARK_FLASH_UPDATE = 0;
			SPARK_LED_FADE = 0;
			SPARK_HANDSHAKE_COMPLETED = 0;
			SPARK_SOCKET_CONNECTED = 0;

			LED_SetRGBColor(RGB_COLOR_GREEN);
			LED_On(LED_RGB);
		}

		return;
	}

	if(WLAN_DHCP && !SPARK_WLAN_SLEEP && !SPARK_SOCKET_CONNECTED)
	{
		Delay(100);

		netapp_ipconfig(&ip_config);

		if(Spark_Error_Count)
		{
			LED_SetRGBColor(RGB_COLOR_RED);

			while(Spark_Error_Count != 0)
			{
				LED_On(LED_RGB);
				Delay(500);
				LED_Off(LED_RGB);
				Delay(500);
				Spark_Error_Count--;
			}

			//Send the Error Count to Cloud: NVMEM_Spark_File_Data[ERROR_COUNT_FILE_OFFSET]
			//To Do

			//Reset Error Count
			NVMEM_Spark_File_Data[ERROR_COUNT_FILE_OFFSET] = 0;
			nvmem_write(NVMEM_SPARK_FILE_ID, 1, ERROR_COUNT_FILE_OFFSET, &NVMEM_Spark_File_Data[ERROR_COUNT_FILE_OFFSET]);
		}

		LED_SetRGBColor(RGB_COLOR_CYAN);
		LED_On(LED_RGB);

		if(Spark_Connect() >= 0)
                {
                        cofd_count  = 0;
                        SPARK_SOCKET_CONNECTED = 1;
                }
                else
		{
			if(SPARK_WLAN_RESET)
				return;

                        if ((cofd_count += RESET_ON_CFOD) == MAX_FAILED_CONNECTS)
			{
			    SPARK_WLAN_RESET = RESET_ON_CFOD;
			    ERROR("Resetting CC3000 due to %d failed connect attempts", MAX_FAILED_CONNECTS);

			}

			if(Internet_Test() < 0)
			{
				//No Internet Connection
	                        if ((cofd_count += RESET_ON_CFOD) == MAX_FAILED_CONNECTS)
	                        {
	                            SPARK_WLAN_RESET = RESET_ON_CFOD;
	                            ERROR("Resetting CC3000 due to %d failed connect attempts", MAX_FAILED_CONNECTS);
	                        }
				Spark_Error_Count = 2;
			}
			else
			{
				//Cloud not Reachable
				Spark_Error_Count = 3;
			}

			NVMEM_Spark_File_Data[ERROR_COUNT_FILE_OFFSET] = Spark_Error_Count;
			nvmem_write(NVMEM_SPARK_FILE_ID, 1, ERROR_COUNT_FILE_OFFSET, &NVMEM_Spark_File_Data[ERROR_COUNT_FILE_OFFSET]);

			SPARK_SOCKET_CONNECTED = 0;
		}
	}

	if (SPARK_SOCKET_CONNECTED)
	{
		if (!SPARK_HANDSHAKE_COMPLETED)
		{
			int err = Spark_Handshake();

			if (err)
			{
				if (0 > err)
				{
					// Wrong key error, red
					LED_SetRGBColor(0xff0000);
				}
				else if (1 == err)
				{
					// RSA decryption error, orange
					LED_SetRGBColor(0xff6000);
				}
				else if (2 == err)
				{
					// RSA signature verification error, magenta
					LED_SetRGBColor(0xff00ff);
				}
				LED_On(LED_RGB);
			}
			else
			{
				SPARK_HANDSHAKE_COMPLETED = 1;
			}
		}

		if (!Spark_Communication_Loop())
		{
			if (LED_RGB_OVERRIDE)
			{
				LED_Signaling_Stop();
			}

			SPARK_FLASH_UPDATE = 0;
			SPARK_LED_FADE = 0;
			SPARK_HANDSHAKE_COMPLETED = 0;
			SPARK_SOCKET_CONNECTED = 0;
		}
	}
}
