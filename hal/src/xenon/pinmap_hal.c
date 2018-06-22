/*
 * Copyright (c) 2018 Particle Industries, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

/* Includes ------------------------------------------------------------------*/
#include "pinmap_hal.h"
#include "pinmap_impl.h"

NRF5x_Pin_Info __PIN_MAP[TOTAL_PINS] =
{
/* D0 / SDA      - 00 */ { NRF_PORT_0,    26, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, CHANNEL_NONE},
/* D1 / SCL      - 01 */ { NRF_PORT_0,    27, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, CHANNEL_NONE},
/* D2 /          - 02 */ { NRF_PORT_1,    1,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, CHANNEL_NONE},
/* D3 /          - 03 */ { NRF_PORT_1,    2,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, CHANNEL_NONE},
/* D4            - 04 */ { NRF_PORT_1,    8,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, 1,                 0           },
/* D5            - 05 */ { NRF_PORT_1,    10, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, 0,                 0           },
/* D6            - 06 */ { NRF_PORT_1,    11, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, 1,                 1           },
/* D7            - 07 */ { NRF_PORT_1,    12, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, 1,                 2           },
/* D8            - 08 */ { NRF_PORT_1,    3,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, 1,                 3           },
/* D9            - 09 */ { NRF_PORT_0,    6,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, CHANNEL_NONE},
/* D10           - 10 */ { NRF_PORT_0,    8,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, CHANNEL_NONE},
/* D11           - 11 */ { NRF_PORT_1,    14, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, CHANNEL_NONE},
/* D12           - 12 */ { NRF_PORT_1,    13, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, CHANNEL_NONE},
/* D13           - 13 */ { NRF_PORT_1,    15, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, CHANNEL_NONE},
/* A5            - 14 */ { NRF_PORT_0,    31, PIN_MODE_NONE, PF_NONE, 7,                PWM_INSTANCE_NONE, CHANNEL_NONE},
/* A4            - 15 */ { NRF_PORT_0,    30, PIN_MODE_NONE, PF_NONE, 6,                PWM_INSTANCE_NONE, CHANNEL_NONE},
/* A3            - 16 */ { NRF_PORT_0,    29, PIN_MODE_NONE, PF_NONE, 5,                2,                 0,          },
/* A2            - 17 */ { NRF_PORT_0,    28, PIN_MODE_NONE, PF_NONE, 4,                2,                 1,          }, 
/* A1            - 18 */ { NRF_PORT_0,    4,  PIN_MODE_NONE, PF_NONE, 2,                2,                 2,          }, 
/* A0            - 19 */ { NRF_PORT_0,    3,  PIN_MODE_NONE, PF_NONE, 1,                2,                 3,          },
/* MODE BUTTON   - 20 */ { NRF_PORT_0,    11, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, CHANNEL_NONE}, 
/* RGBR          - 21 */ { NRF_PORT_0,    13, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, 0,                 1,          }, 
/* RGBG          - 22 */ { NRF_PORT_0,    14, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, 0,                 2,          }, 
/* RGBB          - 23 */ { NRF_PORT_0,    15, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, 0,                 3,          }, 
/* BATT          - 24 */ { NRF_PORT_0,    5,  PIN_MODE_NONE, PF_NONE, 3,                PWM_INSTANCE_NONE, CHANNEL_NONE},
/* PWR           - 25 */ { NRF_PORT_0,    12, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, CHANNEL_NONE},
/* CHG           - 26 */ { NRF_PORT_1,    9,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, CHANNEL_NONE},
/* NFC_PIN1      - 27 */ { NRF_PORT_0,    9,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, CHANNEL_NONE},
/* NFC_PIN2      - 28 */ { NRF_PORT_0,    10, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, CHANNEL_NONE},
/* ANTSW1        - 29 */ { NRF_PORT_0,    24, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, CHANNEL_NONE},
/* ANTSW2        - 30 */ { NRF_PORT_0,    25, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, CHANNEL_NONE},
};

NRF5x_Pin_Info* HAL_Pin_Map(void) {
    return __PIN_MAP;
}
