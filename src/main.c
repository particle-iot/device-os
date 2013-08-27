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
uint8_t DFUDeviceMode = 0;
uint8_t FactoryResetMode = 0;

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

	Set_System();

	Load_SystemFlags();

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

	/* Check if BUTTON1 is pressed */
	if (BUTTON_GetState(BUTTON1) == BUTTON1_PRESSED)
	{
		TimingBUTTON = 10000;
		while (BUTTON_GetState(BUTTON1) == BUTTON1_PRESSED)
		{
			if(TimingBUTTON == 0x00)
			{
				//if pressed for 10 sec, enter Factory Reset Mode
				FactoryResetMode = 1;
				break;
			}
			else if(TimingBUTTON <= 7000)
			{
				//if pressed for >= 3 sec, enter USB DFU Mode
				DFUDeviceMode = 1;
			}
		}
	}

	if (FactoryResetMode)
	{
		Factory_Reset();
	}
	else if (!DFUDeviceMode)
	{
	    if ((BKP_ReadBackupRegister(BKP_DR10) == 0xCCCC) || (Flash_Update_SysFlag == 0xCCCC))
	    {
	    	//If the Factory Reset failed, restore the old working copy
	    	FLASH_Restore(EXTERNAL_FLASH_BKP1_ADDRESS);
			/* The Device will reset at this point */
			//while (1);
	    }

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

#if defined (USE_SPARK_CORE_V02)
    /* Set DFU mode RGB Led Flashing color to Yellow */
    LED_SetRGBColor(RGB_COLOR_YELLOW);
    LED_On(LED_RGB);
#endif

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
    	if(BUTTON_GetDebouncedTime(BUTTON1) >= 1000)
    	{
    		BUTTON_ResetDebouncedState(BUTTON1);
			if (DeviceState == STATE_dfuIDLE || DeviceState == STATE_dfuERROR)
			{
				Reset_Device();	//Reset Device to enter User Application
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
    else if(FactoryResetMode || DFUDeviceMode)
    {
#if defined (USE_SPARK_CORE_V01)
        LED_Toggle(LED1);
        LED_Toggle(LED2);
#elif defined (USE_SPARK_CORE_V02)
        LED_Toggle(LED_RGB);
#endif
        TimingLED = 100;
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
