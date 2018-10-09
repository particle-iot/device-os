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

#ifndef SERVICES_RINGBUFFER_H
#define SERVICES_RINGBUFFER_H

#include <cstddef>
#include "system_error.h"
#include "check.h"

namespace particle {
namespace services {

inline size_t wrap(size_t v, size_t size) {
    return v >= size ? v - size : v;
}

template <typename T>
class RingBuffer {
public:
    RingBuffer() = default;
    RingBuffer(T* buffer, size_t size);

    void init(T* buffer, size_t size);
    void reset();

    size_t size() const;

    bool full() const;
    bool empty() const;

    ssize_t space() const;
    ssize_t data() const;

    ssize_t put(const T& v);
    ssize_t put(const T* v, size_t size);

    ssize_t get(T* v);
    ssize_t get(T* v, size_t size);

    ssize_t peek(T* v);
    ssize_t peek(T* v, size_t size);

    size_t acquirable() const;
    size_t acquirableWrapped() const;
    size_t consumable() const;

    size_t acquirePending() const;
    size_t consumePending() const;

    void acquireBegin();
    T* acquire(size_t size);
    ssize_t acquireCommit(size_t size, size_t cancel = 0);

    T* consume(size_t size);
    ssize_t consumeCommit(size_t size, size_t cancel = 0);

private:
    size_t curData() const;
    size_t curSpace() const;
    void updateCurSize();

public:

    T* buffer_ = nullptr;
    volatile size_t head_;
    volatile size_t tail_;

    volatile size_t headPending_;
    volatile size_t tailPending_;

