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

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

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
