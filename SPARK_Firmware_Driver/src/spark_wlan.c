#include "spark_wlan.h"
#include "string.h"

__IO uint32_t TimingSparkProcessAPI;
__IO uint32_t TimingSparkAliveTimeout;
__IO uint32_t TimingSparkResetTimeout;
__IO uint32_t TimingSparkOTATimeout;

uint8_t WLAN_MANUAL_CONNECT = 0; //For Manual connection, set this to 1
uint8_t WLAN_SMART_CONFIG_START;
uint8_t WLAN_SMART_CONFIG_STOP;
uint8_t WLAN_SMART_CONFIG_FINISHED;
uint8_t WLAN_CONNECTED;
uint8_t WLAN_DHCP;
uint8_t WLAN_CAN_SHUTDOWN;

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
unsigned char _auth = WLAN_SEC_WPA2;

unsigned char NVMEM_Spark_File_Data[NVMEM_SPARK_FILE_SIZE];

__IO uint8_t SPARK_WLAN_SLEEP;
__IO uint8_t SPARK_WLAN_STARTED;
__IO uint8_t SPARK_SOCKET_CONNECTED;
__IO uint8_t SPARK_SOCKET_ALIVE;
__IO uint8_t SPARK_DEVICE_ACKED;
__IO uint8_t SPARK_FLASH_UPDATE;
__IO uint8_t SPARK_PROCESS_CHUNK;
__IO uint8_t SPARK_LED_TOGGLE;
__IO uint8_t SPARK_LED_FADE;

__IO uint8_t Spark_Connect_Count;
__IO uint8_t Spark_Error_Count;

void Set_NetApp_Timeout(void)
{
	unsigned long aucDHCP = 14400;
	unsigned long aucARP = 3600;
	unsigned long aucKeepalive = 10;
	unsigned long aucInactivity = 60;

	NetApp_Timeout_SysFlag = 0xFFFF;
	Save_SystemFlags();
	BKP_WriteBackupRegister(BKP_DR1, 0xFFFF);

	netapp_timeout_values(&aucDHCP, &aucARP, &aucKeepalive, &aucInactivity);

	NetApp_Timeout_SysFlag = 0xAAAA;
	Save_SystemFlags();
	BKP_WriteBackupRegister(BKP_DR1, 0xAAAA);
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
	WLAN_CONNECTED = 0;
	WLAN_DHCP = 0;
	WLAN_CAN_SHUTDOWN = 0;

	unsigned char wlan_profile_index;

	/* Reset all the previous configuration */
	wlan_ioctl_set_connection_policy(DISABLE, DISABLE, DISABLE);

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

#if defined (USE_SPARK_CORE_V02)
    LED_SetRGBColor(RGB_COLOR_BLUE);
	LED_On(LED_RGB);
	SPARK_LED_TOGGLE = 0;
#endif

	/* Wait for SmartConfig to finish */
	while (WLAN_SMART_CONFIG_FINISHED == 0)
	{
#if defined (USE_SPARK_CORE_V01)
		/* Toggle the LED2 every 100ms */
		LED_Toggle(LED2);
#elif defined (USE_SPARK_CORE_V02)
		LED_Toggle(LED_RGB);
#endif
		Delay(100);
	}

#if defined (USE_SPARK_CORE_V01)
	LED_Off(LED2);
#elif defined (USE_SPARK_CORE_V02)
	LED_On(LED_RGB);
#endif

	/* read count of wlan profiles stored */
	nvmem_read(NVMEM_SPARK_FILE_ID, 1, WLAN_PROFILE_FILE_OFFSET, &NVMEM_Spark_File_Data[WLAN_PROFILE_FILE_OFFSET]);

//	if(NVMEM_Spark_File_Data[WLAN_PROFILE_FILE_OFFSET] >= 7)
//	{
//		if(wlan_ioctl_del_profile(255) == 0)
//			NVMEM_Spark_File_Data[WLAN_PROFILE_FILE_OFFSET] = 0;
//	}

	/* Decrypt configuration information and add profile */
	wlan_profile_index = wlan_smart_config_process();
	if(wlan_profile_index != -1)
	{
		NVMEM_Spark_File_Data[WLAN_PROFILE_FILE_OFFSET] = wlan_profile_index + 1;
	}

	/* write count of wlan profiles stored */
	nvmem_write(NVMEM_SPARK_FILE_ID, 1, WLAN_PROFILE_FILE_OFFSET, &NVMEM_Spark_File_Data[WLAN_PROFILE_FILE_OFFSET]);

	/* Configure to connect automatically to the AP retrieved in the Smart config process */
	wlan_ioctl_set_connection_policy(DISABLE, DISABLE, ENABLE);

	if(USE_SYSTEM_FLAGS && (Smart_Config_SysFlag != 0xBBBB))
	{
		Smart_Config_SysFlag = 0xBBBB;
		Save_SystemFlags();
	}
	else if(BKP_ReadBackupRegister(BKP_DR2) != 0xBBBB)
	{
		BKP_WriteBackupRegister(BKP_DR2, 0xBBBB);
	}

	/* Reset the CC3000 */
	wlan_stop();

	WLAN_SMART_CONFIG_START = 0;

	Delay(100);

	wlan_start(0);

	SPARK_WLAN_STARTED = 1;

	/* Mask out all non-required events */
	wlan_set_event_mask(HCI_EVNT_WLAN_KEEPALIVE | HCI_EVNT_WLAN_UNSOL_INIT | HCI_EVNT_WLAN_ASYNC_PING_REPORT);

#if defined (USE_SPARK_CORE_V02)
    LED_SetRGBColor(RGB_COLOR_GREEN);
	LED_On(LED_RGB);
	SPARK_LED_TOGGLE = 1;
#endif

	TimingSparkProcessAPI = 0;
	TimingSparkAliveTimeout = 0;
	TimingSparkResetTimeout = 0;
	TimingSparkOTATimeout = 0;
}

