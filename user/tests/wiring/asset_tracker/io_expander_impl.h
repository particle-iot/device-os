/*
 * Copyright (c) 2019 Particle Industries, Inc.  All rights reserved.
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

#ifndef IO_EXPANDER_IMPL_H
#define IO_EXPANDER_IMPL_H

#include "Particle.h"


#define IO_EXPANDER_PORT_COUNT_MAX              2
#define IO_EXPANDER_PIN_COUNT_PER_PORT_MAX      8

// Ports on PCAL6416A
enum class IoExpanderPort : uint8_t {
    // It must start from 0, as it is also the port index.
    PORT0 = 0,
    PORT1 = 1,
    INVALID = 0x7F
};

// Pins of each port on PCAL6416A
enum class IoExpanderPin : uint8_t {
    // It must start from 0, as it is also the bit index in registers.
    PIN0 = 0,
    PIN1 = 1,
    PIN2 = 2,
    PIN3 = 3,
    PIN4 = 4,
    PIN5 = 5,
    PIN6 = 6,
    PIN7 = 7,
    INVALID = 0x7F
};

// Output drive stength on PCAL6416A
enum class IoExpanderPinDrive : uint8_t {
    PERCENT25 = 0x00,
    PERCENT50 = 0x01,
    PERCENT75 = 0x02,
    PERCENT100 = 0x03
};

#endif // IO_EXPANDER_IMPL_H
