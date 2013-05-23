/**
  ******************************************************************************
  * @file    main.c
  * @author  Spark Application Team
  * @version V1.0.0
  * @date    30-April-2013
  * @brief   main file
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usb_lib.h"
#include "usb_conf.h"
#include "usb_prop.h"
#include "usb_pwr.h"
#include "dfu_mal.h"

/* Private typedef -----------------------------------------------------------*/
typedef  void (*pFunction)(void);

/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static __IO uint32_t TimingDelay, TimingBUTTON, TimingLED;
uint8_t DFUDeviceMode = 0x00;

uint32_t Internal_Flash_Address = 0;
uint32_t External_Flash_Address = 0;
uint32_t Internal_Flash_Data = 0;
uint8_t External_Flash_Data[4];
uint32_t EraseCounter = 0;
uint32_t NbrOfPage = 0;
volatile FLASH_Status FLASHStatus = FLASH_COMPLETE;

uint8_t WLAN_MANUAL_CONNECT = 1;//For Manual connection, set this to 1
uint8_t WLAN_SMART_CONFIG_START;
uint8_t WLAN_SMART_CONFIG_DONE;
uint8_t WLAN_CONNECTED;
uint8_t WLAN_DHCP;
uint8_t WLAN_CAN_SHUTDOWN;

uint8_t DeviceState;
uint8_t DeviceStatus[6];
pFunction Jump_To_Application;
uint32_t JumpAddress;

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
    /*
        At this stage the microcontroller clock setting is already configured, 
        this is done through SystemInit() function which is called from startup
        file (startup_stm32f10x_md.s) before to branch to application main.
        To reconfigure the default setting of SystemInit() function, refer to
        system_stm32f10x.c file
    */

	Set_System();

	/* Check if Enter DFU Mode command is received from marvin's USB Serial Console
	 * This will be stored as a flag in the Backup register which needs to be checked
	 * If true, set DFUDeviceMode = 0x01
	 * TO DO...
	 */

     /* Check if BUTTON1 is pressed for 1 sec to enter DFU Mode */
     if (BUTTON_GetState(BUTTON1) == BUTTON1_PRESSED)
     {
         TimingDelay = 1000;
         while (BUTTON_GetState(BUTTON1) == BUTTON1_PRESSED)
         {
             if(TimingDelay == 0x00)
             {
                 DFUDeviceMode = 0x01;
                 TimingBUTTON = 5000;	//To prevent immediate exit from DFU
                 break;
             }
         }
     }

    if (DFUDeviceMode != 0x01)
    {
        /* Test if user code is programmed starting from APPLICATION_START_ADDRESS */
        if (((*(__IO uint32_t*)APPLICATION_START_ADDRESS) & 0x2FFE0000 ) == 0x20000000)
        {
            /* Jump to user application */    
            JumpAddress = *(__IO uint32_t*) (APPLICATION_START_ADDRESS + 4);
            Jump_To_Application = (pFunction) JumpAddress;
            /* Initialize user application's Stack Pointer */
            __set_MSP(*(__IO uint32_t*) APPLICATION_START_ADDRESS);
            Jump_To_Application();
        }
    } 
    /* Otherwise enters USB/WLAN DFU mode to allow user to program his application */

	/* Enable PWR and BKP clock */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);

	/* Enable write access to Backup domain */
	PWR_BackupAccessCmd(ENABLE);

    /* Reconfigure the Button using EXTI */
    BUTTON_Init(BUTTON1, BUTTON_MODE_EXTI);

    /* Unlock the internal flash */
    FLASH_Unlock();

    if(BKP_ReadBackupRegister(BKP_DR3) != 0xCCCC)
    {
		/* Enter USB DFU mode */

		DeviceState = STATE_dfuERROR;
		DeviceStatus[0] = STATUS_ERRFIRMWARE;
		DeviceStatus[4] = DeviceState;

		/* USB Disconnect configuration */
		USB_Disconnect_Config();

		/* Disable the USB connection till initialization phase end */
		USB_Cable_Config(DISABLE);

		/* Init the media interface */
		MAL_Init();

		/* Enable the USB connection */
		USB_Cable_Config(ENABLE);

		/* USB Clock configuration */
		Set_USBClock();

		/* USB System initialization */
		USB_Init();
    }
    else
    {
		/* Enter WLAN DFU mode */

		/* Initialize CC3000's CS, EN and INT pins to their default states */
		CC3000_WIFI_Init();

#ifdef SPARK_SFLASH_ENABLE
		/* Initialize SPI Flash */
		sFLASH_Init();

		/* Run SPI Flash Self Test (Uncomment for Debugging) */
		sFLASH_SelfTest();
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
    }

    /* Main loop */
    while (1)
    {
	    if(BKP_ReadBackupRegister(BKP_DR3) != 0xCCCC)
	    {
			if(TimingBUTTON == 0x00 && BUTTON_GetDebouncedState(BUTTON1) != 0x00)
			{
				if (DeviceState == STATE_dfuIDLE || DeviceState == STATE_dfuERROR)
				{
					Reset_Device();	//Reset Device to enter User Application
				}
			}

	        if (TimingLED == 0x00)
	        {
	            TimingLED = 250;
	            LED_Toggle(LED1);
	            LED_Toggle(LED2);
	        }
	    }
	    else
	    {
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
			    //wlan_connect(WLAN_SEC_WPA2, "Haxlr8r-upstairs", 16, NULL, "wittycheese551", 14);
			    wlan_connect(WLAN_SEC_WPA2, "VED", 3, NULL, "BD180408", 8);
			    WLAN_MANUAL_CONNECT = 0;
			}
	    }
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

    while (TimingDelay != 0x00);

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
    if (TimingDelay != 0x00)
    { 
        TimingDelay--;
    }

    if (TimingBUTTON != 0x00)
    {
    	TimingBUTTON--;
    }

    if (TimingLED != 0x00)
    {
        TimingLED--;
    }
}

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

