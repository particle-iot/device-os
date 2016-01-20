/**
 ******************************************************************************
 * @file    spark_wiring_can.cpp
 * @author  Brian Spranger, Julien Vanier
 * @version V1.0.0
 * @date    04-Jan-2016
 * @brief   Wiring wrapper for hardware CAN module
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

#include "spark_wiring_can.h"
#include "spark_wiring_constants.h"

// Constructors ////////////////////////////////////////////////////////////////

CANChannel::CANChannel(HAL_CAN_Channel channel,
        uint16_t rxQueueSize, uint16_t txQueueSize)
{
    _channel = channel;
    HAL_CAN_Init(_channel, rxQueueSize, txQueueSize, NULL);
}
// Public Methods //////////////////////////////////////////////////////////////

void CANChannel::begin(unsigned long baud, uint32_t flags)
{
    HAL_CAN_Begin(_channel, baud, flags, NULL);
}

void CANChannel::end()
{
    HAL_CAN_End(_channel, NULL);
}

uint8_t CANChannel::available(void)
{
    return HAL_CAN_Available_Messages(_channel, NULL);
}

bool CANChannel::receive(CANMessage &message)
{
    return HAL_CAN_Receive(_channel, &message, NULL);
}

bool CANChannel::transmit(const CANMessage &message)
{
    return HAL_CAN_Transmit(_channel, &message, NULL);
}

bool CANChannel::addFilter(uint32_t id, uint32_t mask, HAL_CAN_Filters type)
{
    return HAL_CAN_Add_Filter(_channel, id, mask, type, NULL);
}

void CANChannel::clearFilters()
{
    HAL_CAN_Clear_Filters(_channel, NULL);
}

bool CANChannel::isEnabled() {
    return HAL_CAN_Is_Enabled(_channel);
}

HAL_CAN_Errors CANChannel::errorStatus() {
    return HAL_CAN_Error_Status(_channel);
}
