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
#include "debug.h"
#include "spark_utilities.h"
#include "spark_wlan.h"
#include "core_hal.h"
#include "syshealth_hal.h"
#include "watchdog_hal.h"
#include "rgbled.h"

using namespace spark;

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static volatile uint32_t TimingLED;
static volatile uint32_t TimingIWDGReload;

#ifdef MEASURE_LOOP_FREQUENCY
static volatile uint32_t loop_counter;
static volatile uint32_t loop_frequency;
#endif

/* Extern variables ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/
/*******************************************************************************
 * Function Name  : HAL_SysTick_Handler (Declared as weak in core_hal.h)
 * Description    : Decrements the various Timing variables related to SysTick.
 * Input          : None
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
extern "C" void HAL_SysTick_Handler(void)
{
    if (LED_RGB_IsOverRidden())
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
    else if(SPARK_CLOUD_CONNECTED)
    {
        LED_SetRGBColor(RGB_COLOR_CYAN);
        LED_On(LED_RGB);
        SPARK_LED_FADE = 1;
    }
    else
    {
        LED_Toggle(LED_RGB);
        if(SPARK_CLOUD_SOCKETED)
            TimingLED = 50;         //50ms
        else
            TimingLED = 100;        //100ms
    }

    if(SPARK_WLAN_SLEEP)
    {
        //Do nothing
    }
    else if(SPARK_FLASH_UPDATE)
    {
        if (TimingFlashUpdateTimeout >= TIMING_FLASH_UPDATE_TIMEOUT)
        {
            //Reset is the only way now to recover from stuck OTA update
            HAL_Core_System_Reset();
        }
        else
        {
            TimingFlashUpdateTimeout++;
        }
    }
    else if(!WLAN_SMART_CONFIG_START && HAL_Core_Mode_Button_Pressed(3000))
    {
        //reset button debounce state if mode button is pressed for 3 seconds
        HAL_Core_Mode_Button_Reset();

        if(!SPARK_WLAN_SLEEP)
        {
            WLAN_SMART_CONFIG_START = 1;
        }
    }
    else if(HAL_Core_Mode_Button_Pressed(7000))
    {
        //reset button debounce state if mode button is pressed for 3+7=10 seconds
        HAL_Core_Mode_Button_Reset();

        WLAN_DELETE_PROFILES = 1;
    }

#ifdef IWDG_RESET_ENABLE
    if (TimingIWDGReload >= TIMING_IWDG_RELOAD)
    {
        TimingIWDGReload = 0;

        /* Reload WDG counter */
        HAL_Notify_WDT();
        DECLARE_SYS_HEALTH(0xFFFF);
    }
    else
    {
        TimingIWDGReload++;
    }
#endif
}

/*******************************************************************************
 * Function Name  : HAL_RTC_Handler (Declared as weak in rtc_hal.h)
 * Description    : This function handles RTC global interrupt request.
 * Input          : None.
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
extern "C" void HAL_RTC_Handler(void)
{
#ifdef MEASURE_LOOP_FREQUENCY
  loop_frequency = loop_counter;
  loop_counter = 0;
#endif

  if(NULL != Time_Update_Handler)
  {
    Time_Update_Handler();
  }
}

/*******************************************************************************
 * Function Name  : HAL_RTCAlarm_Handler (Declared as weak in rtc_hal.h)
 * Description    : This function handles additional application requirements.
 * Input          : None.
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
extern "C" void HAL_RTCAlarm_Handler(void)
{
  /* Wake up from Spark.sleep mode(SLEEP_MODE_WLAN) */
  SPARK_WLAN_SLEEP = 0;
}

#ifdef MEASURE_LOOP_FREQUENCY
/* Utility call declared as weak - used to return loop() frequency measured in Hz */
uint32_t loop_frequency_hz()
{
  return loop_frequency;
}
#endif

/*******************************************************************************
 * Function Name  : main.
 * Description    : main routine.
 * Input          : None.
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
void app_setup_and_loop(void)
{
    HAL_Core_Init();
    // We have running firmware, otherwise we wouldn't have gotten here
    DECLARE_SYS_HEALTH(ENTERED_Main);
    DEBUG("Hello from Spark!");

    SPARK_WLAN_Setup(Multicast_Presence_Announcement);

    /* Main loop */
    while (1)
    {
        DECLARE_SYS_HEALTH(ENTERED_WLAN_Loop);
        Spark_Idle();

        static uint8_t SPARK_WIRING_APPLICATION = 0;
        if(SPARK_WLAN_SLEEP || !SPARK_CLOUD_CONNECT || SPARK_CLOUD_CONNECTED || SPARK_WIRING_APPLICATION)
        {
            if(!SPARK_FLASH_UPDATE && !HAL_watchdog_reset_flagged())
            {
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
#ifdef MEASURE_LOOP_FREQUENCY
                    loop_counter++;
#endif
                    loop();
                    DECLARE_SYS_HEALTH(RAN_Loop);
                }
            }
        }
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
