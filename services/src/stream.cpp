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

#include "stream.h"

#include "timer_hal.h"

#include "system_error.h"
#include "check.h"

namespace particle {

namespace {

class Timer {
public:
    explicit Timer(unsigned timeout) :
            t1_(timeout ? HAL_Timer_Get_Milli_Seconds() : 0),
            timeout_(timeout) {
    }

    int update() {
        const auto t2 = HAL_Timer_Get_Milli_Seconds();
        const auto dt = t2 - t1_;
        if (dt >= timeout_) {
            timeout_ = 0;
            return SYSTEM_ERROR_TIMEOUT;
        }
        timeout_ -= dt;
        t1_ = t2;
        return 0;
    }

    unsigned timeout() const {
        return timeout_;
    }

    explicit operator bool() const {
        return timeout_;
    }

private:
    system_tick_t t1_;
    unsigned timeout_;
};

} // unnamed

int InputStream::readAll(char* data, size_t size, unsigned timeout) {
    const auto end = data + size;
    Timer t(timeout);
    for (;;) {
        data += CHECK(read(data, end - data));
        if (data == end) {
            break;
        }
        CHECK(waitEvent(EventFlag::READABLE, t.timeout()));
        if (t) {
            CHECK(t.update());
        }
    }
    return size;
}

int InputStream::skipAll(size_t size, unsigned timeout) {
    size_t n = size;
    Timer t(timeout);
    for (;;) {
        n -= CHECK(skip(n));
        if (n == 0) {
            break;
        }
        CHECK(waitEvent(EventFlag::READABLE, t.timeout()));
        if (t) {
            CHECK(t.update());
        }
    }
    return size;
}

int OutputStream::writeAll(const char* data, size_t size, unsigned timeout) {
    const auto end = data + size;
    Timer t(timeout);
    for (;;) {
        data += CHECK(write(data, end - data));
        if (data == end) {
            break;
        }
        CHECK(waitEvent(EventFlag::WRITABLE, t.timeout()));
        if (t) {
            CHECK(t.update());
        }
    }
    return size;
}

} // particle
