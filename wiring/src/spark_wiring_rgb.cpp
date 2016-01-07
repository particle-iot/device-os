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
#include "rgbled.h"

bool RGBClass::controlled(void)
{
    return LED_RGB_IsOverRidden();
}

void RGBClass::control(bool override)
{
    if(override == controlled())
            return;
    else if (override)
            LED_Signaling_Start();
    else
            LED_Signaling_Stop();
}

void RGBClass::color(uint32_t rgb) {
    color((rgb>>16)&0xFF, (rgb>>8)&0xFF, (rgb)&0xFF);
}

void RGBClass::color(int red, int green, int blue)
{
    if (!controlled())
            return;

    LED_SetSignalingColor(red << 16 | green << 8 | blue);
    LED_On(LED_RGB);
}

void RGBClass::brightness(uint8_t brightness, bool update)
{
    LED_SetBrightness(brightness);
    if (controlled() && update)
        LED_On(LED_RGB);
}

void RGBClass::onChange(wiring_rgb_change_handler_t handler) {
  if(handler) {
    auto wrapper = new wiring_rgb_change_handler_t(handler);
    if(wrapper) {
      LED_RGB_SetChangeHandler(call_std_change_handler, wrapper);
    }
  }
  else {
      LED_RGB_SetChangeHandler(NULL, NULL);
  }
}

void RGBClass::onChange(raw_rgb_change_handler_t *handler) {
    LED_RGB_SetChangeHandler(handler ? call_raw_change_handler : NULL, (void*)handler);
}

void RGBClass::call_raw_change_handler(void* data, uint8_t r, uint8_t g, uint8_t b, void* reserved)
{
    auto fn = (raw_rgb_change_handler_t*)(data);
    (*fn)(r, g, b);
}

void RGBClass::call_std_change_handler(void* data, uint8_t r, uint8_t g, uint8_t b, void* reserved)
{
    auto fn = (wiring_rgb_change_handler_t*)(data);
    (*fn)(r, g, b);
}
