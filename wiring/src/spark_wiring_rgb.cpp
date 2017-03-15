/**
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.
  Copyright (c) 2011 Adrian McEwen

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

#include "spark_wiring_rgb.h"
#include "spark_wiring_interrupts.h"

#include "core_hal.h"

RGBClass RGB;

bool RGBClass::controlled(void)
{
    return LED_RGB_IsOverRidden();
}

void RGBClass::control(bool override)
{
    if (override == controlled()) {
        return;
    } else if (override) {
        LED_Signaling_Start();
    } else {
        LED_Signaling_Stop();
    }
}

void RGBClass::color(uint32_t rgb) {
    color((rgb>>16)&0xFF, (rgb>>8)&0xFF, (rgb)&0xFF);
}

void RGBClass::color(int red, int green, int blue)
{
    if (!controlled()) {
        return;
    }
    LED_SetSignalingColor(red << 16 | green << 8 | blue);
    LED_On(LED_RGB);
}

void RGBClass::brightness(uint8_t brightness, bool update)
{
    LED_SetBrightness(brightness);
    if (controlled() && update) {
        LED_On(LED_RGB);
    }
}

uint8_t RGBClass::brightness()
{
    return Get_LED_Brightness();
}

void RGBClass::onChange(wiring_rgb_change_handler_t handler)
{
    // User callback for LED changes can be called from an ISR, so we're masking interrupts before
    // changing current callback function
    ATOMIC_BLOCK() {
        changeHandler_ = std::move(handler);
        if (changeHandler_) {
            LED_RGB_SetChangeHandler(ledChangeHandler, this);
        } else {
            LED_RGB_SetChangeHandler(nullptr, nullptr);
        }
    }
}

void RGBClass::onChange(raw_rgb_change_handler_t* handler)
{
    onChange(wiring_rgb_change_handler_t(handler));
}

void RGBClass::mirrorTo(pin_t rpin, pin_t gpin, pin_t bpin, bool invert, bool bootloader)
{
    HAL_Core_Led_Mirror_Pin(LED_RED + LED_MIRROR_OFFSET, rpin, (uint32_t)invert, (uint8_t)bootloader, nullptr);
    HAL_Core_Led_Mirror_Pin(LED_GREEN + LED_MIRROR_OFFSET, gpin, (uint32_t)invert, (uint8_t)bootloader, nullptr);
    HAL_Core_Led_Mirror_Pin(LED_BLUE + LED_MIRROR_OFFSET, bpin, (uint32_t)invert, (uint8_t)bootloader, nullptr);
}

void RGBClass::mirrorDisable(bool bootloader)
{
    HAL_Core_Led_Mirror_Pin_Disable(LED_RED + LED_MIRROR_OFFSET, (uint8_t)bootloader, nullptr);
    HAL_Core_Led_Mirror_Pin_Disable(LED_GREEN + LED_MIRROR_OFFSET, (uint8_t)bootloader, nullptr);
    HAL_Core_Led_Mirror_Pin_Disable(LED_BLUE + LED_MIRROR_OFFSET, (uint8_t)bootloader, nullptr);
}

void RGBClass::ledChangeHandler(void* data, uint8_t r, uint8_t g, uint8_t b, void* reserved)
{
    RGBClass* const d = static_cast<RGBClass*>(data);
    if (d->changeHandler_) {
        d->changeHandler_(r, g, b);
    }
}
