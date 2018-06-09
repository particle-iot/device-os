/**
 ******************************************************************************
 * @file    pinmap_hal.c
 * @authors Eugene
 * @version V1.0.0
 * @date    13-3-2018
 * @brief
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

/* Includes ------------------------------------------------------------------*/
#include "pinmap_hal.h"
#include "pinmap_impl.h"

NRF5x_Pin_Info __PIN_MAP[TOTAL_PINS] =
{
/* D0 / RX       - 00 */ {PF_NONE, NRF_PORT_0, 8,  PIN_MODE_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, CHANNEL_NONE},
/* D1 / TX       - 01 */ {PF_NONE, NRF_PORT_0, 6,  PIN_MODE_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, CHANNEL_NONE},
/* D2 / SDA      - 02 */ {PF_NONE, NRF_PORT_0, 26, PIN_MODE_NONE, ADC_CHANNEL_NONE, 2,                 0,          },
/* D3 / SCL      - 03 */ {PF_NONE, NRF_PORT_0, 27, PIN_MODE_NONE, ADC_CHANNEL_NONE, 2,                 1,          },
/* D4            - 04 */ {PF_NONE, NRF_PORT_1, 1,  PIN_MODE_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, CHANNEL_NONE},
/* D5            - 05 */ {PF_NONE, NRF_PORT_1, 2,  PIN_MODE_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, CHANNEL_NONE},
/* D6            - 06 */ {PF_NONE, NRF_PORT_1, 8,  PIN_MODE_NONE, ADC_CHANNEL_NONE, 1,                 0,          },
/* D7 / LED      - 07 */ {PF_NONE, NRF_PORT_1, 10, PIN_MODE_NONE, ADC_CHANNEL_NONE, 0,                 0,          },
/* D8            - 08 */ {PF_NONE, NRF_PORT_1, 11, PIN_MODE_NONE, ADC_CHANNEL_NONE, 1,                 1,          },
/* D9            - 09 */ {PF_NONE, NRF_PORT_1, 12, PIN_MODE_NONE, ADC_CHANNEL_NONE, 1,                 2,          },
/* D10           - 10 */ {PF_NONE, NRF_PORT_1, 3,  PIN_MODE_NONE, ADC_CHANNEL_NONE, 1,                 3,          },
/* D11 / MOSI    - 11 */ {PF_NONE, NRF_PORT_1, 13, PIN_MODE_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, CHANNEL_NONE},
/* D12 / MISO    - 12 */ {PF_NONE, NRF_PORT_1, 14, PIN_MODE_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, CHANNEL_NONE},
/* D13 / SCK     - 13 */ {PF_NONE, NRF_PORT_1, 15, PIN_MODE_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, CHANNEL_NONE},
/* A0            - 14 */ {PF_NONE, NRF_PORT_0, 3,  PIN_MODE_NONE, 1,                3,                 0,          },
/* A1            - 15 */ {PF_NONE, NRF_PORT_0, 4,  PIN_MODE_NONE, 2,                3,                 1,          },
/* A2            - 16 */ {PF_NONE, NRF_PORT_0, 28, PIN_MODE_NONE, 4,                3,                 2,          },
/* A3            - 17 */ {PF_NONE, NRF_PORT_0, 29, PIN_MODE_NONE, 5,                3,                 3,          }, 
/* A4            - 18 */ {PF_NONE, NRF_PORT_0, 30, PIN_MODE_NONE, 6,                2,                 2,          }, 
/* A5            - 19 */ {PF_NONE, NRF_PORT_0, 31, PIN_MODE_NONE, 7,                2,                 3,          },
/* MODE BUTTON   - 20 */ {PF_NONE, NRF_PORT_0, 11, PIN_MODE_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, CHANNEL_NONE}, 
/* RGBR          - 21 */ {PF_NONE, NRF_PORT_0, 13, PIN_MODE_NONE, ADC_CHANNEL_NONE, 0,                 1,          }, 
/* RGBG          - 22 */ {PF_NONE, NRF_PORT_0, 14, PIN_MODE_NONE, ADC_CHANNEL_NONE, 0,                 2,          }, 
/* RGBB          - 23 */ {PF_NONE, NRF_PORT_0, 15, PIN_MODE_NONE, ADC_CHANNEL_NONE, 0,                 3,          }, 
};

NRF5x_Pin_Info* HAL_Pin_Map(void) {
    return __PIN_MAP;
}
