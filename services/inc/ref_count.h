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

#include <utility>
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

    void addRef() const {
        count_.fetch_add(1, std::memory_order_relaxed);
    }

    void release() const {
        if (count_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            delete this;
        }
    }

private:
    mutable std::atomic_int count_;
};

/**
 * Smart pointer for reference counted objects.
 */
template<typename T>
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
            RefCountPtr() {
        swap(*this, ptr);
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

    RefCountPtr& operator=(RefCountPtr ptr) {
        swap(*this, ptr);
        return *this;
    }

    explicit operator bool() const {
        return p_;
    }

    static RefCountPtr<T> wrap(T* ptr) {
        return RefCountPtr(ptr, false /* addRef */);
    }

private:
    T* p_;

    RefCountPtr(T* p, bool addRef) :
            p_(p) {
        if (addRef && p_) {
            p_->addRef();
        }
    }

    friend void swap(RefCountPtr& ptr1, RefCountPtr& ptr2) {
        auto p = ptr1.p_;
        ptr1.p_ = ptr2.p_;
        ptr2.p_ = p;
    }
};

template<typename T, typename... ArgsT>
inline RefCountPtr<T> makeRefCountPtr(ArgsT&&... args) {
    return RefCountPtr<T>::wrap(new T(std::forward<ArgsT>(args)...));
}

} // namespace particle
