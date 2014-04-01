/**
 ******************************************************************************
 * @file    spark_wiring_usbmouse.cpp
 * @author  Satish Nair
 * @version V1.0.0
 * @date    31-March-2014
 * @brief   Wrapper for wiring usb mouse hid module
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

#include "spark_wiring_usbmouse.h"

//
// Constructor
//
USBMouse::USBMouse(void)
{
	for(int i = 0 ; i < 4 ; i++)
	{
		mouseBuffer[i] = 0;
	}
}

void USBMouse::buttons(uint8_t button)
{
	if (mouseBuffer[0] != button)
	{
		mouseBuffer[0] = button;
		move(0,0,0);
	}
}

void USBMouse::begin(void)
{
	SPARK_USB_Setup();
}

void USBMouse::end(void)
{
	//To Do
}

void USBMouse::move(uint8_t x, uint8_t y, uint8_t wheel)
{
	mouseBuffer[1] = x;
	mouseBuffer[2] = y;
	mouseBuffer[3] = wheel;
	USB_HID_Send(mouseBuffer, 4);
}

void USBMouse::click(uint8_t button)
{
	mouseBuffer[0] = button;
	move(0,0,0);
	mouseBuffer[0] = 0;
	move(0,0,0);
}

void USBMouse::press(uint8_t button)
{
	buttons(mouseBuffer[0] | button);
}

void USBMouse::release(uint8_t button)
{
	buttons(mouseBuffer[0] & ~button);
}

bool USBMouse::isPressed(uint8_t button)
{
	if ((mouseBuffer[0] & button) > 0)
	{
		return true;
	}

	return false;
}

//Preinstantiate Object
USBMouse Mouse;
