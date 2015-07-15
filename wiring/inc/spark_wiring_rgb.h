/**
 ******************************************************************************
 * @file    spark_wiring_rgb.h
 * @author  Satish Nair, Zachary Crockett, Matthew McGowan
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

#include <stdint.h>
#include "rgbled.h"

class RGBClass {
private:
    static bool _control;
public:
    static bool controlled(void);
    static void control(bool);
    static void color(int, int, int);
    static void color(uint32_t rgb);
    static void brightness(uint8_t, bool update=true);
    static void attachHandler(led_update_handler_fn fn, void* data);
};

extern RGBClass RGB;
