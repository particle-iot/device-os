/*
 * Copyright (c) 2020 Particle Industries, Inc.  All rights reserved.
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

#ifndef MCP25625_H
#define MCP25625_H

#include "Particle.h"
#include "pcal6416a.h"
#include "can_transceiver.h"


namespace particle {

enum class Mcp25625Instruction : uint8_t {
    RESET_REG = 0xC0,
    READ = 0x03,
    WRITE = 0x02,
    READ_STATUS = 0xA0,
    RX_STATUS = 0xB0,
    BIT_MODIFY = 0x05,
};

class Mcp25625 : public CanTransceiverBase {
public:
    int begin();
    int end();
    int sleep();
    int wakeup();

    int reset();
    int getCanCtrl(uint8_t* const value);

    static Mcp25625& getInstance();

private:
    Mcp25625();
    ~Mcp25625();

    int writeRegister(const uint8_t reg, const uint8_t val);
    int readRegister(const uint8_t reg, uint8_t* const val);

    bool initialized_;
    IoExpanderPinObj csPin_;
    IoExpanderPinObj resetPin_;
    IoExpanderPinObj pwrEnPin_;
    static RecursiveMutex mutex_;
}; // class Mcp25625

#define MCP25625 Mcp25625::getInstance()

} // namespace particle

#endif // MCP25625_H