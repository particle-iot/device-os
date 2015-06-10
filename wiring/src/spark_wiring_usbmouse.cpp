/**
 ******************************************************************************
 * @file    spark_wiring_usbmouse.cpp
 * @author  Satish Nair
 * @version V1.0.0
 * @date    31-March-2014
 * @brief   Wrapper for wiring usb mouse hid module
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

#ifdef SPARK_USB_MOUSE
#include "spark_wiring_usbmouse.h"

//
// Constructor
//
USBMouse::USBMouse(void)
{
}

void USBMouse::buttons(uint8_t button)
{
	if (mouseReport.buttons != button)
	{
		mouseReport.buttons = button;
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

void USBMouse::move(int8_t x, int8_t y, int8_t wheel)
{
	mouseReport.x = x;
	mouseReport.y = y;
	mouseReport.wheel = wheel;
	USB_HID_Send_Report(&mouseReport, sizeof(mouseReport));
}

void USBMouse::click(uint8_t button)
{
	mouseReport.buttons = button;
	move(0,0,0);
	delay(100);
	mouseReport.buttons = 0;
	move(0,0,0);
}

void USBMouse::press(uint8_t button)
{
	buttons(mouseReport.buttons | button);
}

void USBMouse::release(uint8_t button)
{
	buttons(mouseReport.buttons & ~button);
}

bool USBMouse::isPressed(uint8_t button)
{
	if ((mouseReport.buttons & button) > 0)
	{
		return true;
	}

	return false;
}

//Preinstantiate Object
USBMouse Mouse;
#endif