void Backup_Application(void)
{
	/* Initialize SPI Flash */
	sFLASH_Init();

	/* Define the number of External Flash pages to be erased */
	NbrOfPage = (INTERNAL_FLASH_END_ADDRESS - APPLICATION_START_ADDRESS) / sFLASH_PAGESIZE;
	NbrOfPage += 1;	//Incase NbrOfPage is not sFLASH_PAGESIZE aligned

	/* Erase the SPI Flash pages */
	for(EraseCounter = 0; (EraseCounter < NbrOfPage); EraseCounter++)
	{
		sFLASH_EraseSector(EXTERNAL_FLASH_APP_ADDRESS + (sFLASH_PAGESIZE * EraseCounter));
	}

	/* Program External Flash */
	Internal_Flash_Address = APPLICATION_START_ADDRESS;
	External_Flash_Address = EXTERNAL_FLASH_APP_ADDRESS;

	while(Internal_Flash_Address < INTERNAL_FLASH_END_ADDRESS)
	{
	    /* Read data from Internal Flash memory */
		Internal_Flash_Data = (*(__IO uint32_t*) Internal_Flash_Address);
		Internal_Flash_Address += 4;

	    /* Program Word to SPI Flash memory */
		External_Flash_Data[0] = (uint8_t)(Internal_Flash_Data & 0xFF);
		External_Flash_Data[1] = (uint8_t)((Internal_Flash_Data & 0xFF00) >> 8);
		External_Flash_Data[2] = (uint8_t)((Internal_Flash_Data & 0xFF0000) >> 16);
		External_Flash_Data[3] = (uint8_t)((Internal_Flash_Data & 0xFF000000) >> 24);;
		sFLASH_WriteBuffer(External_Flash_Data, External_Flash_Address, 4);
		External_Flash_Address += 4;
	}
}

void Restore_Application(void)
{
	FLASHStatus = FLASH_COMPLETE;

	/* Unlock the Internal Flash Bank1 Program Erase controller */
	FLASH_UnlockBank1();

	/* Initialize SPI Flash */
	sFLASH_Init();

	/* Define the number of Internal Flash pages to be erased */
	NbrOfPage = (INTERNAL_FLASH_END_ADDRESS - APPLICATION_START_ADDRESS) / INTERNAL_FLASH_PAGE_SIZE;

	/* Clear All pending flags */
	FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);

	/* Erase the Internal Flash pages */
	for(EraseCounter = 0; (EraseCounter < NbrOfPage) && (FLASHStatus == FLASH_COMPLETE); EraseCounter++)
	{
		FLASHStatus = FLASH_ErasePage(APPLICATION_START_ADDRESS + (INTERNAL_FLASH_PAGE_SIZE * EraseCounter));
	}

	/* Program Internal Flash Bank1 */
	Internal_Flash_Address = APPLICATION_START_ADDRESS;
	External_Flash_Address = EXTERNAL_FLASH_APP_ADDRESS;

	while((Internal_Flash_Address < INTERNAL_FLASH_END_ADDRESS) && (FLASHStatus == FLASH_COMPLETE))
	{
	    /* Read data from SPI Flash memory */
	    sFLASH_ReadBuffer(External_Flash_Data, External_Flash_Address, 4);
	    External_Flash_Address += 4;

	    /* Program Word to Internal Flash memory */
	    Internal_Flash_Data = (uint32_t)(External_Flash_Data[0] | (External_Flash_Data[1] << 8) | (External_Flash_Data[2] << 16) | (External_Flash_Data[3] << 24));
		FLASHStatus = FLASH_ProgramWord(Internal_Flash_Address, Internal_Flash_Data);
		Internal_Flash_Address += 4;
	}

	FLASH_LockBank1();

	NVIC_SystemReset();
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
