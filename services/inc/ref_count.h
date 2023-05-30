/*
 * Copyright (c) 2023 Particle Industries, Inc.  All rights reserved.
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

#include <type_traits>
#include <atomic>

static_assert(std::atomic_int::is_always_lock_free, "std::atomic<int> is not always lock-free");

namespace particle {

/**
 * Base class for reference counted objects.
 */
template<typename T>
class RefCount {
public:
    RefCount() :
            count_(1) {
    }

    virtual ~RefCount() = default;

    void addRef() {
        count_.fetch_add(1, std::memory_order_relaxed);
    }

    void release() {
        if (count_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            delete this;
        }
    }

private:
    std::atomic_int count_;
};

/**
 * Smart pointer for reference counted objects.
 */
template<typename T, typename EnableT = std::enable_if_t<std::is_base_of<RefCount<T>, T>::value>>
class RefCountPtr {
public:
    RefCountPtr() :
            RefCountPtr(nullptr) {
    }

    RefCountPtr(T* ptr) :
            RefCountPtr(ptr, true /* addRef */) {
    }

    RefCountPtr(const RefCountPtr& ptr) :
            p_(ptr.p_) {
        if (p_) {
            p_->addRef();
        }
    }

    RefCountPtr(RefCountPtr&& ptr) :
            p_(ptr.p_) {
        ptr.p_ = nullptr;
    }

    ~RefCountPtr() {
        if (p_) {
            p_->release();
        }
    }

    T* get() const {
        return p_;
    }

    T* unwrap() {
        auto p = p_;
        p_ = nullptr;
        return p;
    }

    T* operator->() const {
        return p_;
    }

    T& operator*() const {
        return *p_;
    }

    explicit operator bool() const {
        return p_;
    }

    RefCountPtr& operator=(const RefCountPtr& ptr) {
        if (ptr != *this) {
            if (p_) {
                p_->release();
            }
            p_ = ptr.p_;
            if (p_) {
                p_->addRef();
            }
        }
        return *this;
    }

    RefCountPtr& operator=(RefCountPtr&& ptr) {
        if (ptr != *this) {
            if (p_) {
                p_->release();
            }
            p_ = ptr.p_;
            ptr.p_ = nullptr;
        }
        return *this;
    }

    static RefCountPtr<T> wrap(T* ptr) {
        return RefCountPtr(ptr, false /* addRef */);
    }

    friend void swap(RefCountPtr& ptr1, RefCountPtr& ptr2) {
        auto p = ptr1.p_;
        ptr1.p_ = ptr2.p_;
        ptr2.p_ = p;
    }

private:
    T* p_;

    RefCountPtr(T* p, bool addRef) :
            p_(p) {
        if (addRef && p_) {
            p_->addRef();
        }
    }
};

} // namespace particle
