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

uint8_t DeviceState;
uint8_t DeviceStatus[6];
pFunction Jump_To_Application;
uint32_t JumpAddress;
uint32_t ApplicationAddress;

uint16_t NetApp_Timeout_SysFlag = 0xFFFF;
uint16_t Smart_Config_SysFlag = 0xFFFF;
uint16_t Flash_Update_SysFlag = 0xFFFF;

uint32_t Internal_Flash_Address = 0;
uint32_t External_Flash_Address = 0;
uint32_t Internal_Flash_Data = 0;
uint8_t External_Flash_Data[4];
uint32_t EraseCounter = 0;
uint32_t NbrOfPage = 0;
volatile FLASH_Status FLASHStatus = FLASH_COMPLETE;

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

	/* Run SPI Flash Self Test (Uncomment for Debugging) */
	//sFLASH_SelfTest();

	switch(BKP_ReadBackupRegister(BKP_DR10))
	{
		case 0x5000:
			ApplicationAddress = OTA_DFU_ADDRESS;
			break;
		case 0xC000:
			ApplicationAddress = CORE_FW_ADDRESS;
			break;
		default:
			if ((*(__IO uint16_t*)0x08004C04) == 0xC000)
				ApplicationAddress = CORE_FW_ADDRESS;
			else
				ApplicationAddress = OTA_DFU_ADDRESS;
			break;
	}

	/*
	 *	Factory Reset Conditional Code will go here
	 */

	/* Check if BUTTON1 is pressed for 3 sec to enter DFU Mode */
	if (BUTTON_GetState(BUTTON1) == BUTTON1_PRESSED)
	{
		TimingBUTTON = 3000;
		while (BUTTON_GetState(BUTTON1) == BUTTON1_PRESSED)
		{
			if(TimingBUTTON == 0x00)
			{
				DFUDeviceMode = 0x01;
				break;
			}
		}
	}

	if (DFUDeviceMode != 0x01)
	{
	    if (((*(__IO uint32_t*)ApplicationAddress) & 0x2FFE0000 ) != 0x20000000)
	        ApplicationAddress = CORE_FW_ADDRESS;

		/* Test if user code is programmed starting from ApplicationAddress */
		if (((*(__IO uint32_t*)ApplicationAddress) & 0x2FFE0000 ) == 0x20000000)
		{
			/* Jump to user application */
			JumpAddress = *(__IO uint32_t*) (ApplicationAddress + 4);
			Jump_To_Application = (pFunction) JumpAddress;
			/* Initialize user application's Stack Pointer */
			__set_MSP(*(__IO uint32_t*) ApplicationAddress);
			Jump_To_Application();
		}
	}
    /* Otherwise enters DFU mode to allow user to program his application */

    /* Enter DFU mode */
    DeviceState = STATE_dfuERROR;
    DeviceStatus[0] = STATUS_ERRFIRMWARE;
    DeviceStatus[4] = DeviceState;

    /* Unlock the internal flash */
    FLASH_Unlock();

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

#if defined (USE_SPARK_CORE_V02)
    /* Set DFU mode RGB Led Flashing color to Yellow */
    LED_SetRGBColor(RGB_COLOR_YELLOW);
#endif

    /* Main loop */
    while (1)
    {
    	if(BUTTON_GetDebouncedTime(BUTTON1) >= 1000)
    	{
    		BUTTON_ResetDebouncedState(BUTTON1);
			if (DeviceState == STATE_dfuIDLE || DeviceState == STATE_dfuERROR)
			{
				Reset_Device();	//Reset Device to enter User Application
			}
    	}

        if (TimingLED == 0x00)
        {
            TimingLED = 250;
#if defined (USE_SPARK_CORE_V01)
            LED_Toggle(LED1);
            LED_Toggle(LED2);
#elif defined (USE_SPARK_CORE_V02)
            LED_Toggle(LED_RGB);
#endif
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

    while (TimingDelay != 0x00);
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

void Load_SystemFlags(void)
{
	uint32_t Address = SYSTEM_FLAGS_ADDRESS;

	NetApp_Timeout_SysFlag = (*(__IO uint16_t*) Address);
	Address += 2;

	Smart_Config_SysFlag = (*(__IO uint16_t*) Address);
	Address += 2;

	Flash_Update_SysFlag = (*(__IO uint16_t*) Address);
	Address += 2;
}

void Save_SystemFlags(void)
{
	uint32_t Address = SYSTEM_FLAGS_ADDRESS;
	FLASH_Status FLASHStatus = FLASH_COMPLETE;

	/* Unlock the Flash Program Erase Controller */
	FLASH_Unlock();

	/* Clear All pending flags */
	FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);

	/* Erase the Internal Flash pages */
	FLASHStatus = FLASH_ErasePage(SYSTEM_FLAGS_ADDRESS);
	while(FLASHStatus != FLASH_COMPLETE);

	/* Program NetApp_Timeout_SysFlag */
	FLASHStatus = FLASH_ProgramHalfWord(Address, NetApp_Timeout_SysFlag);
	while(FLASHStatus != FLASH_COMPLETE);
	Address += 2;

	/* Program Smart_Config_SysFlag */
	FLASHStatus = FLASH_ProgramHalfWord(Address, Smart_Config_SysFlag);
	while(FLASHStatus != FLASH_COMPLETE);
	Address += 2;

	/* Program Flash_Update_SysFlag */
	FLASHStatus = FLASH_ProgramHalfWord(Address, Flash_Update_SysFlag);
	while(FLASHStatus != FLASH_COMPLETE);
	Address += 2;

	/* Locks the FLASH Program Erase Controller */
	FLASH_Lock();
}

void FLASH_Begin(void)
{
	FLASHStatus = FLASH_COMPLETE;

	Flash_Update_SysFlag = 0xCCCC;
	Save_SystemFlags();
	//BKP_WriteBackupRegister(BKP_DR10, 0xCCCC);

	/* Unlock the Flash Program Erase Controller */
	FLASH_Unlock();

	/* Define the number of Internal Flash pages to be erased */
	NbrOfPage = (INTERNAL_FLASH_END_ADDRESS - CORE_FW_ADDRESS) / INTERNAL_FLASH_PAGE_SIZE;

	/* Clear All pending flags */
	FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);

	/* Erase the Internal Flash pages */
	for (EraseCounter = 0; (EraseCounter < NbrOfPage) && (FLASHStatus == FLASH_COMPLETE); EraseCounter++)
	{
		FLASHStatus = FLASH_ErasePage(CORE_FW_ADDRESS + (INTERNAL_FLASH_PAGE_SIZE * EraseCounter));
	}

	Internal_Flash_Address = CORE_FW_ADDRESS;
}