    size_t size_ = 0;
    volatile size_t curSize_;
    volatile bool full_;
};

template <typename T>
inline RingBuffer<T>::RingBuffer(T* buffer, size_t size)
        : buffer_(buffer),
          head_(0),
          tail_(0),
          headPending_(0),
          tailPending_(0),
          size_(size),
          curSize_(size),
          full_(false) {
}

template <typename T>
inline void RingBuffer<T>::init(T* buffer, size_t size) {
    buffer_ = buffer;
    size_ = size;
    reset();
}

template <typename T>
inline void RingBuffer<T>::reset() {
    curSize_ = size_;

    head_ = tail_ = headPending_ = tailPending_ = 0;
    full_ = false;
}

template <typename T>
inline size_t RingBuffer<T>::size() const {
    return size_;
}

template <typename T>
inline bool RingBuffer<T>::full() const {
    return head_ == tail_ && full_;
}

template <typename T>
inline bool RingBuffer<T>::empty() const {
    return head_ == tail_ && !full_;
}

template <typename T>
inline ssize_t RingBuffer<T>::space() const {
    CHECK_TRUE(headPending_ == 0, SYSTEM_ERROR_INVALID_STATE);
    return curSpace();
}

template <typename T>
inline ssize_t RingBuffer<T>::data() const {
    CHECK_TRUE(tailPending_ == 0, SYSTEM_ERROR_INVALID_STATE);
    return curData();
}

template <typename T>
inline ssize_t RingBuffer<T>::put(const T& v) {
    return put(&v, 1);
}

template <typename T>
inline ssize_t RingBuffer<T>::put(const T* v, size_t size) {
    if (size == 0) {
        return 0;
    }
    CHECK_TRUE(v && size, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(space() >= (ssize_t)size, SYSTEM_ERROR_TOO_LARGE);

    size_t head = head_;

    if (v != nullptr) {
        for (size_t i = 0; i < size; i++) {
            buffer_[head] = v[i];
            head = wrap(head + 1, curSize_);
        }
    } else {
        head = wrap(head + size, curSize_);
    }

    head_ = head;
    full_ = (head_ == tail_);

    return size;
}

template <typename T>
inline ssize_t RingBuffer<T>::get(T* v) {
    return get(v, 1);
}

template <typename T>
inline ssize_t RingBuffer<T>::get(T* v, size_t size) {
    if (size == 0) {
        return 0;
    }
    CHECK_TRUE(data() >= (ssize_t)size, SYSTEM_ERROR_TOO_LARGE);

    size_t tail = tail_;

    if (v != nullptr) {
        for (size_t i = 0; i < size; i++) {
            v[i] = buffer_[tail];
            tail = wrap((tail + 1), curSize_);
        }
    } else {
        tail = wrap(tail + size, curSize_);
    }

    tail_ = tail;
    full_ = false;

    return size;
}

template <typename T>
inline ssize_t RingBuffer<T>::peek(T* v) {
    return peek(v, 1);
}

template <typename T>
inline ssize_t RingBuffer<T>::peek(T* v, size_t size) {
    if (size == 0) {
        return 0;
    }
    CHECK_TRUE(data() >= (ssize_t)size, SYSTEM_ERROR_TOO_LARGE);
    CHECK_TRUE(v, SYSTEM_ERROR_INVALID_ARGUMENT);

    for (size_t i = 0; i < size; i++) {
        v[i] = buffer_[wrap((tail_ + i), curSize_)];
    }

    return size;
}

template <typename T>
inline void RingBuffer<T>::acquireBegin() {
    updateCurSize();
}

template <typename T>
inline size_t RingBuffer<T>::acquirable() const {
    // Calculate provisional head and full
    size_t head = wrap(head_ + headPending_, curSize_);
    bool full = this->full() || ((head == tail_) && (headPending_ != 0));
    if (head >= tail_ && !full) {
        return (curSize_ - head);
    } else {
        return (tail_ - head);
    }
}

template <typename T>
inline size_t RingBuffer<T>::acquirableWrapped() const {
    // Calculate provisional head and full
    size_t head = wrap(head_ + headPending_, curSize_);
    bool full = this->full() || ((head == tail_) && (headPending_ != 0));
    if (head >= tail_ && !full) {
        return tail_;
    } else {
        return 0;
    }
}

template <typename T>
inline size_t RingBuffer<T>::consumable() const {
    // Calculate provisional tail
    size_t tail = wrap(tail_ + tailPending_, curSize_);
    if (head_ >= tail && !full_) {
        return head_ - tail;
    } else {
        return curSize_ - tail;
    }
}

template <typename T>
inline size_t RingBuffer<T>::acquirePending() const {
    return headPending_;
}

template <typename T>
inline size_t RingBuffer<T>::consumePending() const {
    return tailPending_;
}

template <typename T>
inline T* RingBuffer<T>::acquire(size_t size) {
    if (acquirable() >= size) {
        // Calculate provisional head
        size_t head = wrap(head_ + headPending_, curSize_);
        headPending_ += size;
        return buffer_ + head;
    } else if (acquirableWrapped() >= size) {
        // Calculate provisional head
        size_t head = wrap(head_ + headPending_, curSize_);
        curSize_ = head;
        head_ = 0;
        tail_ = wrap(tail_, curSize_);
        headPending_ += size;
        return buffer_;
    }

    return nullptr;
}

template <typename T>
inline ssize_t RingBuffer<T>::acquireCommit(size_t size, size_t cancel) {
#ifdef DEBUG_BUILD
    CHECK_TRUE(headPending_ >= (size + cancel), SYSTEM_ERROR_TOO_LARGE);
    if (cancel != 0) {
        CHECK_TRUE((ssize_t)headPending_ - (size + cancel) == 0, SYSTEM_ERROR_INVALID_STATE);
    }
#endif // DEBUG_BUILD

    headPending_ -= (size + cancel);
    head_ = wrap(head_ + size, curSize_);
    full_ = ((head_ == tail_) && size > 0);

    return (size);
}

template <typename T>
inline T* RingBuffer<T>::consume(size_t size) {
#ifdef DEBUG_BUILD
    if (consumable() >= size) {
#else
    {
#endif // DEBUG_BUILD
        // Calculate provisional tail
        size_t tail = wrap(tail_ + tailPending_, curSize_);
        tailPending_ += size;
        return buffer_ + tail;
    }

    return nullptr;
}

template <typename T>
inline ssize_t RingBuffer<T>::consumeCommit(size_t size, size_t cancel) {
#ifdef DEBUG_BUILD
    CHECK_TRUE(tailPending_ >= (size + cancel), SYSTEM_ERROR_TOO_LARGE);
    if (cancel != 0) {
        CHECK_TRUE((ssize_t)tailPending_ - (size + cancel) == 0, SYSTEM_ERROR_INVALID_STATE);
    }
#endif // DEBUG_BUILD

    tailPending_ -= (size + cancel);
    tail_ = wrap(tail_ + size, curSize_);
    if (size > 0) {
        full_ = false;
    }
    return (size);
}

template <typename T>
inline size_t RingBuffer<T>::curSpace() const {
    return curSize_ - curData();
}

template <typename T>
inline size_t RingBuffer<T>::curData() const {
    if (head_ >= tail_ && !full_) {
        return head_ - tail_;
    } else {
        return curSize_ + head_ - tail_;
    }
}

template <typename T>
inline void RingBuffer<T>::updateCurSize() {
    if (curSize_ != size_ && head_ >= tail_ && !full_) {
        curSize_ = size_;
    }
}

} // services
} // particle

#endif // SERVICES_RINGBUFFER_H
