/**
 ******************************************************************************
 * @file    spark_wiring_can.cpp
 * @author  Brian Spranger
 * @version V1.0.0
 * @date    01 October 2015
 * @brief   Wrapper for wiring hardware can module
 ******************************************************************************
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.
  Copyright (c) 2006 Nicholas Zambetti.  All right reserved.

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

#include "spark_wiring_can.h"
#include "spark_wiring_constants.h"

// Constructors ////////////////////////////////////////////////////////////////

CANChannel::CANChannel(HAL_CAN_Channel channel, CAN_Ring_Buffer *rx_buffer, CAN_Ring_Buffer *tx_buffer)
{
  _channel = channel;
  HAL_CAN_Init(_channel, rx_buffer, tx_buffer);
}
// Public Methods //////////////////////////////////////////////////////////////

void CANChannel::begin(unsigned long baud)
{
  HAL_CAN_Begin(_channel, baud);
}

// TODO
void CANChannel::begin(unsigned long baud, byte config)
{

}

void CANChannel::end()
{
  HAL_CAN_End(_channel);
}

int CANChannel::available(void)
{
  return HAL_CAN_Available_Data(_channel);
}

int CANChannel::peek(CAN_Message_Struct *pmessage)
{
  return HAL_CAN_Peek_Data(_channel, pmessage);
}

int CANChannel::read(CAN_Message_Struct *pmessage)
{
  return HAL_CAN_Read_Data(_channel, pmessage);
}

void CANChannel::flush()
{
  HAL_CAN_Flush_Data(_channel);
}

size_t CANChannel::write(CAN_Message_Struct *pmessage)
{
  return HAL_CAN_Write_Data(_channel, pmessage);
}

CANChannel::operator bool() {
  return true;
}

bool CANChannel::isEnabled() {
  return HAL_CAN_Is_Enabled(_channel);
}

#ifndef SPARK_WIRING_NO_CAN
// Preinstantiate Objects //////////////////////////////////////////////////////
static CAN_Ring_Buffer can_rx_buffer;
static CAN_Ring_Buffer can_tx_buffer;


CANChannel Can1(HAL_CAN_Channel1, &can_rx_buffer, &can_tx_buffer);
// optional Serial2 is instantiated from libraries/Serial2/Serial2.h
#endif

