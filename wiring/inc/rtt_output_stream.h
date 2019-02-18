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

#pragma once

#include "spark_wiring_print.h"
#include "spark_wiring_platform.h"

#if Wiring_Rtt

namespace particle {

// Class implementing a SEGGER RTT (Real Time Transfer) output stream
class RttOutputStream: public Print {
public:
    RttOutputStream();
    virtual ~RttOutputStream();

    int open();
    void close();

    bool isOpen();

    // Reimplemented from `Print`
    virtual size_t write(uint8_t c) override;
    virtual size_t write(const uint8_t* data, size_t size) override;

private:
    bool open_, connected_;
};

inline bool RttOutputStream::isOpen() {
    return open_;
}

inline size_t RttOutputStream::write(uint8_t c) {
    return write(&c, 1);
}

} // particle

#endif // Wiring_Rtt
