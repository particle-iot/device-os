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
#include "usb_lib.h"
#include "usb_desc.h"
#include "usb_pwr.h"
#include "sst25vf_spi.h"
#include "spark_utilities.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
__IO uint32_t TimingDelay;
__IO uint32_t TimingLED1, TimingLED2;
__IO uint32_t TimingBUTTON1;
__IO uint32_t TimingMillis;

__IO uint32_t TimingSparkProcessAPI;
__IO uint32_t TimingSparkAliveTimeout;

__IO uint8_t TIMING_BUTTON1_PRESSED;

uint8_t WLAN_MANUAL_CONNECT = 0;//For Manual connection, set this to 1
uint8_t WLAN_SMART_CONFIG_START;
uint8_t WLAN_SMART_CONFIG_DONE;
uint8_t WLAN_CONNECTED;
uint8_t WLAN_DHCP;
uint8_t WLAN_CAN_SHUTDOWN;

__IO uint8_t SPARK_SOCKET_CONNECTED;
__IO uint8_t SPARK_SOCKET_ALIVE;
__IO uint8_t SPARK_DEVICE_ACKED;

//tNetappIpconfigRetArgs ipconfig;

/* Extern variables ----------------------------------------------------------*/

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
	Set_System();

#ifdef SPARK_WIRING_ENABLE
	if(NULL != setup)
	{
		setup();
	}
#endif

#ifdef SPARK_WLAN_ENABLE
	//
	//Initialize CC3000's CS, EN and INT pins to their default states
	//
	CC3000_WIFI_Init();

#ifdef SPARK_SFLASH_ENABLE
	//
	//Initialize SPI Flash
	//
	sFLASH_Init();
#endif

	//
	// Configure & initialize CC3000 SPI_DMA Interface
	//
	CC3000_SPI_DMA_Init();

	//
	// WLAN On API Implementation
	//
	wlan_init(WLAN_Async_Callback, WLAN_Firmware_Patch, WLAN_Driver_Patch, WLAN_BootLoader_Patch,
				CC3000_Read_Interrupt_Pin, CC3000_Interrupt_Enable, CC3000_Interrupt_Disable, CC3000_Write_Enable_Pin);

	//
	// Trigger a WLAN device
	//
	wlan_start(0);

	//
	// Mask out all non-required events from CC3000
	//
	wlan_set_event_mask(HCI_EVNT_WLAN_KEEPALIVE | HCI_EVNT_WLAN_UNSOL_INIT | HCI_EVNT_WLAN_ASYNC_PING_REPORT);

	/* Enable PWR and BKP clock */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);

	/* Enable write access to Backup domain */
	PWR_BackupAccessCmd(ENABLE);

	// This will be replaced with SPI-Flash based backup
    if(BKP_ReadBackupRegister(BKP_DR1) != 0xAAAA)
    {
    	Set_NetApp_Timeout();
    }

	if(!WLAN_MANUAL_CONNECT)
	{
		// This will be replaced with SPI-Flash based backup
		if(BKP_ReadBackupRegister(BKP_DR2) != 0xBBBB)
		{
			WLAN_SMART_CONFIG_START = 1;
		}
	}
#endif

//	unsigned char patchVer[2];
//	nvmem_read_sp_version(patchVer);
//	if (patchVer[1] == 14)
//	{
//	/* Latest Patch Available */
//	}

	/* Main loop */
	while (1)
	{
#ifdef SPARK_WLAN_ENABLE
		if(WLAN_SMART_CONFIG_START)
		{
			//
			// Start CC3000 first time configuration
			//
			Start_Smart_Config();
		}
		else if (WLAN_MANUAL_CONNECT && !WLAN_DHCP)
		{
		    wlan_ioctl_set_connection_policy(DISABLE, DISABLE, DISABLE);
		    wlan_connect(WLAN_SEC_WPA2, "Haxlr8r-upstairs", 16, NULL, "wittycheese551", 14);
		    WLAN_MANUAL_CONNECT = 0;
		}

		if(WLAN_DHCP && !SPARK_SOCKET_CONNECTED)
		{
//			netapp_ipconfig(&ipconfig);
//
//			if(ipconfig.aucIP[0] == 0x00)
//				continue;

			if(Spark_Connect() < 0)
				SPARK_SOCKET_CONNECTED = 0;
			else
				SPARK_SOCKET_CONNECTED = 1;
		}
#endif

#ifdef SPARK_WIRING_ENABLE
#ifdef SPARK_WLAN_ENABLE
		if(SPARK_SOCKET_CONNECTED && SPARK_DEVICE_ACKED)
		{
#endif
			if(NULL != loop)
			{
				loop();
			}
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

    // /* Enable the SysTick Counter */
    // SysTick->CTRL |= SysTick_CTRL_ENABLE;

    while(TimingDelay != 0);

    // /* Disable the SysTick Counter */
    // SysTick->CTRL &= ~SysTick_CTRL_ENABLE;

    // /* Clear the SysTick Counter */
    // SysTick->VAL = (uint32_t)0x00;
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

    if (TimingLED1 != 0x00)
    {
        TimingLED1--;
    }
    else if(!SPARK_DEVICE_ACKED)
    {
    	LED_Toggle(LED1);
    	TimingLED1 = 100;	//100ms
    }
    else
    {
    	static __IO uint8_t SparkDeviceAckedLedOn = 0;
    	if(!SparkDeviceAckedLedOn)
    	{
    		LED_On(LED1);//SPARK_DEVICE_ACKED
    		SparkDeviceAckedLedOn = 1;
    	}
    }

    if (TimingLED2 != 0x00)
    {
        TimingLED2--;
    }

    if (TimingBUTTON1 != 0x00)
    {
    	TimingBUTTON1--;
    }

    if(BUTTON_GetDebouncedState(BUTTON1) != 0x00)
    {
    	//Enter First Time Config On Next System Reset
    	//Since socket connect() is currently blocking
    	BKP_WriteBackupRegister(BKP_DR2, 0xFFFF);
    	NVIC_SystemReset();
    }

#ifdef SPARK_WLAN_ENABLE
	if (SPARK_SOCKET_CONNECTED)
	{
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
			Spark_Disconnect();

			SPARK_SOCKET_CONNECTED = 0;
			SPARK_DEVICE_ACKED = 0;
		}
	}
#endif
}

