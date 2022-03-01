/**
  ******************************************************************************
  * @file    dfu_hal.h
  * @author  Satish Nair
  * @version V1.0.0
  * @date    20-Oct-2014
  * @brief   Header for dfu_hal module
  ******************************************************************************
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.

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
/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

uint8_t is_application_valid(uint32_t address);
void HAL_DFU_USB_Init(void);
void DFU_Check_Reset(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __DFU_HAL_H */
