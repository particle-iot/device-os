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

#pragma once

#include <memory>
#include <mutex>
#include "service_debug.h"

namespace particle {

template <typename MutexT>
class UniqueLock {
public:
    UniqueLock()
            : mutex_(nullptr),
              locked_(false) {
    }

    explicit UniqueLock(MutexT& m)
            : mutex_(std::addressof(m)),
              locked_(false) {
        lock();
    }

    UniqueLock(MutexT& m, std::defer_lock_t)
            : mutex_(std::addressof(m)),
              locked_(false) {
    }

    UniqueLock(MutexT& m, std::try_to_lock_t)
            : mutex_(std::addressof(m)),
              locked_(mutex_->try_lock()) {
    }

    UniqueLock(MutexT& m, std::adopt_lock_t)
            : mutex_(std::addressof(m)),
              locked_(true) {
    }

    ~UniqueLock() {
        if (locked_) {
            unlock();
        }
    }

    UniqueLock(const UniqueLock&) = delete;
    UniqueLock& operator=(const UniqueLock&) = delete;

    UniqueLock(UniqueLock&& other)
            : mutex_(other.mutex_),
              locked_(other.locked_) {
        other.mutex_ = nullptr;
        other.locked_ = false;
    }

    UniqueLock& operator=(UniqueLock&& other) noexcept
    {
        UniqueLock(std::move(other)).swap(*this);
        return *this;
    }

    void lock() {
        SPARK_ASSERT(mutex_ && !locked_);
        mutex_->lock();
        locked_ = true;
    }

    void unlock() {
        SPARK_ASSERT(mutex_ && locked_);
        mutex_->unlock();
        locked_ = false;
    }

    bool try_lock() {
        SPARK_ASSERT(mutex_ && !locked_);
        locked_ = mutex_->try_lock();
        return locked_;
    }

    void swap(UniqueLock& other)
    {
        std::swap(mutex_, other.mutex_);
        std::swap(locked_, other.locked_);
    }

    MutexT* release() {
        MutexT* m = mutex_;
        mutex_ = nullptr;
        locked_ = false;
        return m;
    }

    bool owns_lock() const {
        return locked_;
    }

    explicit operator bool() const {
        return owns_lock();
    }

    MutexT* mutex() const {
        return mutex_;
    }

private:
    MutexT* mutex_;
    bool locked_;
};

} // namespace particle
