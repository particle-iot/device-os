/**
 ******************************************************************************
 * @file    main.c
 * @author  Spark Application Team
 * @version V1.0.0
 * @date    13-March-2013
 * @brief   Main program body.
 ******************************************************************************
 */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usb_conf.h"
#include "usb_lib.h"
#include "usb_desc.h"
#include "usb_pwr.h"
#include "usb_prop.h"
#include "sst25vf_spi.h"
#include "spark_utilities.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
__IO uint32_t TimingMillis;
__IO uint32_t TimingDelay;
__IO uint32_t TimingLED;

__IO uint32_t TimingSparkIWDGReload;
__IO uint32_t TimingSparkProcessAPI;
__IO uint32_t TimingSparkAliveTimeout;
__IO uint32_t TimingSparkResetTimeout;

uint8_t WLAN_MANUAL_CONNECT = 0;//For Manual connection, set this to 1
uint8_t WLAN_SMART_CONFIG_START;
uint8_t WLAN_SMART_CONFIG_STOP;
uint8_t WLAN_SMART_CONFIG_FINISHED;
uint8_t WLAN_CONNECTED;
uint8_t WLAN_DHCP;
uint8_t WLAN_CAN_SHUTDOWN;

__IO uint8_t SPARK_SOCKET_CONNECTED;
__IO uint8_t SPARK_SOCKET_ALIVE;
__IO uint8_t SPARK_DEVICE_ACKED;
__IO uint8_t SPARK_DEVICE_IWDGRST;
__IO uint8_t SPARK_LED_TOGGLE;
__IO uint8_t SPARK_LED_FADE;

__IO uint8_t Socket_Connect_Count;
__IO uint8_t Spark_Error_Count;

unsigned char patchVer[2];

/* Smart Config Prefix */
char aucCC3000_prefix[] = {'T', 'T', 'T'};
/* AES key "sparkdevices2013" */
const unsigned char smartconfigkey[] = "sparkdevices2013";	//16 bytes
/* device name used by smart config response */
char device_name[] = "CC3000";

unsigned char NVMEM_Spark_File_Data[NVMEM_SPARK_FILE_SIZE];

uint8_t  USART_Rx_Buffer[USART_RX_DATA_SIZE];
uint32_t USART_Rx_ptr_in = 0;
uint32_t USART_Rx_ptr_out = 0;
uint32_t USART_Rx_length  = 0;

uint8_t USB_Rx_Buffer[VIRTUAL_COM_PORT_DATA_SIZE];
uint16_t USB_Rx_length = 0;
uint16_t USB_Rx_ptr = 0;

uint8_t  USB_Tx_State = 0;
uint8_t  USB_Rx_State = 0;

uint32_t USB_USART_BaudRate = 9600;

static void IntToUnicode (uint32_t value , uint8_t *pbuf , uint8_t len);

/* Extern variables ----------------------------------------------------------*/
extern LINE_CODING linecoding;

/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

/*******************************************************************************
 * Function Name  : main.
 * Description    : main routine.
 * Input          : None.
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
int main(void)
{
#ifdef DFU_BUILD_ENABLE
	/* Set the Vector Table(VT) base location at 0xC000 */
	NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0xC000);
#endif

#ifdef SWD_JTAG_DISABLE
    /* Disable the Serial Wire JTAG Debug Port SWJ-DP */
    GPIO_PinRemapConfig(GPIO_Remap_SWJ_Disable, ENABLE);
#endif

	Set_System();

#if defined (USE_SPARK_CORE_V02)
    LED_SetRGBColor(RGB_COLOR_WHITE);
    LED_On(LED_RGB);

#if defined (RTC_TEST_ENABLE)
    RTC_Configuration();
#endif
#endif

#ifdef IWDG_RESET_ENABLE
	/* Check if the system has resumed from IWDG reset */
	if (RCC_GetFlagStatus(RCC_FLAG_IWDGRST) != RESET)
	{
		/* IWDGRST flag set */
		SPARK_DEVICE_IWDGRST = 1;

		/* Clear reset flags */
		RCC_ClearFlag();
	}

	/* Set IWDG Timeout to 3 secs */
	IWDG_Reset_Enable(3 * TIMING_SPARK_IWDG_RELOAD);
