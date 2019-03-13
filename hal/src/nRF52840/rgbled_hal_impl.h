/**
 ******************************************************************************
  Copyright (c) 2013-2016 Particle Industries, Inc.  All rights reserved.

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

#ifndef RGBLED_HAL_IMPL_H_
#define RGBLED_HAL_IMPL_H_

#include <stdint.h>

#ifdef  __cplusplus
extern "C" {
#endif

#define LED_CONFIG_STRUCT_VERSION           0x01
typedef struct led_config_t {
    uint8_t             version;        // Struct version
    uint16_t            pin;
    union {
        struct {
            uint8_t is_active   : 1;    // Is LED active?
            uint8_t is_inverted : 1;    // If LED is "inverted", active state is 0, instead of 1
        };
    };
    uint8_t             padding[18];
} led_config_t;

#ifdef  __cplusplus
}
#endif

#endif  /* RGBLED_HAL_IMPL_H_ */
