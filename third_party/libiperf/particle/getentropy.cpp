/*
 * Copyright (c) 2026 Particle Industries, Inc.  All rights reserved.
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

#include <cstddef>
#include <cerrno>

#include "random.h"

extern "C" int getentropy(void* buffer, size_t length) {
    // POSIX limits a single request to 256 bytes
    if (!buffer || length > 256) {
        errno = EIO;
        return -1;
    }
    particle::Random rand;
    rand.gen((char*)buffer, length);
    return 0;
}
