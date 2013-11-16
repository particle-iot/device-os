/**
  ******************************************************************************
  * @file    coap.cpp
  * @authors  Zachary Crockett
  * @version V1.0.0
  * @date    15-Nov-2013
  * @brief   COAP
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
#include "coap.h"

CoAPCode::Enum CoAP::code(const unsigned char *message)
{
  switch (message[1])
  {
    case 0x00: return CoAPCode::EMPTY;
    case 0x01: return CoAPCode::GET;
    case 0x02: return CoAPCode::POST;
    case 0x03: return CoAPCode::PUT;
    default: return CoAPCode::ERROR;
  }
}
