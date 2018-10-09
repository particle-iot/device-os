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

#include <cstddef>

namespace particle {

// Base abstract class for an IO stream
class BaseStream {
public:
    enum EventFlag {
        READABLE = 0x01,
        WRITABLE = 0x02
    };

    virtual ~BaseStream() = default;

    virtual int waitEvent(unsigned flags, unsigned timeout = 0) = 0;
};

// Base abstract class for an input stream
class InputStream: virtual public BaseStream {
public:
    virtual int read(char* data, size_t size) = 0;
    virtual int peek(char* data, size_t size) = 0;
    virtual int skip(size_t size) = 0;
    virtual int availForRead() = 0;

    int readAll(char* data, size_t size, unsigned timeout = 0);
    int skipAll(size_t size, unsigned timeout = 0);
};

// Base abstract class for an output stream
class OutputStream: virtual public BaseStream {
public:
    virtual int write(const char* data, size_t size) = 0;
    virtual int flush() = 0;
    virtual int availForWrite() = 0;

    int writeAll(const char* data, size_t size, unsigned timeout = 0);
};

// Base abstract class for a bidirectional stream
class Stream:
        public InputStream,
        public OutputStream {
};

} // particle