#endif

#ifdef DFU_BUILD_ENABLE
	Load_SystemFlags();

	if(Flash_Update_SysFlag != 0xC000)
	{
		Flash_Update_SysFlag = 0xC000;
		Save_SystemFlags();
	}
#endif

#ifdef SPARK_WIRING_ENABLE
	if((SPARK_DEVICE_IWDGRST != 1) && (NULL != setup))
	{
		setup();
	}
#endif

#ifdef SPARK_WLAN_ENABLE

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

	/* Mask out all non-required events from CC3000 */
	wlan_set_event_mask(HCI_EVNT_WLAN_KEEPALIVE | HCI_EVNT_WLAN_UNSOL_INIT | HCI_EVNT_WLAN_ASYNC_PING_REPORT);

	if(nvmem_read(NVMEM_SPARK_FILE_ID, NVMEM_SPARK_FILE_SIZE, 0, NVMEM_Spark_File_Data) != 0)
	{
		/* Delete all previously stored wlan profiles */
		wlan_ioctl_del_profile(255);

		/* Create new entry for Spark File in CC3000 EEPROM */
		nvmem_create_entry(NVMEM_SPARK_FILE_ID, NVMEM_SPARK_FILE_SIZE);
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

#ifdef DFU_BUILD_ENABLE
    if(NetApp_Timeout_SysFlag != 0xAAAA)
#else
    if(BKP_ReadBackupRegister(BKP_DR1) != 0xAAAA)
#endif
    {
    	Set_NetApp_Timeout();
    }

	if(!WLAN_MANUAL_CONNECT)
	{
#ifdef DFU_BUILD_ENABLE
		if(Smart_Config_SysFlag != 0xBBBB || NVMEM_Spark_File_Data[WLAN_PROFILE_FILE_OFFSET] == 0)
#else
		if(BKP_ReadBackupRegister(BKP_DR2) != 0xBBBB || NVMEM_Spark_File_Data[WLAN_PROFILE_FILE_OFFSET] == 0)
#endif
		{
			WLAN_SMART_CONFIG_START = 1;
		}
	}

	nvmem_read_sp_version(patchVer);
	if (patchVer[1] == 19)
	{
		/* patchVer = "\001\023" */
		/* Latest Patch Available after flashing "cc3000-patch-programmer.bin" */
	}
#endif

#if defined (USE_SPARK_CORE_V02)
	if(WLAN_MANUAL_CONNECT || !WLAN_SMART_CONFIG_START)
		LED_SetRGBColor(RGB_COLOR_GREEN);
	else
		LED_SetRGBColor(RGB_COLOR_BLUE);

	LED_On(LED_RGB);
#endif

	SPARK_LED_TOGGLE = 1;

	/* Main loop */
	while (1)
	{
#ifdef SPARK_WLAN_ENABLE
		if(WLAN_SMART_CONFIG_START)
		{
			/* Start CC3000 Smart Config Process */
			Start_Smart_Config();
		}
		else if (WLAN_MANUAL_CONNECT && !WLAN_DHCP)
		{
		    wlan_ioctl_set_connection_policy(DISABLE, DISABLE, DISABLE);
		    /* Edit the below line before use*/
		    wlan_connect(WLAN_SEC_WPA2, "ssid", 4, NULL, "password", 8);
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

		if(WLAN_DHCP && !SPARK_SOCKET_CONNECTED)
		{
#if defined (USE_SPARK_CORE_V02)
			LED_SetRGBColor(RGB_COLOR_CYAN);
			LED_On(LED_RGB);
			SPARK_LED_TOGGLE = 1;
#endif

			Socket_Connect_Count++;

			if(Spark_Connect() < 0)
				SPARK_SOCKET_CONNECTED = 0;
			else
				SPARK_SOCKET_CONNECTED = 1;
		}
#endif

#ifdef SPARK_WIRING_ENABLE
#ifdef SPARK_WLAN_ENABLE
		if(SPARK_SOCKET_CONNECTED && SPARK_DEVICE_ACKED && !SPARK_DEVICE_IWDGRST)
		{
#endif
			if(NULL != loop)
			{
				loop();
			}

			if(NULL != pHandleMessage)
			{
				pHandleMessage();
			}

			Spark_User_Func_Execute();
#ifdef SPARK_WLAN_ENABLE
		}
#endif
#endif
	}
}

/*******************************************************************************
* Function Name  : Delay
* Description    : Inserts a delay time.
* Input          : nTime: specifies the delay time length, in milliseconds.
* Output         : None
* Return         : None
*******************************************************************************/
void Delay(uint32_t nTime)
{
    TimingDelay = nTime;

    while(TimingDelay != 0);
}

/*******************************************************************************
* Function Name  : Timing_Decrement
* Description    : Decrements the various Timing variables related to SysTick.
* Input          : None
* Output         : Timing
* Return         : None
*******************************************************************************/
void Timing_Decrement(void)
{
	TimingMillis++;

    if (TimingDelay != 0x00)
    {
        TimingDelay--;
    }

    if (TimingLED != 0x00)
    {
        TimingLED--;
    }
    else if(SPARK_LED_FADE)
    {
#if defined (USE_SPARK_CORE_V02)
    	LED_Fade(LED_RGB);
    	TimingLED = 25;	//25ms
#endif
    }
    else if(!SPARK_DEVICE_ACKED)
    {
#if defined (USE_SPARK_CORE_V01)
    	LED_Toggle(LED1);
#elif defined (USE_SPARK_CORE_V02)
    	if(SPARK_LED_TOGGLE)
    		LED_Toggle(LED_RGB);
#endif
    	TimingLED = 100;	//100ms
    }
    else
    {
    	static __IO uint8_t SparkDeviceAckedLedOn = 0;
    	if(!SparkDeviceAckedLedOn)
    	{
#if defined (USE_SPARK_CORE_V01)
    		LED_On(LED1);//SPARK_DEVICE_ACKED
#elif defined (USE_SPARK_CORE_V02)
    		LED_SetRGBColor(RGB_COLOR_CYAN);
    		LED_On(LED_RGB);
    		SPARK_LED_FADE = 1;
#endif
    		SparkDeviceAckedLedOn = 1;
    	}
    }

	if(BUTTON_GetDebouncedTime(BUTTON1) >= 3000)
	{
		BUTTON_ResetDebouncedState(BUTTON1);
    	//Enter Smart Config Process On Next System Reset
    	//Since socket connect() is currently blocking
#ifdef DFU_BUILD_ENABLE
		Smart_Config_SysFlag = 0xFFFF;
		Save_SystemFlags();
#else
		BKP_WriteBackupRegister(BKP_DR2, 0xFFFF);
#endif

    	NVIC_SystemReset();
    }

#ifdef IWDG_RESET_ENABLE
	if (TimingSparkIWDGReload >= TIMING_SPARK_IWDG_RELOAD)
	{
		TimingSparkIWDGReload = 0;

	    /* Reload IWDG counter */
	    IWDG_ReloadCounter();
	}
	else
	{
		TimingSparkIWDGReload++;
	}
#endif

#ifdef SPARK_WLAN_ENABLE
    if (WLAN_CONNECTED && !SPARK_SOCKET_CONNECTED)
    {
		if (!Socket_Connect_Count)
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
		else if (Socket_Connect_Count >= SOCKET_CONNECT_MAX_ATTEMPT)
		{
			Spark_Error_Count = 3;
			NVMEM_Spark_File_Data[ERROR_COUNT_FILE_OFFSET] = Spark_Error_Count;
			nvmem_write(NVMEM_SPARK_FILE_ID, 1, ERROR_COUNT_FILE_OFFSET, &NVMEM_Spark_File_Data[ERROR_COUNT_FILE_OFFSET]);

			NVIC_SystemReset();
		}
    }

	if (SPARK_SOCKET_CONNECTED)
	{
		if(Socket_Connect_Count)
			Socket_Connect_Count = 0;
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
			Socket_Connect_Count = 0;
		}
	}
#endif
}

