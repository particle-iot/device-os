/**
  ******************************************************************************
  * @file    dfu_hal.h
  * @author  Satish Nair
  * @version V1.0.0
  * @date    20-Oct-2014
  * @brief   Header for dfu_hal module
  ******************************************************************************
  Copyright (c) 2013-14 Spark Labs, Inc.  All rights reserved.

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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __DFU_HAL_H
#define __DFU_HAL_H

/* Includes ------------------------------------------------------------------*/
#include "hw_config.h"

/* Exported types ------------------------------------------------------------*/
typedef enum
{
    BKP_DR_01 = 0x01,
    BKP_DR_02 = 0x02,
    BKP_DR_03 = 0x03,
    BKP_DR_04 = 0x04,
    BKP_DR_05 = 0x05,
    BKP_DR_06 = 0x06,
    BKP_DR_07 = 0x07,
    BKP_DR_08 = 0x08,
    BKP_DR_09 = 0x09,
    BKP_DR_10 = 0x10
} BKP_DR_TypeDef;

/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */

void HAL_DFU_USB_Init(void);
int32_t HAL_Core_Backup_Register(uint32_t BKP_DR);
void HAL_Core_Write_Backup_Register(uint32_t BKP_DR, uint32_t Data);
uint32_t HAL_Core_Read_Backup_Register(uint32_t BKP_DR);

#endif /* __DFU_HAL_H */
