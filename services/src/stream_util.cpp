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

#include "stream_util.h"

#include "timer_hal.h"

#include "stream.h"
#include "check.h"

#include <cctype>

namespace particle {

namespace {

template<typename ReadFn>
int readWhile(InputStream* strm, unsigned timeout, const ReadFn& readFn) {
    size_t bytesRead = 0;
    auto t = HAL_Timer_Get_Milli_Seconds();
    for (;;) {
        bool stop = false;
        for (;;) {
            const size_t n = CHECK(strm->availForRead());
            if (n == 0) {
                break;
            }
            char c = 0;
            CHECK(strm->peek(&c, 1));
            if (!readFn(c)) {
                stop = true;
                break;
            }
            CHECK(strm->skip(1));
            ++bytesRead;
        }
        if (stop || timeout == 0) {
            break;
        }
        const int r = strm->waitEvent(InputStream::READABLE, timeout);
        if (r == SYSTEM_ERROR_TIMEOUT) {
            break;
        }
        if (r < 0) {
            return r;
        }
        const auto t2 = HAL_Timer_Get_Milli_Seconds();
        t = t2 - t;
        if (t < timeout) {
            timeout -= t;
            t = t2;
        } else {
            timeout = 0;
        }
    }
    return bytesRead;
}

} // unnamed

int readLine(InputStream* strm, char* data, size_t size, unsigned timeout) {
    size_t n = 0;
    CHECK(readWhile(strm, timeout, [data, size, &n](char c) {
        if (n == size || std::isspace((unsigned char)c)) {
            return false;
        }
        data[n++] = c;
        return true;
    }));
    if (size > 0) {
        if (n == size) {
            --n;
        }
        data[n] = '\0';
    }
    return n;
}

int skipAll(InputStream* strm, unsigned timeout) {
    return readWhile(strm, timeout, [](char c) {
        return true;
    });
}

int skipNonPrintable(InputStream* strm, unsigned timeout) {
    return readWhile(strm, timeout, [](char c) {
        return !std::isprint((unsigned char)c);
    });
}

int skipWhitespace(InputStream* strm, unsigned timeout) {
    return readWhile(strm, timeout, [](char c) -> bool {
        return std::isspace((unsigned char)c);
    });
}

} // particle
