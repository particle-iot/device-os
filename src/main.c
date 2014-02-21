/**
  ******************************************************************************
  * @file    main.c
  * @authors  Satish Nair, Zachary Crockett and Mohit Bhoite
  * @version V1.0.0
  * @date    30-April-2013
  * @brief   main file
  ******************************************************************************
  Copyright (c) 2013 Spark Labs, Inc.  All rights reserved.

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this program; if not, see <http://www.gnu.org/licenses/>.
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

uint8_t REFLASH_FROM_BACKUP = 0;    //0, 1
uint8_t OTA_FLASH_AVAILABLE = 0;    //0, 1
uint8_t USB_DFU_MODE = 0;		//0, 1
uint8_t FACTORY_RESET_MODE = 0;	//0, 1

uint8_t DeviceState;
uint8_t DeviceStatus[6];
pFunction Jump_To_Application;
uint32_t JumpAddress;
uint32_t ApplicationAddress;

/* Extern variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
static void IntToUnicode (uint32_t value , uint8_t *pbuf , uint8_t len);

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

	//FLASH_WriteProtection_Enable(BOOTLOADER_FLASH_PAGES);

    //--------------------------------------------------------------------------
    //    Initialize the system
    //--------------------------------------------------------------------------
    //    System Clocks
    //    System Interrupts
    //    Configure the I/Os
    //    Configure the Timer
    //    Configure the LEDs
    //    Configure the MODE button 
    //--------------------------------------------------------------------------
	Set_System();

    //--------------------------------------------------------------------------

    /* Setup SysTick Timer for 1 msec interrupts to call Timing_Decrement() */
	SysTick_Configuration();

	USE_SYSTEM_FLAGS = 1;

    //--------------------------------------------------------------------------
    //  Load the system flags saved at SYSTEM_FLAGS_ADDRESS = 0x08004C00
    //  CORE_FW_Version_SysFlag
    //  NVMEM_SPARK_Reset_SysFlag
    //  FLASH_OTA_Update_SysFlag
    //--------------------------------------------------------------------------
	Load_SystemFlags();

    //--------------------------------------------------------------------------

    // 0x5000 is written to the backup register after transferring the FW from 
    // the external flash to the STM32's internal memory 
	if((BKP_ReadBackupRegister(BKP_DR10) == 0x5000) ||
       (FLASH_OTA_Update_SysFlag == 0x5000))
	{
		ApplicationAddress = CORE_FW_ADDRESS; //0x08005000
	}

    // 0x0005 is written to the backup register at the end of firmware update.
    // if the register reads 0x0005, it signifies that the firmware update 
    // was successful
	else if((BKP_ReadBackupRegister(BKP_DR10) == 0x0005) || 
            (FLASH_OTA_Update_SysFlag == 0x0005))
	{
		//OTA was complete and the firmware is now available to be transfered to 
        //the internal flash memory
        OTA_FLASH_AVAILABLE = 1;
	}

    // 0x5555 is written to the backup register at the beginning of firmware update
    // if the register still reads 0x5555, it signifies that the firmware update
    // was never completed => FAIL
	else if((BKP_ReadBackupRegister(BKP_DR10) == 0x5555) || 
            (FLASH_OTA_Update_SysFlag == 0x5555))
	{
        //OTA transfer failed, hence, load firmware from the backup address
        OTA_FLASH_AVAILABLE = 0;
        REFLASH_FROM_BACKUP = 1;
	}
    // worst case: fall back on the DFU MODE
	else
	{
		USB_DFU_MODE = 1;
	}

	__IO uint16_t BKP_DR1_Value = BKP_ReadBackupRegister(BKP_DR1);

	/* Check if the system has resumed from IWDG reset due to corrupt/invalid firmware*/
	if ((BKP_DR1_Value != 0xFFFF) && (RCC_GetFlagStatus(RCC_FLAG_IWDGRST) != RESET))
	{
		REFLASH_FROM_BACKUP = 0;
		OTA_FLASH_AVAILABLE = 0;
		USB_DFU_MODE = 0;
		FACTORY_RESET_MODE = 0;

		switch(BKP_DR1_Value)
		{
		case 1:	//On 1st recovery attempt, try to recover using sFlash - Backup Area
			REFLASH_FROM_BACKUP = 1;
			BKP_DR1_Value += 1;
			break;

		case 2:	//On 2nd recovery attempt, try to recover using sFlash - Factory Reset
			FACTORY_RESET_MODE = 1;
			BKP_DR1_Value += 1;
			break;

		case 3:	//On 3rd recovery attempt, try to recover using USB DFU Mode
			USB_DFU_MODE = 1;
			FLASH_Erase();
			break;
		}

		BKP_WriteBackupRegister(BKP_DR1, BKP_DR1_Value);
		BKP_WriteBackupRegister(BKP_DR10, 0x0000);
		FLASH_OTA_Update_SysFlag = 0x0000;
	    OTA_FLASHED_Status_SysFlag = 0x0000;
		Save_SystemFlags();

		/* Clear reset flags */
		RCC_ClearFlag();
	}
	else
	{
		BKP_DR1_Value = 1;
		BKP_WriteBackupRegister(BKP_DR1, BKP_DR1_Value);
	}

	/* Set IWDG Timeout to 3 secs */
	IWDG_Reset_Enable(3 * TIMING_IWDG_RELOAD);

	//--------------------------------------------------------------------------
    //    Check if BUTTON1 is pressed and determine the status
    //--------------------------------------------------------------------------
	if (BUTTON_GetState(BUTTON1) == BUTTON1_PRESSED)
	{
		TimingBUTTON = 10000;
		while (BUTTON_GetState(BUTTON1) == BUTTON1_PRESSED)
		{
			if(TimingBUTTON == 0x00)
			{
				//if pressed for 10 sec, enter Factory Reset Mode
                OTA_FLASH_AVAILABLE = 0;
                REFLASH_FROM_BACKUP = 0;
				USB_DFU_MODE = 0;
				LED_SetRGBColor(RGB_COLOR_WHITE);
				FACTORY_RESET_MODE = 1;
				break;
			}
			else if(!USB_DFU_MODE && TimingBUTTON <= 7000)
			{
				//if pressed for >= 3 sec, enter USB DFU Mode
                OTA_FLASH_AVAILABLE = 0;
                REFLASH_FROM_BACKUP = 0;
				FACTORY_RESET_MODE = 0;
				LED_SetRGBColor(RGB_COLOR_YELLOW);
				USB_DFU_MODE = 1;
			}
		}
	}
    //--------------------------------------------------------------------------

	if (OTA_FLASH_AVAILABLE == 1)
	{
		LED_SetRGBColor(RGB_COLOR_MAGENTA);
		OTA_Flash_Update();
	}
	else if (FACTORY_RESET_MODE == 1)
	{
        //This tells the WLAN setup to clear the WiFi user profiles on bootup
		NVMEM_SPARK_Reset_SysFlag = 0x0001;
		Save_SystemFlags();
        //This following function takes a backup of the current firmware
        //and then restores the factory reset firmware
		Factory_Flash_Reset();
	}
	else if (USB_DFU_MODE == 0)
	{
		if (REFLASH_FROM_BACKUP == 1)
	    {
			LED_SetRGBColor(RGB_COLOR_RED);
	    	//If the Factory Reset or OTA Update failed, restore the old working copy
	    	FLASH_Restore(EXTERNAL_FLASH_BKP_ADDRESS);

	    	Finish_Update();
	    }

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

    LED_SetRGBColor(RGB_COLOR_YELLOW);

    USB_DFU_MODE = 1;

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

    /* Main loop */
    while (1)
    {
        /*
    	if(BUTTON_GetDebouncedTime(BUTTON1) >= 1000)
    	{
            //clear the button debounced time
    		BUTTON_ResetDebouncedState(BUTTON1);
            //make sure that there is no fw download in progress
			if (DeviceState == STATE_dfuIDLE || DeviceState == STATE_dfuERROR)
			{
				Finish_Update();	//Reset Device to enter User Application
			}
    	}
        */
    }
}

/*******************************************************************************
* Function Name  : Timing_Decrement
* Description    : Decrements the various Timing variables related to SysTick.
                   This function is called every 1mS.
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
    else if(FACTORY_RESET_MODE || REFLASH_FROM_BACKUP || OTA_FLASH_AVAILABLE)
    {
        LED_Toggle(LED_RGB);
        TimingLED = 50;
    }
    else if(USB_DFU_MODE)
    {
        LED_Toggle(LED_RGB);
        TimingLED = 100;
    }

	if (TimingIWDGReload >= TIMING_IWDG_RELOAD)
	{
		TimingIWDGReload = 0;

		/* Reload IWDG counter */
		IWDG_ReloadCounter();
	}
	else
	{
		TimingIWDGReload++;
	}
}

/*******************************************************************************
* Function Name  : Get_SerialNum.
* Description    : Create the serial number string descriptor.
* Input          : None.
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
        IntToUnicode (Device_Serial0, &DFU_StringSerial[2] , 8);
        IntToUnicode (Device_Serial1, &DFU_StringSerial[18], 4);
    }
}

/*******************************************************************************
* Function Name  : HexToChar.
* Description    : Convert Hex 32Bits value into char.
* Input          : None.
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
