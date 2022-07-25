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

#define MCP23S17_PORT_A ((uint8_t)0u)
#define MCP23S17_PORT_B ((uint8_t)1u)
#define DEMUX_PORT      ((uint8_t)0u)

static hal_pin_info_t pinmap[TOTAL_PINS] = {
/* User space */
/* D0 / A3       - 00 */ { RTL_PORT_B,      6,  PIN_MODE_NONE, PF_NONE, 2,                PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, HAL_PIN_TYPE_MCU, 0},
/* D1 / A4       - 01 */ { RTL_PORT_B,      5,  PIN_MODE_NONE, PF_NONE, 1,                1,                 9,                HAL_PIN_TYPE_MCU, 0},
/* D2            - 02 */ { RTL_PORT_A,      16, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, HAL_PIN_TYPE_MCU, 0},
/* D3            - 03 */ { RTL_PORT_A,      17, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, HAL_PIN_TYPE_MCU, 0},
/* D4            - 04 */ { RTL_PORT_A,      18, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, HAL_PIN_TYPE_MCU, 0},
/* D5            - 05 */ { RTL_PORT_A,      19, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, HAL_PIN_TYPE_MCU, 0},
/* D6            - 06 */ { RTL_PORT_B,      3,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, HAL_PIN_TYPE_MCU, 0},
/* D7            - 07 */ { RTL_PORT_A,      27, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, HAL_PIN_TYPE_MCU, 0},
/* D8            - 08 */ { RTL_PORT_A,      7,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, HAL_PIN_TYPE_MCU, 0},
/* D9            - 09 */ { RTL_PORT_A,      8,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, HAL_PIN_TYPE_MCU, 0},
/* D10           - 10 */ { RTL_PORT_A,      15, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, HAL_PIN_TYPE_MCU, 0},
/* A0            - 11 */ { RTL_PORT_B,      1,  PIN_MODE_NONE, PF_NONE, 4,                PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, HAL_PIN_TYPE_MCU, 0},
/* A1            - 12 */ { RTL_PORT_B,      2,  PIN_MODE_NONE, PF_NONE, 5,                PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, HAL_PIN_TYPE_MCU, 0},
/* A2            - 13 */ { RTL_PORT_B,      7,  PIN_MODE_NONE, PF_NONE, 3,                1,                 17,               HAL_PIN_TYPE_MCU, 0},
/* A5            - 14 */ { RTL_PORT_B,      4,  PIN_MODE_NONE, PF_NONE, 0,                1,                 8,                HAL_PIN_TYPE_MCU, 0},
/* S0            - 15 */ { RTL_PORT_A,      12, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, 1,                 0,                HAL_PIN_TYPE_MCU, 0},
/* S1            - 16 */ { RTL_PORT_A,      13, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, 1,                 1,                HAL_PIN_TYPE_MCU, 0},
/* S2            - 17 */ { RTL_PORT_A,      14, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, HAL_PIN_TYPE_MCU, 0},
/* S3            - 18 */ { RTL_PORT_B,      26, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, HAL_PIN_TYPE_MCU, 0},
/* S4            - 19 */ { RTL_PORT_A,      0,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, HAL_PIN_TYPE_MCU, 0},
/* S5            - 20 */ { RTL_PORT_B,      29, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, HAL_PIN_TYPE_MCU, 0},
/* S6            - 21 */ { RTL_PORT_B,      31, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, HAL_PIN_TYPE_MCU, 0},

/* System space */
/* RGBR          - 22 */ { RTL_PORT_A,      30, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, 0,                 1,                HAL_PIN_TYPE_MCU, 0},
/* RGBG          - 23 */ { RTL_PORT_B,      23, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, 0,                 3,                HAL_PIN_TYPE_MCU, 0},
/* RGBB          - 24 */ { RTL_PORT_B,      22, PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, 0,                 2,                HAL_PIN_TYPE_MCU, 0},
/* MODE BUTTON   - 25 */ { RTL_PORT_A,      4,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, HAL_PIN_TYPE_MCU, 0},
/* ANTSW         - 26 */ { RTL_PORT_A,      2,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, HAL_PIN_TYPE_MCU, 0},
/* LOW_BAT_UC    - 27 */ { MCP23S17_PORT_A, 0,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, HAL_PIN_TYPE_IO_EXPANDER, 0},
/* SENSOR_INT1   - 28 */ { MCP23S17_PORT_A, 1,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, HAL_PIN_TYPE_IO_EXPANDER, 0},
/* CAN_STBY      - 29 */ { MCP23S17_PORT_A, 2,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, HAL_PIN_TYPE_IO_EXPANDER, 0},
/* CAN_INT       - 30 */ { MCP23S17_PORT_A, 3,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, HAL_PIN_TYPE_IO_EXPANDER, 0},
/* PGOOD         - 31 */ { MCP23S17_PORT_A, 4,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, HAL_PIN_TYPE_IO_EXPANDER, 0},
/* DCDC_EN       - 32 */ { MCP23S17_PORT_A, 5,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, HAL_PIN_TYPE_IO_EXPANDER, 0},
/* GNSS_PWR_EN   - 33 */ { MCP23S17_PORT_A, 6,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, HAL_PIN_TYPE_IO_EXPANDER, 0},
/* GNSS_INT      - 34 */ { MCP23S17_PORT_A, 7,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, HAL_PIN_TYPE_IO_EXPANDER, 0},
/* GNSS_SAFEBOOT - 35 */ { MCP23S17_PORT_B, 0,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, HAL_PIN_TYPE_IO_EXPANDER, 0},
/* GNSS_RST      - 36 */ { MCP23S17_PORT_B, 1,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, HAL_PIN_TYPE_IO_EXPANDER, 0},
/* CAN_RTS1      - 37 */ { MCP23S17_PORT_B, 2,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, HAL_PIN_TYPE_IO_EXPANDER, 0},
/* CAN_RTS2      - 38 */ { MCP23S17_PORT_B, 3,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, HAL_PIN_TYPE_IO_EXPANDER, 0},
/* CAN_RTS0      - 39 */ { MCP23S17_PORT_B, 4,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, HAL_PIN_TYPE_IO_EXPANDER, 0},
/* P2_CELL_DIR   - 40 */ { MCP23S17_PORT_B, 5,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, HAL_PIN_TYPE_IO_EXPANDER, 0},
/* CAN_RST       - 41 */ { MCP23S17_PORT_B, 6,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, HAL_PIN_TYPE_IO_EXPANDER, 0},
/* CAN_VDD_EN    - 42 */ { MCP23S17_PORT_B, 7,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, HAL_PIN_TYPE_IO_EXPANDER, 0},

/* SENSOR_CS     - 43 */ { DEMUX_PORT,      2,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, HAL_PIN_TYPE_DEMUX, 0},
/* GNSS_CS       - 44 */ { DEMUX_PORT,      4,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, HAL_PIN_TYPE_DEMUX, 0},
/* IOE_CS        - 45 */ { DEMUX_PORT,      6,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, HAL_PIN_TYPE_DEMUX, 0},
/* CAN_CS        - 46 */ { DEMUX_PORT,      7,  PIN_MODE_NONE, PF_NONE, ADC_CHANNEL_NONE, PWM_INSTANCE_NONE, PWM_CHANNEL_NONE, HAL_PIN_TYPE_DEMUX, 0},

};

} // anonymous

hal_pin_info_t* hal_pin_map(void) {
    return pinmap;
}
