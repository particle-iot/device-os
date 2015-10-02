/**
 ******************************************************************************
 * @file    spark_wiring_can.h
 * @author  Brian Spranger
 * @version V1.0.0
 * @date    01 October 2015
 * @brief   Header for spark_wiring_can.c module
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

#ifndef __SPARK_WIRING_CAN_H
#define __SPARK_WIRING_CAN_H

#include "spark_wiring_stream.h"
#include "can_hal.h"

class CANChannel : public Stream
{
private:
  HAL_CAN_Channel _channel;
public:
  CANChannel(HAL_CAN_Channel channel, CAN_Ring_Buffer *rx_buffer, CAN_Ring_Buffer *tx_buffer);
  virtual ~CANChannel() {};
  void begin(unsigned long);
  void begin(unsigned long, uint8_t);
  void end();

  virtual int available(void);
  virtual int peek(CAN_Message_Struct *pmessage);
  virtual int read(CAN_Message_Struct *pmessage);
  virtual void flush(void);
  virtual size_t write(CAN_Message_Struct *pmessage);

  operator bool();

  bool isEnabled(void);
};

#ifndef SPARK_WIRING_NO_CAN
//extern CANChannel CANChannel1;
#endif

#endif
