/*
 * Copyright (c) 2024 Particle Industries, Inc.  All rights reserved.
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

#ifndef TMP112A_H
#define TMP112A_H

#include "Particle.h"
#include "mcp23s17.h"

namespace particle {

class Tmp112a {
public:
    int begin();
    int end();
    int getTemperature(float* temp);

    static Tmp112a& getInstance();

private:
    Tmp112a(TwoWire* wire);
    ~Tmp112a();

    bool initialized_;
    uint8_t address_;
    TwoWire* wire_;

    std::pair<uint8_t, uint8_t> intPin_  = {MCP23S17_PORT_A, 2};

    static constexpr uint8_t TMP112A_SLAVE_ADDR = 0x48;

    static constexpr uint8_t CONFIG_REG     = 0x01;
    static constexpr uint8_t TEMP_REG       = 0x00;
    static constexpr uint16_t DEFAULT_CONFIG = 0x60A0;
}; // class Tmp112a

} // namespace particle

#endif // TMP112A_H