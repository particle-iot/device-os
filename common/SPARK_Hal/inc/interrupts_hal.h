/**
 ******************************************************************************
 * @file    interrupts_hal.h
 * @author  Satish Nair, Brett Walach
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __INTERRUPTS_HAL_H
#define __INTERRUPTS_HAL_H

/* Includes ------------------------------------------------------------------*/
#include "pinmap_hal.h"

/* Exported types ------------------------------------------------------------*/
typedef enum InterruptMode {
  CHANGE,
  RISING,
  FALLING
} InterruptMode;

typedef void (*voidFuncPtr)(void);

/* Exported constants --------------------------------------------------------*/

/* Exported macros -----------------------------------------------------------*/

/* Exported functions --------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif

void HAL_Interrupts_Attach(uint16_t pin, voidFuncPtr handler, InterruptMode mode);
void HAL_Interrupts_Detach(uint16_t pin);
void HAL_Interrupts_Enable_All(void);
void HAL_Interrupts_Disable_All(void);

int HAL_disable_irq();
void HAL_enable_irq(int mask);


#ifdef __cplusplus
}
#endif

#endif  /* __INTERRUPTS_HAL_H */
