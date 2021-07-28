/*
 * Copyright (c) 2021 Particle Industries, Inc.  All rights reserved.
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

#include "pinmap_hal.h"

namespace {

static hal_pin_info_t pinmap[TOTAL_PINS] = {
/* User space */
/* D0 / SDA      - 00 */ { RTL_PORT_A, 26, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, 1,                 5,              },
/* D1 / SCL      - 01 */ { RTL_PORT_A, 25, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, 1,                 4,              },
/* D2 /          - 02 */ { RTL_PORT_A, 16, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE},
/* D3 /          - 03 */ { RTL_PORT_A, 17, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE},
/* D4            - 04 */ { RTL_PORT_A, 18, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE},
/* D5            - 05 */ { RTL_PORT_A, 19, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE},
/* D6            - 06 */ { RTL_PORT_B, 3,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE},
/* D7            - 07 */ { RTL_PORT_A, 27, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE},
/* D8            - 08 */ { RTL_PORT_A, 28, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, 1                , 6,              },
/* D9            - 09 */ { RTL_PORT_A, 7,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE},
/* D10           - 10 */ { RTL_PORT_A, 8,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE},
/* D11           - 11 */ { RTL_PORT_A, 15, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE},
/* A0            - 12 */ { RTL_PORT_B, 1,  PIN_MODE_NONE, PF_NONE, 0,                PWM_INSTANCE_NONE, PWM_CHANNEL_NONE},
/* A1            - 13 */ { RTL_PORT_B, 2,  PIN_MODE_NONE, PF_NONE, 1,                PWM_INSTANCE_NONE, PWM_CHANNEL_NONE},
/* A2            - 14 */ { RTL_PORT_B, 7,  PIN_MODE_NONE, PF_NONE, 2,                1,                 17,             },
/* A3            - 15 */ { RTL_PORT_B, 6,  PIN_MODE_NONE, PF_NONE, 3,                PWM_INSTANCE_NONE, PWM_CHANNEL_NONE},
/* A4            - 16 */ { RTL_PORT_B, 5,  PIN_MODE_NONE, PF_NONE, 4,                1,                 9,              },
/* A5            - 17 */ { RTL_PORT_B, 4,  PIN_MODE_NONE, PF_NONE, 5,                1,                 8,              },
/* S0            - 18 */ { RTL_PORT_A, 12, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, 1,                 0,              },
/* S1            - 19 */ { RTL_PORT_A, 13, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, 1,                 1,              },
/* S2            - 20 */ { RTL_PORT_A, 14, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE},
/* S3            - 21 */ { RTL_PORT_B, 26, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE},
/* S4            - 22 */ { RTL_PORT_A, 0,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE},
/* S5            - 23 */ { RTL_PORT_B, 29, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE},
/* S6            - 24 */ { RTL_PORT_B, 31, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE},

/* System space */
/* RGBR          - 25 */ { RTL_PORT_A, 30, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, 0,                 1,              },
/* RGBG          - 26 */ { RTL_PORT_B, 23, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, 0,                 3,              },
/* RGBB          - 27 */ { RTL_PORT_B, 22, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, 0,                 2,              },
/* MODE BUTTON   - 28 */ { RTL_PORT_A, 4,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE},
/* ANTSW         - 29 */ { RTL_PORT_A, 2,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE},
};

} // anonymous

hal_pin_info_t* hal_pin_map(void) {
    return pinmap;
}
