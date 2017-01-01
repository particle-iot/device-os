/**
 ******************************************************************************
 * @file    spark_wiring_usbmouse.h
 * @author  Satish Nair
 * @version V1.0.0
 * @date    31-March-2014
 * @brief   Header for spark_wiring_usbmouse.c module
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

#ifndef __SPARK_WIRING_USBMOUSE_H
#define __SPARK_WIRING_USBMOUSE_H

#include "usb_config_hal.h"

#ifdef SPARK_USB_MOUSE
#include "spark_wiring.h"
#include <array>

#define MOUSE_LEFT      0x01
#define MOUSE_RIGHT     0x02
#define MOUSE_MIDDLE    0x04
#define MOUSE_ALL       (MOUSE_LEFT | MOUSE_RIGHT | MOUSE_MIDDLE)

class USBMouse
{
public:
    typedef std::array<float, 4> ScreenMargin;
private:

#pragma pack(push, 1)
    typedef struct
    {
        uint8_t reportId; // 0x01
        uint8_t buttons;
        int16_t x;
        int16_t y;
        int8_t wheel;
    } MouseReport;

    typedef struct
    {
        uint8_t reportId; // 0x03
        uint8_t sw;
        int16_t x;
        int16_t y;
    } DigitizerReport;
#pragma pack(pop)

    MouseReport mouseReport;
    DigitizerReport digitizerReport;
    uint16_t screenWidth_;
    uint16_t screenHeight_;
    ScreenMargin margin_;
    bool digitizerState_;

    void buttons(uint8_t button);
    void sendMouseReport();
    void sendDigitizerReport();

public:
    USBMouse(void);

    void begin(void);
    void end(void);
    void moveTo(int16_t x, int16_t y);

    void enableMoveTo(bool state);

    void screenSize(uint16_t width, uint16_t height,
                    float marginLeft = 0.0f, float marginRight = 0.0f,
                    float marginTop = 0.0f, float marginBottom = 0.0f);

    void screenSize(uint16_t width, uint16_t height, const ScreenMargin& margin);

    void move(int16_t x, int16_t y, int8_t wheel = 0);
    void scroll(int8_t wheel) {
        move(0, 0, wheel);
    }
	void click(uint8_t button = MOUSE_LEFT);
	void press(uint8_t button = MOUSE_LEFT);		// press LEFT by default
	void release(uint8_t button = MOUSE_LEFT);		// release LEFT by default
	bool isPressed(uint8_t button = MOUSE_LEFT);	// check LEFT by default
};

extern USBMouse& _fetch_usbmouse();
#define Mouse _fetch_usbmouse()

#endif

#endif
