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
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
__IO uint8_t WLAN_SMART_CONFIG_DONE;
__IO uint8_t WLAN_CONNECTED;
__IO uint8_t WLAN_DHCP;
__IO uint8_t WLAN_CAN_SHUTDOWN;

/*
// WIP! Not tested
// Simple Config Prefix
const char aucCC3000_prefix[] = {'T', 'T', 'T'};
//AES key "smartconfigAES16"
const unsigned char smartconfigkey[] = {0x73,0x6d,0x61,0x72,0x74,0x63,0x6f,0x6e,0x66,0x69,0x67,0x41,0x45,0x53,0x31,0x36};
*/

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

	/******************* WLAN Test Code ********************/
	//
	//Initialize CC3000's CS, EN and INT pins to their default states
	//
	CC3000_WIFI_Init();

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

	//WLAN device initialization completed
	LED_On(LED2);

	//
	// Mask out all non-required events from CC3000
	//
	wlan_set_event_mask(HCI_EVNT_WLAN_KEEPALIVE | HCI_EVNT_WLAN_UNSOL_INIT | HCI_EVNT_WLAN_UNSOL_DHCP | HCI_EVNT_WLAN_ASYNC_PING_REPORT);

	//
	// Start the Smart Config process
	//
	//wlan_smart_config_start(1);

	// Turn on Auto Connect option
	// C3000 device tries to connect to any AP it detects during scanning
	wlan_ioctl_set_connection_policy(1, 0, 0);	//WIP! - This only worked the first time

	/* Main loop */
	while (1)
	{
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
// WIP! Not tested
/*
	WLAN_SMART_CONFIG_DONE = 0;
	WLAN_CONNECTED = 0;
	WLAN_DHCP = 0;
	WLAN_CAN_SHUTDOWN = 0;

	//
	// Reset all the previous configuration
	//
	wlan_ioctl_set_connection_policy(DISABLE, DISABLE, DISABLE);
	wlan_ioctl_del_profile(255);

	//Wait until CC3000 is disconnected
	while (WLAN_CONNECTED == 1)
	{
		SysCtlDelay(100);
		hci_unsolicited_event_handler();
	}

	//
	// Trigger the Smart Config process
	//
	// Start blinking LED during Smart Configuration process
	LED_On(LED2);

	wlan_smart_config_set_prefix((char*)aucCC3000_prefix);
	LED_Off(LED2);

	//
	// Start the SmartConfig start process
	//
	wlan_smart_config_start(1);
	LED_On(LED2);

	//
	// Wait for First Time config finished
	//
	while (WLAN_SMART_CONFIG_DONE == 0)
	{
		LED_Off(LED2);
		SysCtlDelay(16500000);
		LED_On(LED2);
		SysCtlDelay(16500000);
	}

	LED_On(LED2);

	// create new entry for AES encryption key
	nvmem_create_entry(NVMEM_AES128_KEY_FILEID,16);

	// write AES key to NVMEM
	aes_write_key((unsigned char *)(&smartconfigkey[0]));

	// Decrypt configuration information and add profile
	wlan_smart_config_process();

	//
	// Configure to connect automatically to the AP retrieved in the
	// First Time config process
	//
	wlan_ioctl_set_connection_policy(DISABLE, DISABLE, ENABLE);

	//
	// reset the CC3000
	//
	wlan_stop();

	SysCtlDelay(100000);

	wlan_start(0);

	//
	// Mask out all non-required events
	//
	wlan_set_event_mask(HCI_EVNT_WLAN_KEEPALIVE | HCI_EVNT_WLAN_UNSOL_INIT | HCI_EVNT_WLAN_UNSOL_DHCP | HCI_EVNT_WLAN_ASYNC_PING_REPORT);
*/
}

/* WLAN Application related callbacks passed to wlan_init */
void WLAN_Async_Callback(long lEventType, char *data, unsigned char length)
{
	switch (lEventType)
	{
		case HCI_EVNT_WLAN_ASYNC_SIMPLE_CONFIG_DONE:
			WLAN_SMART_CONFIG_DONE = 1;
			break;

		case HCI_EVNT_WLAN_UNSOL_CONNECT:
			WLAN_CONNECTED = 1;
			break;

		case HCI_EVNT_WLAN_UNSOL_DISCONNECT:
			WLAN_CONNECTED = 0;
			WLAN_DHCP = 0;
			break;

		case HCI_EVNT_WLAN_UNSOL_DHCP:
			WLAN_DHCP = 1;
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