/*
void BUTTON1_IntHandler(void)
{
	if (BUTTON_GetState(BUTTON1) == BUTTON1_PRESSED)
    {
        if(!TIMING_BUTTON1_PRESSED)
        {
        	TimingBUTTON1 = 2000; //2 sec
        }

        if(TIMING_BUTTON1_PRESSED && !TimingBUTTON1)
        {
        	TIMING_BUTTON1_PRESSED = 0x00;

        	//Enter First Time Config On Next System Reset
        	//Since socket connect() is currently blocking
        	BKP_WriteBackupRegister(BKP_DR2, 0xFFFF);
        	NVIC_SystemReset();
        }
        else
        {
        	TIMING_BUTTON1_PRESSED = 0x01;
        }
    }
    else
    {
    	TIMING_BUTTON1_PRESSED = 0x00;
    }
}
*/

void Set_NetApp_Timeout(void)
{
	unsigned long aucDHCP = 14400;
	unsigned long aucARP = 3600;
	unsigned long aucKeepalive = 10;
	unsigned long aucInactivity = 60;

	BKP_WriteBackupRegister(BKP_DR1, 0xFFFF);

	netapp_timeout_values(&aucDHCP, &aucARP, &aucKeepalive, &aucInactivity);

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
	WLAN_SMART_CONFIG_DONE = 0;
	WLAN_CONNECTED = 0;
	WLAN_DHCP = 0;
	WLAN_CAN_SHUTDOWN = 0;

	BKP_WriteBackupRegister(BKP_DR2, 0xFFFF);

	//
	// Reset all the previous configuration
	//
	wlan_ioctl_set_connection_policy(DISABLE, DISABLE, DISABLE);
	wlan_ioctl_del_profile(255);

	//Wait until CC3000 is disconnected
	while (WLAN_CONNECTED == 1)
	{
		//Delay 100ms
		Delay(100);
		hci_unsolicited_event_handler();
	}

	//
	// Start the SmartConfig start process
	//
	wlan_smart_config_start(1);

	//
	// Wait for First Time config finished
	//
	while (WLAN_SMART_CONFIG_DONE == 0)
	{
		/* Toggle the LED2 every 100ms */
		LED_Toggle(LED2);
		Delay(100);
	}

	LED_Off(LED2);

	//
	// Configure to connect automatically to the AP retrieved in the
	// First Time config process
	//
	wlan_ioctl_set_connection_policy(DISABLE, DISABLE, ENABLE);

	BKP_WriteBackupRegister(BKP_DR2, 0xBBBB);

	NVIC_SystemReset();

/*
	//
	// reset the CC3000
	//
	wlan_stop();

	// Delay 1 sec
	Delay(1000);

	wlan_start(0);

	//
	// Mask out all non-required events
	//
	wlan_set_event_mask(HCI_EVNT_WLAN_KEEPALIVE | HCI_EVNT_WLAN_UNSOL_INIT | HCI_EVNT_WLAN_ASYNC_PING_REPORT);
*/
}

/* WLAN Application related callbacks passed to wlan_init */
void WLAN_Async_Callback(long lEventType, char *data, unsigned char length)
{
	switch (lEventType)
	{
		case HCI_EVNT_WLAN_ASYNC_SIMPLE_CONFIG_DONE:
			WLAN_SMART_CONFIG_DONE = 1;
			WLAN_MANUAL_CONNECT = 0;
			break;

		case HCI_EVNT_WLAN_UNSOL_CONNECT:
			WLAN_CONNECTED = 1;
			break;

		case HCI_EVNT_WLAN_UNSOL_DISCONNECT:
			WLAN_CONNECTED = 0;
			WLAN_DHCP = 0;
			SPARK_SOCKET_CONNECTED = 0;
			SPARK_SOCKET_ALIVE = 0;
			SPARK_DEVICE_ACKED = 0;
			LED_Off(LED2);
			break;

		case HCI_EVNT_WLAN_UNSOL_DHCP:
			WLAN_DHCP = 1;
			LED_On(LED2);
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