/* WLAN Application related callbacks passed to wlan_init */
void WLAN_Async_Callback(long lEventType, char *data, unsigned char length)
{
	switch (lEventType)
	{
		case HCI_EVNT_WLAN_ASYNC_SIMPLE_CONFIG_DONE:
			WLAN_SMART_CONFIG_FINISHED = 1;
			WLAN_SMART_CONFIG_STOP = 1;
			WLAN_MANUAL_CONNECT = 0;
			break;

		case HCI_EVNT_WLAN_UNSOL_CONNECT:
			WLAN_CONNECTED = 1;
			break;

		case HCI_EVNT_WLAN_UNSOL_DISCONNECT:
			if(WLAN_CONNECTED)
			{
#if defined (USE_SPARK_CORE_V01)
				LED_Off(LED2);
#elif defined (USE_SPARK_CORE_V02)
				LED_SetRGBColor(RGB_COLOR_GREEN);
				LED_On(LED_RGB);
#endif
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
			WLAN_CONNECTED = 0;
			WLAN_DHCP = 0;
			SPARK_SOCKET_CONNECTED = 0;
			SPARK_SOCKET_ALIVE = 0;
			SPARK_DEVICE_ACKED = 0;
			SPARK_FLASH_UPDATE = 0;
			SPARK_PROCESS_CHUNK = 0;
			SPARK_LED_TOGGLE = 1;
			SPARK_LED_FADE = 0;
			Spark_Connect_Count = 0;
			break;

		case HCI_EVNT_WLAN_UNSOL_DHCP:
			if (*(data + 20) == 0)
			{
				WLAN_DHCP = 1;
#if defined (USE_SPARK_CORE_V01)
				LED_On(LED2);
#elif defined (USE_SPARK_CORE_V02)
				LED_SetRGBColor(RGB_COLOR_GREEN);
				LED_On(LED_RGB);
#endif
			}
			else
			{
				WLAN_DHCP = 0;
			}
			break;

		case HCI_EVENT_CC3000_CAN_SHUT_DOWN:
			WLAN_CAN_SHUTDOWN = 1;
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

void SPARK_WLAN_Setup(void)
{
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

	SPARK_WLAN_STARTED = 1;

	/* Mask out all non-required events from CC3000 */
	wlan_set_event_mask(HCI_EVNT_WLAN_KEEPALIVE | HCI_EVNT_WLAN_UNSOL_INIT | HCI_EVNT_WLAN_ASYNC_PING_REPORT);

	if(nvmem_read(NVMEM_SPARK_FILE_ID, NVMEM_SPARK_FILE_SIZE, 0, NVMEM_Spark_File_Data) != 0)
	{
		/* Delete all previously stored wlan profiles */
		wlan_ioctl_del_profile(255);

		/* Create new entry for Spark File in CC3000 EEPROM */
		nvmem_create_entry(NVMEM_SPARK_FILE_ID, NVMEM_SPARK_FILE_SIZE);

		int i = 0;
		for(i = 0; i < NVMEM_SPARK_FILE_SIZE; i++)
			NVMEM_Spark_File_Data[i] = 0;

		nvmem_write(NVMEM_SPARK_FILE_ID, NVMEM_SPARK_FILE_SIZE, 0, NVMEM_Spark_File_Data);
	}

	Spark_Error_Count = NVMEM_Spark_File_Data[ERROR_COUNT_FILE_OFFSET];

#if defined (USE_SPARK_CORE_V02)
	if(Spark_Error_Count)
	{
		LED_SetRGBColor(RGB_COLOR_RED);
		LED_On(LED_RGB);

		while(Spark_Error_Count != 0)
		{
			LED_Toggle(LED_RGB);
			Spark_Error_Count--;
			Delay(250);
		}

		NVMEM_Spark_File_Data[ERROR_COUNT_FILE_OFFSET] = 0;
		nvmem_write(NVMEM_SPARK_FILE_ID, 1, ERROR_COUNT_FILE_OFFSET, &NVMEM_Spark_File_Data[ERROR_COUNT_FILE_OFFSET]);
	}
#endif

	if(USE_SYSTEM_FLAGS && (NetApp_Timeout_SysFlag != 0xAAAA))
    	Set_NetApp_Timeout();
	else if(BKP_ReadBackupRegister(BKP_DR1) != 0xAAAA)
		Set_NetApp_Timeout();

	if(!WLAN_MANUAL_CONNECT)
	{
		if(USE_SYSTEM_FLAGS && (Smart_Config_SysFlag != 0xBBBB || NVMEM_Spark_File_Data[WLAN_PROFILE_FILE_OFFSET] == 0))
			WLAN_SMART_CONFIG_START = 1;
		else if(BKP_ReadBackupRegister(BKP_DR2) != 0xBBBB || NVMEM_Spark_File_Data[WLAN_PROFILE_FILE_OFFSET] == 0)
			WLAN_SMART_CONFIG_START = 1;
	}

	nvmem_read_sp_version(patchVer);
	if (patchVer[1] == 19)
	{
		/* patchVer = "\001\023" */
		/* Latest Patch Available after flashing "cc3000-patch-programmer.bin" */
	}

#if defined (USE_SPARK_CORE_V02)
	if(WLAN_MANUAL_CONNECT || !WLAN_SMART_CONFIG_START)
		LED_SetRGBColor(RGB_COLOR_GREEN);
	else
		LED_SetRGBColor(RGB_COLOR_BLUE);

	LED_On(LED_RGB);
#endif

	SPARK_LED_TOGGLE = 1;

	// Clear out the DHCP settings
	unsigned long pucSubnetMask = 0;
	unsigned long pucIP_Addr = 0;
	unsigned long pucIP_DefaultGWAddr = 0;
	unsigned long pucDNS = 0;

	netapp_dhcp(&pucIP_Addr, &pucSubnetMask, &pucIP_DefaultGWAddr, &pucDNS);
}

void SPARK_WLAN_Loop(void)
{
	if(SPARK_WLAN_SLEEP)
	{
		if(SPARK_WLAN_STARTED)
		{
			//Spark_Disconnect();

			CC3000_Write_Enable_Pin(WLAN_DISABLE);

			Delay(100);

			LED_SetRGBColor(RGB_COLOR_GREEN);
			LED_On(LED_RGB);

			SPARK_WLAN_STARTED = 0;
			WLAN_CONNECTED = 0;
			SPARK_SOCKET_CONNECTED = 0;
			SPARK_LED_TOGGLE = 1;
			SPARK_LED_FADE = 0;
			Spark_Connect_Count = 0;
		}
	}
	else
	{
		if(!SPARK_WLAN_STARTED)
		{
			wlan_start(0);

			Delay(100);

			SPARK_WLAN_STARTED = 1;
			SPARK_DEVICE_ACKED = 0;
			SPARK_FLASH_UPDATE = 0;
			SPARK_PROCESS_CHUNK = 0;

			TimingSparkProcessAPI = 0;
			TimingSparkAliveTimeout = 0;
			TimingSparkResetTimeout = 0;
			TimingSparkOTATimeout = 0;
		}
	}

	if(WLAN_SMART_CONFIG_START)
	{
		/* Start CC3000 Smart Config Process */
		Start_Smart_Config();
	}
	else if (WLAN_MANUAL_CONNECT && !WLAN_DHCP)
	{
	    wlan_ioctl_set_connection_policy(DISABLE, DISABLE, DISABLE);
	    /* Edit the below line before use*/
	    wlan_connect(WLAN_SEC_WPA2, _ssid, strlen(_ssid), NULL, _password, strlen(_password));
	    WLAN_MANUAL_CONNECT = 0;
	}

#if defined (USE_SPARK_CORE_V02)
	if(Spark_Error_Count)
	{
		SPARK_LED_TOGGLE = 0;
		LED_SetRGBColor(RGB_COLOR_RED);
		LED_On(LED_RGB);

		while(Spark_Error_Count != 0)
		{
			LED_Toggle(LED_RGB);
			Spark_Error_Count--;
			Delay(250);
		}

		LED_SetRGBColor(RGB_COLOR_GREEN);
		LED_On(LED_RGB);
		SPARK_LED_TOGGLE = 1;
	}
#endif

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

	if(WLAN_DHCP && !SPARK_WLAN_SLEEP && !SPARK_SOCKET_CONNECTED)
	{
#if defined (USE_SPARK_CORE_V02)
		LED_SetRGBColor(RGB_COLOR_CYAN);
		LED_On(LED_RGB);
		SPARK_LED_TOGGLE = 1;
#endif

		Spark_Connect_Count++;

		if(Spark_Connect() < 0)
			SPARK_SOCKET_CONNECTED = 0;
		else
			SPARK_SOCKET_CONNECTED = 1;
	}
}

void SPARK_WLAN_Timing(void)
{
    if (WLAN_CONNECTED && !SPARK_WLAN_SLEEP && !SPARK_SOCKET_CONNECTED)
    {
		if (!Spark_Connect_Count)
		{
			if (TimingSparkResetTimeout >= TIMING_SPARK_RESET_TIMEOUT)
			{
				Spark_Error_Count = 2;
				NVMEM_Spark_File_Data[ERROR_COUNT_FILE_OFFSET] = Spark_Error_Count;
				nvmem_write(NVMEM_SPARK_FILE_ID, 1, ERROR_COUNT_FILE_OFFSET, &NVMEM_Spark_File_Data[ERROR_COUNT_FILE_OFFSET]);

				NVIC_SystemReset();
			}
			else
			{
				TimingSparkResetTimeout++;
			}
		}
		else if (Spark_Connect_Count >= SPARK_CONNECT_MAX_ATTEMPT)
		{
			Spark_Error_Count = 3;
			NVMEM_Spark_File_Data[ERROR_COUNT_FILE_OFFSET] = Spark_Error_Count;
			nvmem_write(NVMEM_SPARK_FILE_ID, 1, ERROR_COUNT_FILE_OFFSET, &NVMEM_Spark_File_Data[ERROR_COUNT_FILE_OFFSET]);

			NVIC_SystemReset();
		}
    }

	if (SPARK_SOCKET_CONNECTED)
	{
		if(Spark_Connect_Count)
			Spark_Connect_Count = 0;
		else
			TimingSparkResetTimeout = 0;

		SPARK_SOCKET_ALIVE = 1;

		if (TimingSparkProcessAPI >= TIMING_SPARK_PROCESS_API)
		{
			TimingSparkProcessAPI = 0;

			if(Spark_Process_API_Response() < 0)
				SPARK_SOCKET_ALIVE = 0;
		}
		else
		{
			TimingSparkProcessAPI++;
		}

		if (SPARK_DEVICE_ACKED)
		{
			if (TimingSparkAliveTimeout >= TIMING_SPARK_ALIVE_TIMEOUT)
			{
				TimingSparkAliveTimeout = 0;

				SPARK_SOCKET_ALIVE = 0;
			}
			else
			{
				TimingSparkAliveTimeout++;
			}
		}

		if(SPARK_SOCKET_ALIVE != 1)
		{
			Spark_Error_Count = 4;
			NVMEM_Spark_File_Data[ERROR_COUNT_FILE_OFFSET] = Spark_Error_Count;
			nvmem_write(NVMEM_SPARK_FILE_ID, 1, ERROR_COUNT_FILE_OFFSET, &NVMEM_Spark_File_Data[ERROR_COUNT_FILE_OFFSET]);

			Spark_Disconnect();

			SPARK_SOCKET_CONNECTED = 0;
			SPARK_DEVICE_ACKED = 0;
			SPARK_FLASH_UPDATE = 0;
			SPARK_PROCESS_CHUNK = 0;
			Spark_Connect_Count = 0;
		}
	}
}