void FLASH_End(void)
{
	/* Locks the FLASH Program Erase Controller */
	FLASH_Lock();

	Flash_Update_SysFlag = 0xC000;
	Save_SystemFlags();
	//BKP_WriteBackupRegister(BKP_DR10, 0xC000);

	NVIC_SystemReset();
}

void FLASH_Backup(uint32_t sFLASH_Address)
{
	/* Initialize SPI Flash */
	sFLASH_Init();

	/* Define the number of External Flash pages to be erased */
	NbrOfPage = (INTERNAL_FLASH_END_ADDRESS - CORE_FW_ADDRESS) / sFLASH_PAGESIZE;
	NbrOfPage += 1;	//Incase NbrOfPage is not sFLASH_PAGESIZE aligned

	/* Erase the SPI Flash pages */
	for (EraseCounter = 0; (EraseCounter < NbrOfPage); EraseCounter++)
	{
		sFLASH_EraseSector(sFLASH_Address + (sFLASH_PAGESIZE * EraseCounter));
	}

	Internal_Flash_Address = CORE_FW_ADDRESS;
	External_Flash_Address = sFLASH_Address;

	/* Program External Flash */
	while (Internal_Flash_Address < INTERNAL_FLASH_END_ADDRESS)
	{
	    /* Read data from Internal Flash memory */
		Internal_Flash_Data = (*(__IO uint32_t*) Internal_Flash_Address);
		Internal_Flash_Address += 4;

	    /* Program Word to SPI Flash memory */
		External_Flash_Data[0] = (uint8_t)(Internal_Flash_Data & 0xFF);
		External_Flash_Data[1] = (uint8_t)((Internal_Flash_Data & 0xFF00) >> 8);
		External_Flash_Data[2] = (uint8_t)((Internal_Flash_Data & 0xFF0000) >> 16);
		External_Flash_Data[3] = (uint8_t)((Internal_Flash_Data & 0xFF000000) >> 24);
		//OR
		//*((uint32_t *)External_Flash_Data) = Internal_Flash_Data;
		sFLASH_WriteBuffer(External_Flash_Data, External_Flash_Address, 4);
		External_Flash_Address += 4;
	}
}

void FLASH_Restore(uint32_t sFLASH_Address)
{
	/* Initialize SPI Flash */
	sFLASH_Init();

	FLASH_Begin();

	Internal_Flash_Address = CORE_FW_ADDRESS;
	External_Flash_Address = sFLASH_Address;

	/* Program Internal Flash Bank1 */
	while ((Internal_Flash_Address < INTERNAL_FLASH_END_ADDRESS) && (FLASHStatus == FLASH_COMPLETE))
	{
	    /* Read data from SPI Flash memory */
	    sFLASH_ReadBuffer(External_Flash_Data, External_Flash_Address, 4);
	    External_Flash_Address += 4;

	    /* Program Word to Internal Flash memory */
	    Internal_Flash_Data = (uint32_t)(External_Flash_Data[0] | (External_Flash_Data[1] << 8) | (External_Flash_Data[2] << 16) | (External_Flash_Data[3] << 24));
	    //OR
	    //Internal_Flash_Data = *((uint32_t *)External_Flash_Data);
		FLASHStatus = FLASH_ProgramWord(Internal_Flash_Address, Internal_Flash_Data);
		Internal_Flash_Address += 4;
	}

	FLASH_End();
}

void Factory_Reset(void)
{
#if defined (USE_SPARK_CORE_V02)
    LED_SetRGBColor(RGB_COLOR_WHITE);
    LED_On(LED_RGB);
#endif

	FLASH_Restore(EXTERNAL_FLASH_FAC_ADDRESS);
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
