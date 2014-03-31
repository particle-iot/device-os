/**
 ******************************************************************************
 * @file    spark_wiring_usbkeyboard.h
 * @author  Satish Nair
 * @version V1.0.0
 * @date    31-March-2014
 * @brief   Header for spark_wiring_usbkeyboard.c module
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

#ifndef __SPARK_WIRING_USBKEYBOARD_H
#define __SPARK_WIRING_USBKEYBOARD_H

#include "spark_wiring.h"

class USBKeyboard
{
private:

public:
	// public methods
	USBKeyboard();

	void begin();
	void end();
	void press();
	void release();
};

extern USBKeyboard Keyboard;

#endif
