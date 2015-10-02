/**
 ******************************************************************************
 * @file    can_hal.h
 * @author  Brian Spranger
 * @version V1.0.0
 * @date    30-Sep-2015
 * @brief
 ******************************************************************************
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.

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
#ifndef __CAN_HAL_H
#define __CAN_HAL_H

#include <stdbool.h>

/* Includes ------------------------------------------------------------------*/
#include "pinmap_hal.h"

/* Exported defines ----------------------------------------------------------*/
#if PLATFORM_ID == 6 // Photon
	#define TOTAL_CAN		1
#else
	#define TOTAL_CAN		0
#endif

#define CAN_BUFFER_SIZE      32

/* Exported types ------------------------------------------------------------*/
typedef struct CAN_Message_Struct
{
    bool Ext;
    bool rtr;
    unsigned char Len;
    unsigned char Data[8];
    unsigned long ID;
}CAN_Message_Struct;

typedef struct CAN_Ring_Buffer
{
  CAN_Message_Struct buffer[CAN_BUFFER_SIZE];
  volatile uint8_t head;
  volatile uint8_t tail;
} CAN_Ring_Buffer;

typedef enum HAL_CAN_Channel {
  HAL_CAN_Channel1 = 0    //maps to CAN2
} HAL_CAN_Channel;

/* Exported constants --------------------------------------------------------*/

/* Exported macros -----------------------------------------------------------*/

/* Exported functions --------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif

void HAL_CAN_Init(HAL_CAN_Channel channel, CAN_Ring_Buffer *rx_buffer, CAN_Ring_Buffer *tx_buffer);
void HAL_CAN_Begin(HAL_CAN_Channel channel, uint32_t baud);
void HAL_CAN_End(HAL_CAN_Channel channel);
uint32_t HAL_CAN_Write_Data(HAL_CAN_Channel channel, CAN_Message_Struct *pmessage);
int32_t HAL_CAN_Available_Data(HAL_CAN_Channel channel);
int32_t HAL_CAN_Read_Data(HAL_CAN_Channel channel, CAN_Message_Struct *pmessage);
int32_t HAL_CAN_Peek_Data(HAL_CAN_Channel channel, CAN_Message_Struct *pmessage);
void HAL_CAN_Flush_Data(HAL_CAN_Channel channel);
bool HAL_CAN_Is_Enabled(HAL_CAN_Channel channel);


#ifdef __cplusplus
}
#endif

#endif  /* __CAN_HAL_H */
