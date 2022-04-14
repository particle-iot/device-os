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

#include <cstdlib>

namespace particle {

// Abstract allocator
class SimpleAllocator {
public:
    virtual ~SimpleAllocator() = default;

    virtual void* alloc(size_t size) = 0;
    virtual void free(void* ptr) = 0;
};

class Allocator: public SimpleAllocator {
public:
    virtual void* realloc(void* ptr, size_t size) = 0;
};

// Allocator interface for malloc()
class HeapAllocator: public Allocator {
public:
    virtual void* alloc(size_t size) override {
        return ::malloc(size);
    }

    virtual void* realloc(void* ptr, size_t size) override {
        return ::realloc(ptr, size);
    }

    virtual void free(void* ptr) override {
        ::free(ptr);
    }

    static HeapAllocator* instance() {
        static HeapAllocator alloc;
        return &alloc;
    }
};

} // particle
