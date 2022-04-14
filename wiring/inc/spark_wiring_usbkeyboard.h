/**
 ******************************************************************************
 * @file    spark_wiring_usbkeyboard.h
 * @author  Satish Nair
 * @version V1.0.0
 * @date    31-March-2014
 * @brief   Header for spark_wiring_usbkeyboard.c module
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

#ifndef __SPARK_WIRING_USBKEYBOARD_H
#define __SPARK_WIRING_USBKEYBOARD_H

#include "usb_config_hal.h"

#ifdef SPARK_USB_KEYBOARD
#include "spark_wiring.h"
#include "spark_wiring_usbkeyboard_scancode.h"

typedef struct
{
    uint8_t reportId; // 0x02
    uint8_t modifiers;
    uint8_t reserved;
    uint8_t keys[6];
} KeyReport;

class USBKeyboard : public Print
{
private:
    KeyReport keyReport;

public:
    USBKeyboard(void);

    void begin(void);
    void end(void);

    using Print::write;
    virtual size_t write(uint8_t ch);

    virtual size_t writeKey(uint16_t k, uint16_t modifiers = 0);
    virtual size_t click(uint16_t k, uint16_t modifiers = 0);
    virtual size_t press(uint16_t k, uint16_t modifiers = 0);
    virtual size_t release(uint16_t k, uint16_t modifiers = 0);
    virtual void releaseAll(void);

private:
    void sendReport();
};

extern USBKeyboard& _fetch_usbkeyboard();
#define Keyboard _fetch_usbkeyboard()

#endif

#endif
