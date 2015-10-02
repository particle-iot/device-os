/**
 ******************************************************************************
 * @file    can_hal.c
 * @author  Brian Spranger
 * @version V1.0.0
 * @date    11-Oct-2015
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

/* Includes ------------------------------------------------------------------*/
#include "can_hal.h"

void HAL_CAN_Init(HAL_CAN_Serial channel, CAN_Ring_Buffer *rx_buffer, CAN_Ring_Buffer *tx_buffer)
{
}
void HAL_CAN_Begin(HAL_CAN_Serial channel, uint32_t baud)
{
}

void HAL_CAN_End(HAL_CAN_Serial channel)
{
}

uint32_t HAL_CAN_Write_Data(HAL_CAN_Serial channel, CAN_Message_Struct *pmessage)
{
  return 0;
}

int32_t HAL_CAN_Available_Data(HAL_CAN_Serial channel)
{
    return 0;
}

int32_t HAL_CAN_Read_Data(HAL_CAN_Serial channel, CAN_Message_Struct *pmessage)
{
    return 0;
}

int32_t HAL_CAN_Peek_Data(HAL_CAN_Serial channel, CAN_Message_Struct *pmessage)
{
    return 0;
}

void HAL_CAN_Flush_Data(HAL_CAN_Serial channel)
{
}

bool HAL_CAN_Is_Enabled(HAL_CAN_Serial channel)
{
    return false;
}
