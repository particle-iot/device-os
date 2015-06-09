/**
 ******************************************************************************
 * @file    rng_hal.c
 * @author  Satish Nair
 * @version V1.0.0
 * @date    13-Jan-2015
 * @brief
 ******************************************************************************
  Copyright (c) 2015 Particle Industries, Inc.  All rights reserved.

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
#include "rng_hal.h"
#include "stm32f2xx.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Extern variables ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/**
 * @brief  RNG configuration
 * @param  None
 * @retval None
 */
void HAL_RNG_Configuration(void)
{
    /* Enable RNG clock source */
    RCC_AHB2PeriphClockCmd(RCC_AHB2Periph_RNG, ENABLE);

    /* RNG Peripheral enable */
    RNG_Cmd(ENABLE);
}

/**
 * @brief  Get a 32bit random number
 * @param  None
 * @retval random number
 */
uint32_t HAL_RNG_GetRandomNumber(void)
{
    /* Wait until one RNG number is ready */
    while(RNG_GetFlagStatus(RNG_FLAG_DRDY) == RESET)
    {
    }

    /* Return a 32bit Random number */
    return RNG_GetRandomNumber();
}
