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

#include "allocator.h"

#include <type_traits>

namespace particle {

namespace detail {

constexpr size_t alignedSize(size_t size) {
    return ((size + sizeof(uintptr_t) - 1) / sizeof(uintptr_t)) * sizeof(uintptr_t);
}

} // particle::detail

// Mixin class implementing a linked buffer
template<typename... BaseT>
struct LinkedBuffer: BaseT... {
    LinkedBuffer<BaseT...>* next;
};

template<typename BufferT>
inline BufferT* allocLinkedBuffer(size_t size, SimpleAllocator* alloc) {
    const auto buf = (BufferT*)alloc->alloc(size + detail::alignedSize(sizeof(BufferT)));
    if (buf) {
        new(buf) BufferT();
    }
    return buf;
}

template<typename BufferT, typename EnableT = typename std::enable_if<std::is_trivially_copyable<BufferT>::value>::type>
inline BufferT* reallocLinkedBuffer(BufferT* buf, size_t size, Allocator* alloc) {
    const auto b = (BufferT*)alloc->realloc(buf, size + detail::alignedSize(sizeof(BufferT)));
    if (!buf && b) {
        new(b) BufferT();
    }
    return b;
}

template<typename BufferT>
inline void freeLinkedBuffer(BufferT* buf, SimpleAllocator* alloc) {
    if (buf) {
        buf->~BufferT();
        alloc->free(buf);
    }
}

template<typename BufferT>
inline BufferT* allocLinkedBuffer(size_t size) {
    return allocLinkedBuffer<BufferT>(size, HeapAllocator::instance());
}

template<typename BufferT>
inline BufferT* reallocLinkedBuffer(BufferT* buf, size_t size) {
    return reallocLinkedBuffer(buf, size, HeapAllocator::instance());
}

template<typename BufferT>
inline void freeLinkedBuffer(BufferT* buf) {
    freeLinkedBuffer(buf, HeapAllocator::instance());
}

template<typename BufferT>
inline char* linkedBufferData(BufferT* buf) {
    return (char*)buf + detail::alignedSize(sizeof(BufferT));
}

template<typename BufferT>
inline const char* linkedBufferData(const BufferT* buf) {
    return (const char*)buf + detail::alignedSize(sizeof(BufferT));
}

} // particle
