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

#include "usb_hal.h"

#ifdef SPARK_USB_MOUSE
#include "spark_wiring_usbmouse.h"

//
// Constructor
//
USBMouse::USBMouse(void)
{
    memset((void*)&mouseReport, 0, sizeof(mouseReport));
    mouseReport.reportId = 0x01;
    HAL_USB_HID_Init(0, NULL);
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
	HAL_USB_HID_Begin(0, NULL);
}

void USBMouse::end(void)
{
	HAL_USB_HID_End(0);
}

void USBMouse::move(int8_t x, int8_t y, int8_t wheel)
{
	mouseReport.x = x;
	mouseReport.y = y;
	mouseReport.wheel = wheel;
	sendReport();
}

void USBMouse::click(uint8_t button)
{
	mouseReport.buttons = button;
	move(0,0,0);
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

void USBMouse::sendReport()
{
    uint32_t m = millis();
    while(HAL_USB_HID_Status(0, nullptr)) {
        // Wait 1 bInterval (1ms)
        delay(1);
        if ((millis() - m) >= 50)
            return;
    }
    HAL_USB_HID_Send_Report(0, &mouseReport, sizeof(mouseReport), NULL);
    m = millis();
    while(HAL_USB_HID_Status(0, nullptr)) {
        // Wait 1 bInterval (1ms)
        delay(1);
        if ((millis() - m) >= 50)
            return;
    }
}

//Preinstantiate Object
USBMouse Mouse;
#endif
