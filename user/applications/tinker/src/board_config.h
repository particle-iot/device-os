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

#pragma once

#include "Particle.h"

namespace particle {

class BoardConfig {
public:
    BoardConfig();
    ~BoardConfig();

    static BoardConfig* instance();

    bool process(JSONValue config);
    char* reply();
    size_t replySize();
    
private:
    JSONBufferWriter replyWriter_;
    char replyBuffer_[64];

#if HAL_PLATFORM_POWER_MANAGEMENT
    static constexpr uint8_t auxPwrCtrlPin_ = D7;
    static constexpr uint8_t pmicIntPin_ = A7;
#endif
#if HAL_PLATFORM_ETHERNET
    static constexpr uint8_t ethernetCsPin_ = A3;
    static constexpr uint8_t ethernetIntPin_ = A4;
    static constexpr uint8_t ethernetResetPin_ = PIN_INVALID;
#endif

    void configureBaseBoard();
    bool detectI2cSlave(uint8_t addr);
    void configForMuon();
    void configForGeneric();
};

}