void Set_NetApp_Timeout(void)
{
	unsigned long aucDHCP = 14400;
	unsigned long aucARP = 3600;
	unsigned long aucKeepalive = 10;
	unsigned long aucInactivity = 60;

#ifdef DFU_BUILD_ENABLE
	NetApp_Timeout_SysFlag = 0xFFFF;
	Save_SystemFlags();
#else
	BKP_WriteBackupRegister(BKP_DR1, 0xFFFF);
#endif

	netapp_timeout_values(&aucDHCP, &aucARP, &aucKeepalive, &aucInactivity);

#ifdef DFU_BUILD_ENABLE
	NetApp_Timeout_SysFlag = 0xAAAA;
	Save_SystemFlags();
#else
	BKP_WriteBackupRegister(BKP_DR1, 0xAAAA);
#endif
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

	if(Smart_Config_SysFlag != 0xBBBB)
	{
#ifdef DFU_BUILD_ENABLE
		Smart_Config_SysFlag = 0xBBBB;
		Save_SystemFlags();
#else
		BKP_WriteBackupRegister(BKP_DR2, 0xBBBB);
#endif
	}

	/* Reset the CC3000 */
	wlan_stop();

	WLAN_SMART_CONFIG_START = 0;

	Delay(100);

	wlan_start(0);

	/* Mask out all non-required events */
	wlan_set_event_mask(HCI_EVNT_WLAN_KEEPALIVE | HCI_EVNT_WLAN_UNSOL_INIT | HCI_EVNT_WLAN_ASYNC_PING_REPORT);

#if defined (USE_SPARK_CORE_V02)
    LED_SetRGBColor(RGB_COLOR_GREEN);
	LED_On(LED_RGB);
	SPARK_LED_TOGGLE = 1;
#endif
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
			SPARK_LED_TOGGLE = 1;
			SPARK_LED_FADE = 0;
			Socket_Connect_Count = 0;
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

void Start_OTA_Update(void)
{
#ifdef DFU_BUILD_ENABLE
	Flash_Update_SysFlag = 0x5000;
	Save_SystemFlags();
#endif
	BKP_WriteBackupRegister(BKP_DR10, 0x5000);

	NVIC_SystemReset();
}

/*******************************************************************************
* Function Name  : USB_USART_Init
* Description    : Start USB-USART protocol.
* Input          : baudRate.
* Return         : None.
*******************************************************************************/
void USB_USART_Init(uint32_t baudRate)
{
	linecoding.bitrate = baudRate;
	USB_Disconnect_Config();
	Set_USBClock();
	USB_Interrupts_Config();
	USB_Init();
}

/*******************************************************************************
* Function Name  : USB_USART_Available_Data.
* Description    : Return the length of available data received from USB.
* Input          : None.
* Return         : Length.
*******************************************************************************/
uint8_t USB_USART_Available_Data(void)
{
	if(bDeviceState == CONFIGURED)
	{
		if(USB_Rx_State == 1)
		{
			return (USB_Rx_length - USB_Rx_ptr);
		}
	}

	return 0;
}

/*******************************************************************************
* Function Name  : USB_USART_Receive_Data.
* Description    : Return data sent by USB Host.
* Input          : None
* Return         : Data.
*******************************************************************************/
int32_t USB_USART_Receive_Data(void)
{
	if(bDeviceState == CONFIGURED)
	{
		if(USB_Rx_State == 1)
		{
			if((USB_Rx_length - USB_Rx_ptr) == 1)
			{
				USB_Rx_State = 0;

				/* Enable the receive of data on EP3 */
				SetEPRxValid(ENDP3);
			}

			return USB_Rx_Buffer[USB_Rx_ptr++];
		}
	}

	return -1;
}

/*******************************************************************************
* Function Name  : USB_USART_Send_Data.
* Description    : Send Data from USB_USART to USB Host.
* Input          : Data.
* Return         : None.
*******************************************************************************/
void USB_USART_Send_Data(uint8_t Data)
{
	if(bDeviceState == CONFIGURED)
	{
		USART_Rx_Buffer[USART_Rx_ptr_in] = Data;

		USART_Rx_ptr_in++;

		/* To avoid buffer overflow */
		if(USART_Rx_ptr_in == USART_RX_DATA_SIZE)
		{
			USART_Rx_ptr_in = 0;
		}
	}
}

/*******************************************************************************
* Function Name  : Handle_USBAsynchXfer.
* Description    : send data to USB.
* Input          : None.
* Return         : None.
*******************************************************************************/
void Handle_USBAsynchXfer (void)
{

  uint16_t USB_Tx_ptr;
  uint16_t USB_Tx_length;

  if(USB_Tx_State != 1)
  {
    if (USART_Rx_ptr_out == USART_RX_DATA_SIZE)
    {
      USART_Rx_ptr_out = 0;
    }

    if(USART_Rx_ptr_out == USART_Rx_ptr_in)
    {
      USB_Tx_State = 0;
      return;
    }

    if(USART_Rx_ptr_out > USART_Rx_ptr_in) /* rollback */
    {
      USART_Rx_length = USART_RX_DATA_SIZE - USART_Rx_ptr_out;
    }
    else
    {
      USART_Rx_length = USART_Rx_ptr_in - USART_Rx_ptr_out;
    }

    if (USART_Rx_length > VIRTUAL_COM_PORT_DATA_SIZE)
    {
      USB_Tx_ptr = USART_Rx_ptr_out;
      USB_Tx_length = VIRTUAL_COM_PORT_DATA_SIZE;

      USART_Rx_ptr_out += VIRTUAL_COM_PORT_DATA_SIZE;
      USART_Rx_length -= VIRTUAL_COM_PORT_DATA_SIZE;
    }
    else
    {
      USB_Tx_ptr = USART_Rx_ptr_out;
      USB_Tx_length = USART_Rx_length;

      USART_Rx_ptr_out += USART_Rx_length;
      USART_Rx_length = 0;
    }
    USB_Tx_State = 1;
    UserToPMABufferCopy(&USART_Rx_Buffer[USB_Tx_ptr], ENDP1_TXADDR, USB_Tx_length);
    SetEPTxCount(ENDP1, USB_Tx_length);
    SetEPTxValid(ENDP1);
  }

}

/*******************************************************************************
* Function Name  : Get_SerialNum.
* Description    : Create the serial number string descriptor.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
void Get_SerialNum(void)
{
  uint32_t Device_Serial0, Device_Serial1, Device_Serial2;

  Device_Serial0 = *(uint32_t*)ID1;
  Device_Serial1 = *(uint32_t*)ID2;
  Device_Serial2 = *(uint32_t*)ID3;

  Device_Serial0 += Device_Serial2;

  if (Device_Serial0 != 0)
  {
    IntToUnicode (Device_Serial0, &Virtual_Com_Port_StringSerial[2] , 8);
    IntToUnicode (Device_Serial1, &Virtual_Com_Port_StringSerial[18], 4);
  }
}

/*******************************************************************************
* Function Name  : HexToChar.
* Description    : Convert Hex 32Bits value into char.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
static void IntToUnicode (uint32_t value , uint8_t *pbuf , uint8_t len)
{
  uint8_t idx = 0;

  for( idx = 0 ; idx < len ; idx ++)
  {
    if( ((value >> 28)) < 0xA )
    {
      pbuf[ 2* idx] = (value >> 28) + '0';
    }
    else
    {
      pbuf[2* idx] = (value >> 28) + 'A' - 10;
    }

    value = value << 4;

    pbuf[ 2* idx + 1] = 0;
  }
}

#ifdef USE_FULL_ASSERT
/*******************************************************************************
 * Function Name  : assert_failed
 * Description    : Reports the name of the source file and the source line number
 *                  where the assert_param error has occurred.
 * Input          : - file: pointer to the source file name
 *                  - line: assert_param error line source number
 * Output         : None
 * Return         : None
 *******************************************************************************/
void assert_failed(uint8_t* file, uint32_t line)
{
	/* User can add his own implementation to report the file name and line number,
	 ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

	/* Infinite loop */
	while (1)
	{
	}
}
#endif
