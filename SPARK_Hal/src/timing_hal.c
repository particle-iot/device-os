/**
 ******************************************************************************
 * @file    timing_hal.c
 * @author  Satish Nair
 * @version V1.0.0
 * @date    12-Sept-2014
 * @brief
 ******************************************************************************
  Copyright (c) 2013-14 Spark Labs, Inc.  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "timing_hal.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
volatile uint32_t TimingFlashUpdateTimeout;

/* Extern variables ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

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

#if !defined (RGB_NOTIFICATIONS_ON)     && defined (RGB_NOTIFICATIONS_OFF)
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
                        TimingLED = 50;         //50ms
                else
                        TimingLED = 100;        //100ms
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
