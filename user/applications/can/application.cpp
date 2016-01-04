/**
 ******************************************************************************
 * @file    application.cpp
 * @authors Brian Spranger
 * @version V1.0.1
 * @date    15 October 2015
 * @brief   CAN Test application
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

/* Includes ------------------------------------------------------------------*/
#include "application.h"

CANChannel can(CAN_D1_D2);
#ifdef HAL_HAS_CAN_C4_C5
CANChannel canb(CAN_C4_C5);
#endif

SYSTEM_THREAD(ENABLED);

auto& console = Serial;

/* This function is called once at start up ----------------------------------*/
void setup()
{
    console.begin(9600);

    can.begin(125000, CAN_TEST_MODE);
    // Only accept message with even ids
    can.addFilter(0, 1);

#ifdef HAL_HAS_CAN_C4_C5
    canb.begin(500000, CAN_TEST_MODE);
    // Only accept message with odd ids
    canb.addFilter(1, 1);
#endif
}

/* This function loops forever --------------------------------------------*/
void loop()
{
    CANMessage Message;

    for(int i = 0; i < 15; i++) {
        Message.id = 100 + i;
        can.transmit(Message);
#ifdef HAL_HAS_CAN_C4_C5
        canb.transmit(Message);
#endif
    }

    delay(100);
    console.print("CAN received ");
    while(can.receive(Message))
    {
        console.print(String(Message.id, DEC));
        console.print(", ");
    }
    console.print("\n");

#ifdef HAL_HAS_CAN_C4_C5
    console.print("CANB received ");
    while(canb.receive(Message))
    {
        console.print(String(Message.id, DEC));
        console.print(", ");
    }
    console.print("\n");
#endif

    delay(1000);
}
