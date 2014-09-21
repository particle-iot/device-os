/**
 ******************************************************************************
 * @file    main.cpp
 * @author  Satish Nair, Zachary Crockett, Zach Supalla and Mohit Bhoite
 * @version V1.0.0
 * @date    13-March-2013
 * 
 * Updated: 14-Feb-2014 David Sidrane <david_s5@usa.net>
 * 
 * @brief   Main program body.
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
#include "debug.h"
#include "spark_utilities.h"
extern "C" {
#include "sst25vf_spi.h"
}

using namespace spark;

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
volatile uint32_t TimingFlashUpdateTimeout;

/* Extern variables ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

/*******************************************************************************
 * Function Name  : SparkCoreConfig.
 * Description    : Called in startup routine, before calling C++ constructors.
 * Input          : None.
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
extern "C" void SparkCoreConfig(void)
{
        DECLARE_SYS_HEALTH(ENTERED_SparkCoreConfig);
#ifdef DFU_BUILD_ENABLE
	/* Set the Vector Table(VT) base location at 0x5000 */
	NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0x5000);

	USE_SYSTEM_FLAGS = 1;
#endif

#ifdef SWD_JTAG_DISABLE
	/* Disable the Serial Wire JTAG Debug Port SWJ-DP */
	GPIO_PinRemapConfig(GPIO_Remap_SWJ_Disable, ENABLE);
#endif

	Set_System();

	SysTick_Configuration();

	/* Enable CRC clock */
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_CRC, ENABLE);
#if !defined (RGB_NOTIFICATIONS_ON)	&& defined (RGB_NOTIFICATIONS_OFF)
	LED_RGB_OVERRIDE = 1;
#endif

#if defined (SPARK_RTC_ENABLE)
	RTC_Configuration();
#endif

        /* Execute Stop mode if STOP mode flag is set via Spark.sleep(pin, mode) */
        Enter_STOP_Mode();

        LED_SetRGBColor(RGB_COLOR_WHITE);
        LED_On(LED_RGB);
        SPARK_LED_FADE = 1;

#ifdef IWDG_RESET_ENABLE
	// ToDo this needs rework for new bootloader
	/* Check if the system has resumed from IWDG reset */
	if (RCC_GetFlagStatus(RCC_FLAG_IWDGRST) != RESET)
	{
		/* IWDGRST flag set */
		IWDG_SYSTEM_RESET = 1;

		/* Clear reset flags */
		RCC_ClearFlag();
	}

	/* We are duplicating the IWDG call here for compatibility with old bootloader */
	/* Set IWDG Timeout to 3 secs */
	IWDG_Reset_Enable(3 * TIMING_IWDG_RELOAD);
#endif

#ifdef DFU_BUILD_ENABLE
	Load_SystemFlags();
#endif

#ifdef SPARK_SFLASH_ENABLE
	sFLASH_Init();
#endif

#ifdef SPARK_WLAN_ENABLE
	/* Start Spark Wlan and connect to Wifi Router by default */
	SPARK_WLAN_SETUP = 1;

	/* Connect to Spark Cloud by default */
	SPARK_CLOUD_CONNECT = 1;
#endif
}

