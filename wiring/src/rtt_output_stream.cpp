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

#include "rtt_output_stream.h"

#if Wiring_Rtt

#include "concurrent_hal.h"
#include "delay_hal.h"

#include "SEGGER_RTT.h"

namespace particle {

namespace {

const unsigned WRITE_RETRY_COUNT = 3;
const unsigned WRITE_RETRY_DELAY = 1;

class RttInitializer {
public:
    RttInitializer() {
        SEGGER_RTT_Init();
    }
};

} // particle::

RttOutputStream::RttOutputStream() :
        open_(false),
        connected_(false) {
}

RttOutputStream::~RttOutputStream() {
    close();
}

int RttOutputStream::open() {
    static RttInitializer rttInit;
    open_ = true;
    return 0;
}

void RttOutputStream::close() {
    connected_ = false;
    open_ = false;
}

size_t RttOutputStream::write(const uint8_t* data, size_t size) {
    size_t written = 0;
    if (open_) {
        unsigned retries = 0;
        while (written < size) {
            const unsigned n = SEGGER_RTT_WriteNoLock(0 /* BufferIndex */, data + written, size - written);
            if (n > 0) {
                written += n;
                connected_ = true;
            } else if (connected_) {
                if (retries++ >= WRITE_RETRY_COUNT) {
                    connected_ = false;
                    break;
                }
                if (WRITE_RETRY_DELAY > 0) {
                    HAL_Delay_Milliseconds(WRITE_RETRY_DELAY);
                } else {
                    os_thread_yield();
                }
            } else {
                break;
            }
        }
    }
    return written;
}

} // particle

#endif // Wiring_Rtt
