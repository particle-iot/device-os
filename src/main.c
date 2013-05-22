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
#include "hw_config.h" 
#include "usb_lib.h"
#include "usb_conf.h"
#include "usb_prop.h"
#include "usb_pwr.h"
#include "dfu_mal.h"
#include "sst25vf_spi.h"

/* Private typedef -----------------------------------------------------------*/
typedef  void (*pFunction)(void);

/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static __IO uint32_t TimingDelay, TimingBUTTON, TimingLED;
uint8_t DFUDeviceMode = 0x00;

/* Extern variables ----------------------------------------------------------*/
uint8_t DeviceState;
uint8_t DeviceStatus[6];
pFunction Jump_To_Application;
uint32_t JumpAddress;

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

    /* Reconfigure the Button using EXTI */
    BUTTON_Init(BUTTON1, BUTTON_MODE_EXTI);

    /* Unlock the internal flash */
    FLASH_Unlock();

//#ifdef SPARK_SFLASH_ENABLE
//	/* Initialize SPI Flash */
//	sFLASH_Init();
//
//	/* Run SPI Flash Self Test (Uncomment for Debugging) */
//	sFLASH_SelfTest();
//#endif

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