/*******************************************************************************
 * Function Name  : main.
 * Description    : main routine.
 * Input          : None.
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
int main(void)
{
  // We have running firmware, otherwise we wouldn't have gotten here
  DECLARE_SYS_HEALTH(ENTERED_Main);
  DEBUG("Hello from Spark!");

#ifdef SPARK_WLAN_ENABLE
  if (SPARK_WLAN_SETUP)
  {
    SPARK_WLAN_Setup(Multicast_Presence_Announcement);
  }
#endif

  /* Main loop */
  while (1)
  {
#ifdef SPARK_WLAN_ENABLE
    if(SPARK_WLAN_SETUP)
    {
      DECLARE_SYS_HEALTH(ENTERED_WLAN_Loop);
      SPARK_WLAN_Loop();
    }
#endif

#ifdef SPARK_WIRING_ENABLE
		static uint8_t SPARK_WIRING_APPLICATION = 0;
#ifdef SPARK_WLAN_ENABLE
		if(!SPARK_WLAN_SETUP || SPARK_WLAN_SLEEP || !SPARK_CLOUD_CONNECT || SPARK_CLOUD_CONNECTED || SPARK_WIRING_APPLICATION)
		{
			if(!SPARK_FLASH_UPDATE && !IWDG_SYSTEM_RESET)
			{
#endif
				if((SPARK_WIRING_APPLICATION != 1) && (NULL != setup))
				{
					//Execute user application setup only once
				        DECLARE_SYS_HEALTH(ENTERED_Setup);
					setup();
					SPARK_WIRING_APPLICATION = 1;
				}

				if(NULL != loop)
				{
					//Execute user application loop
			                DECLARE_SYS_HEALTH(ENTERED_Loop);
					loop();
                                        DECLARE_SYS_HEALTH(RAN_Loop);
				}
#ifdef SPARK_WLAN_ENABLE
			}
		}
#endif
#endif
	}
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

#if !defined (RGB_NOTIFICATIONS_ON)	&& defined (RGB_NOTIFICATIONS_OFF)
	//Just needed in case LED_RGB_OVERRIDE is set to 0 by accident
	if (LED_RGB_OVERRIDE == 0)
	{
		LED_RGB_OVERRIDE = 1;
		LED_Off(LED_RGB);
	}
#endif

	if (LED_RGB_OVERRIDE != 0)
	{
		if ((LED_Spark_Signal != 0) && (NULL != LED_Signaling_Override))
		{
			LED_Signaling_Override();
		}
	}
	else if (TimingLED != 0x00)
	{
		TimingLED--;
	}
	else if(WLAN_SMART_CONFIG_START || SPARK_FLASH_UPDATE || Spark_Error_Count)
	{
		//Do nothing
	}
	else if(SPARK_LED_FADE)
	{
		LED_Fade(LED_RGB);
		TimingLED = 20;//Breathing frequency kept constant
	}
	else if(SPARK_WLAN_SETUP && SPARK_CLOUD_CONNECTED)
	{
#if defined (RGB_NOTIFICATIONS_CONNECTING_ONLY)
		LED_Off(LED_RGB);
#else
		LED_SetRGBColor(RGB_COLOR_CYAN);
		LED_On(LED_RGB);
		SPARK_LED_FADE = 1;
#endif
	}
	else
	{
		LED_Toggle(LED_RGB);
		if(SPARK_CLOUD_SOCKETED)
			TimingLED = 50;		//50ms
		else
			TimingLED = 100;	//100ms
	}

#ifdef SPARK_WLAN_ENABLE
	if(!SPARK_WLAN_SETUP || SPARK_WLAN_SLEEP)
	{
		//Do nothing
	}
	else if(SPARK_FLASH_UPDATE)
	{
		if (TimingFlashUpdateTimeout >= TIMING_FLASH_UPDATE_TIMEOUT)
		{
			//Reset is the only way now to recover from stuck OTA update
			NVIC_SystemReset();
		}
		else
		{
			TimingFlashUpdateTimeout++;
		}
	}
	else if(!WLAN_SMART_CONFIG_START && BUTTON_GetDebouncedTime(BUTTON1) >= 3000)
	{
		BUTTON_ResetDebouncedState(BUTTON1);

		if(!SPARK_WLAN_SLEEP)
		{
			WiFi.listen();
		}
	}
	else if(BUTTON_GetDebouncedTime(BUTTON1) >= 7000)
	{
		BUTTON_ResetDebouncedState(BUTTON1);

		WLAN_DELETE_PROFILES = 1;
	}
#endif

#ifdef IWDG_RESET_ENABLE
	if (TimingIWDGReload >= TIMING_IWDG_RELOAD)
	{
		TimingIWDGReload = 0;

		/* Reload WDG counter */
		KICK_WDT();
		DECLARE_SYS_HEALTH(0xFFFF);
	}
	else
	{
		TimingIWDGReload++;
	}
#endif
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
