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

#ifndef KG200Z_H
#define KG200Z_H

#include "Particle.h"
#include "mcp23s17.h"

namespace particle {

class Kg200z {
public:
    int begin();
    int end();
    int getVersion();
    int uartDownloadMode(bool enable, bool reset = false);

    static Kg200z& getInstance();

private:
    Kg200z(TwoWire* wire);
    ~Kg200z();

    void sendAtComand(const char *command);
    void receiveAtResponse();

    bool initialized_;
    uint8_t address_;
    TwoWire* wire_;

    std::pair<uint8_t, uint8_t> intPin_     = {MCP23S17_PORT_A, 7};
    std::pair<uint8_t, uint8_t> rstPin_     = {MCP23S17_PORT_B, 2};
    std::pair<uint8_t, uint8_t> busSelPin_  = {MCP23S17_PORT_B, 1};
    std::pair<uint8_t, uint8_t> bootPin_    = {MCP23S17_PORT_B, 0};

    static constexpr uint8_t KG200Z_SLAVE_ADDR = 0x61;

    static constexpr uint8_t CMD_AT            = 0x01;
    static constexpr uint8_t CMD_AT_RSP_LEN    = 0x02;
    static constexpr uint8_t CMD_AT_RSP_DATA   = 0x03;
}; // class Kg200z

} // namespace particle

#endif // KG200Z_H