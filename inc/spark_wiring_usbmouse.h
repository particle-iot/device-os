/**
 ******************************************************************************
 * @file    spark_wiring_usbmouse.h
 * @author  Satish Nair
 * @version V1.0.0
 * @date    31-March-2014
 * @brief   Header for spark_wiring_usbmouse.c module
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

#ifndef __SPARK_WIRING_USBMOUSE_H
#define __SPARK_WIRING_USBMOUSE_H

#include "spark_wiring.h"

#define MOUSE_LEFT		0x01
#define MOUSE_RIGHT		0x02
#define MOUSE_MIDDLE	0x04
#define MOUSE_ALL		(MOUSE_LEFT | MOUSE_RIGHT | MOUSE_MIDDLE)

class USBMouse
{
private:
	uint8_t mouseBuffer[4];//Unsigned type
	//mouseBuffer[0] : Buttons (Bit 0 -> Right, Bit 1 -> Middle, Bit 2 -> Left)
	//mouseBuffer[1] : X Axis Relative Movement
	//mouseBuffer[2] : Y Axis Relative Movement
	//mouseBuffer[3] : Wheel Scroll
	void buttons(uint8_t button);

public:
	USBMouse(void);

	void begin(void);
	void end(void);
	void move(uint8_t x, uint8_t y, uint8_t wheel);
	void click(uint8_t button = MOUSE_LEFT);
	void press(uint8_t button = MOUSE_LEFT);		// press LEFT by default
	void release(uint8_t button = MOUSE_LEFT);		// release LEFT by default
	bool isPressed(uint8_t button = MOUSE_LEFT);	// check LEFT by default
};

extern USBMouse Mouse;

#endif
