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

#include <algorithm>
#include <cstring>
#include <cstdlib>

#include "rng_hal.h"

namespace particle {

class Random {
public:
    Random() :
            Random(HAL_RNG_GetRandomNumber()) {
    }

    explicit Random(unsigned seed) :
            seed_(seed) {
    }

    template<typename T>
    T gen() {
        T val = 0;
        gen((char*)&val, sizeof(val));
        return val;
    }

    void gen(char* data, size_t size) {
        while (size > 0) {
            const int v = rand_r(&seed_);
            const size_t n = std::min(size, sizeof(v));
            memcpy(data, &v, n);
            data += n;
            size -= n;
        }
    }

    void genAlpha(char* data, size_t dataSize, const char* alpha, size_t alphaSize) {
        for (size_t i = 0; i < dataSize; ++i) {
            const auto ai = gen<size_t>() % alphaSize;
            data[i] = alpha[ai];
        }
    }

    void genBase32(char* data, size_t size) {
        static const char alpha[32] = { 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
                'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '2', '3', '4', '5', '6', '7' }; // RFC 4648
        genAlpha(data, size, alpha, sizeof(alpha));
    }

    static void genSecure(char* data, size_t size) {
        while (size > 0) {
            const uint32_t v = HAL_RNG_GetRandomNumber();
            const size_t n = std::min(size, sizeof(v));
            memcpy(data, &v, n);
            data += n;
            size -= n;
        }
    }

private:
    unsigned seed_;
};

} // particle
