/*
 * Copyright (c) 2017 Particle Industries, Inc.  All rights reserved.
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

#include "spark_wiring_thread.h"
#include "spark_wiring_interrupts.h"
#include "spark_wiring_error.h"
#include "spark_wiring_global.h"

#include "diagnostics.h"
#include "debug.h"

#include <mutex>
#include <atomic>

namespace particle {

// Base abstract class for a diagnostic data source
class AbstractDiagnosticData {
public:
    static int get(uint16_t id, void* data, size_t& size);
    static int get(const diag_source* src, void* data, size_t& size);

    // This class is non-copyable
    AbstractDiagnosticData(const AbstractDiagnosticData&) = delete;
    AbstractDiagnosticData& operator=(const AbstractDiagnosticData&) = delete;

protected:
    AbstractDiagnosticData(uint16_t id, diag_type type);
    AbstractDiagnosticData(uint16_t id, const char* name, diag_type type);

    virtual int get(void* data, size_t& size) = 0;

private:
    diag_source d_;

    static int callback(const diag_source* src, int cmd, void* data);
};

// Base abstract class for a data source containing an integer value
class AbstractIntegerDiagnosticData: public AbstractDiagnosticData {
public:
    typedef int32_t IntType; // Underlying integer type

    static int get(uint16_t id, IntType& val);
    static int get(const diag_source* src, IntType& val);

protected:
    explicit AbstractIntegerDiagnosticData(uint16_t id, const char* name = nullptr);

    virtual int get(IntType& val) = 0;

private:
    virtual int get(void* data, size_t& size) override; // AbstractDiagnosticData
};

template<typename LockingPolicyT>
class IntegerDiagnosticData:
        public AbstractIntegerDiagnosticData,
        private LockingPolicyT {
public:
    explicit IntegerDiagnosticData(uint16_t id, IntType val = 0);
    IntegerDiagnosticData(uint16_t id, const char* name, IntType val = 0);

    IntType operator++();
    IntType operator++(int);
    IntType operator--();
    IntType operator--(int);
    IntType operator+=(IntType val);
    IntType operator-=(IntType val);

    IntegerDiagnosticData<LockingPolicyT>& operator=(IntType val);
    operator IntType() const;

private:
    IntType val_;

    virtual int get(IntType& val) override; // AbstractIntegerDiagnosticData
};

template<>
class IntegerDiagnosticData<AtomicLockingPolicy>: public AbstractIntegerDiagnosticData {
public:
    explicit IntegerDiagnosticData(uint16_t id, IntType val = 0);
    IntegerDiagnosticData(uint16_t id, const char* name, IntType val = 0);

    IntType operator++();
    IntType operator++(int);
    IntType operator--();
    IntType operator--(int);
    IntType operator+=(IntType val);
    IntType operator-=(IntType val);

    IntegerDiagnosticData<AtomicLockingPolicy>& operator=(IntType val);
    operator IntType() const;

private:
    std::atomic<IntType> val_;

    virtual int get(IntType& val) override; // AbstractIntegerDiagnosticData
};

template<typename EnumT, typename LockingPolicyT>
class EnumDiagnosticData:
        public AbstractIntegerDiagnosticData,
        private LockingPolicyT {
public:
    typedef EnumT EnumType;

    EnumDiagnosticData(uint16_t id, EnumT val);
    EnumDiagnosticData(uint16_t id, const char* name, EnumT val);

    EnumDiagnosticData<EnumT, LockingPolicyT>& operator=(EnumT val);
    operator EnumT() const;

private:
    IntType val_;

    virtual int get(IntType& val) override; // AbstractIntegerDiagnosticData
};

template<typename EnumT>
class EnumDiagnosticData<EnumT, AtomicLockingPolicy>: public AbstractIntegerDiagnosticData {
public:
    typedef EnumT EnumType;

    EnumDiagnosticData(uint16_t id, EnumT val);
    EnumDiagnosticData(uint16_t id, const char* name, EnumT val);

    EnumDiagnosticData<EnumT, AtomicLockingPolicy>& operator=(EnumT val);
    operator EnumT() const;

private:
    std::atomic<IntType> val_;

    virtual int get(IntType& val) override; // AbstractIntegerDiagnosticData
};

// Convenience typedefs
typedef IntegerDiagnosticData<NoLockingPolicy> SimpleIntegerDiagnosticData;
typedef IntegerDiagnosticData<AtomicLockingPolicy> AtomicIntegerDiagnosticData;

template<typename EnumT>
using SimpleEnumDiagnosticData = EnumDiagnosticData<EnumT, NoLockingPolicy>;

template<typename EnumT>
using AtomicEnumDiagnosticData = EnumDiagnosticData<EnumT, AtomicLockingPolicy>;

} // namespace particle

inline particle::AbstractDiagnosticData::AbstractDiagnosticData(uint16_t id, diag_type type) :
        AbstractDiagnosticData(id, nullptr, type) {
}

inline particle::AbstractDiagnosticData::AbstractDiagnosticData(uint16_t id, const char* name, diag_type type) :
        d_{ sizeof(diag_source), 0 /* flags */, id, (uint16_t)type, name, this /* data */, callback } {
    diag_register_source(&d_, nullptr);
}

inline int particle::AbstractDiagnosticData::get(uint16_t id, void* data, size_t& size) {
    const diag_source* src = nullptr;
    const int ret = diag_get_source(id, &src, nullptr);
    if (ret != SYSTEM_ERROR_NONE) {
        return ret;
    }
    return get(src, data, size);
}

inline int particle::AbstractDiagnosticData::get(const diag_source* src, void* data, size_t& size) {
    SPARK_ASSERT(src && src->callback);
    diag_source_get_cmd_data d = { sizeof(diag_source_get_cmd_data), 0 /* reserved */, data, size };
    const int ret = src->callback(src, DIAG_SOURCE_CMD_GET, &d);
    if (ret == SYSTEM_ERROR_NONE) {
        size = d.data_size;
    }
    return ret;
}

inline int particle::AbstractDiagnosticData::callback(const diag_source* src, int cmd, void* data) {
    const auto d = static_cast<AbstractDiagnosticData*>(src->data);
    switch (cmd) {
    case DIAG_SOURCE_CMD_GET: {
        const auto cmdData = static_cast<diag_source_get_cmd_data*>(data);
        return d->get(cmdData->data, cmdData->data_size);
    }
    default:
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }
}

inline particle::AbstractIntegerDiagnosticData::AbstractIntegerDiagnosticData(uint16_t id, const char* name) :
        AbstractDiagnosticData(id, name, DIAG_TYPE_INT) {
}

inline int particle::AbstractIntegerDiagnosticData::get(uint16_t id, IntType& val) {
    const diag_source* src = nullptr;
    const int ret = diag_get_source(id, &src, nullptr);
    if (ret != SYSTEM_ERROR_NONE) {
        return ret;
    }
    return get(src, val);
}

inline int particle::AbstractIntegerDiagnosticData::get(const diag_source* src, IntType& val) {
    SPARK_ASSERT(src->type == DIAG_TYPE_INT);
    size_t size = sizeof(IntType);
    return AbstractDiagnosticData::get(src, &val, size);
}

inline int particle::AbstractIntegerDiagnosticData::get(void* data, size_t& size) {
    if (!data) {
        size = sizeof(IntType);
        return SYSTEM_ERROR_NONE;
    }
    if (size < sizeof(IntType)) {
        return SYSTEM_ERROR_TOO_LARGE;
    }
    const int ret = get(*(IntType*)data);
    if (ret == SYSTEM_ERROR_NONE) {
        size = sizeof(IntType);
    }
    return ret;
}

template<typename LockingPolicyT>
inline particle::IntegerDiagnosticData<LockingPolicyT>::IntegerDiagnosticData(uint16_t id, IntType val) :
        IntegerDiagnosticData(id, nullptr, val) {
}

template<typename LockingPolicyT>
inline particle::IntegerDiagnosticData<LockingPolicyT>::IntegerDiagnosticData(uint16_t id, const char* name, IntType val) :
        AbstractIntegerDiagnosticData(id, name),
        val_(val) {
}

template<typename LockingPolicyT>
inline particle::AbstractIntegerDiagnosticData::IntType particle::IntegerDiagnosticData<LockingPolicyT>::operator++() {
    const auto lock = this->lock();
    const auto val = ++val_;
    this->unlock(lock);
    return val;
}

template<typename LockingPolicyT>
inline particle::AbstractIntegerDiagnosticData::IntType particle::IntegerDiagnosticData<LockingPolicyT>::operator++(int) {
    const auto lock = this->lock();
    const auto v = val_++;
    this->unlock(lock);
    return v;
}

template<typename LockingPolicyT>
inline particle::AbstractIntegerDiagnosticData::IntType particle::IntegerDiagnosticData<LockingPolicyT>::operator--() {
    const auto lock = this->lock();
    const auto v = --val_;
    this->unlock(lock);
    return v;
}

template<typename LockingPolicyT>
inline particle::AbstractIntegerDiagnosticData::IntType particle::IntegerDiagnosticData<LockingPolicyT>::operator--(int) {
    const auto lock = this->lock();
    const auto v = val_--;
    this->unlock(lock);
    return v;
}

template<typename LockingPolicyT>
inline particle::AbstractIntegerDiagnosticData::IntType particle::IntegerDiagnosticData<LockingPolicyT>::operator+=(IntType val) {
    const auto lock = this->lock();
    const auto v = (val_ += val);
    this->unlock(lock);
    return v;
}

template<typename LockingPolicyT>
inline particle::AbstractIntegerDiagnosticData::IntType particle::IntegerDiagnosticData<LockingPolicyT>::operator-=(IntType val) {
    const auto lock = this->lock();
    const auto v = (val_ -= val);
    this->unlock(lock);
    return v;
}

template<typename LockingPolicyT>
inline particle::IntegerDiagnosticData<LockingPolicyT>& particle::IntegerDiagnosticData<LockingPolicyT>::operator=(IntType val) {
    const auto lock = this->lock();
    val_ = val;
    this->unlock(lock);
    return *this;
}

template<typename LockingPolicyT>
inline particle::IntegerDiagnosticData<LockingPolicyT>::operator IntType() const {
    const auto lock = this->lock();
    const auto v = val_;
    this->unlock(lock);
    return v;
}

template<typename LockingPolicyT>
inline int particle::IntegerDiagnosticData<LockingPolicyT>::get(IntType& val) {
    const auto lock = this->lock();
    val = val_;
    this->unlock(lock);
    return SYSTEM_ERROR_NONE;
}

inline particle::IntegerDiagnosticData<particle::AtomicLockingPolicy>::IntegerDiagnosticData(uint16_t id, IntType val) :
        IntegerDiagnosticData(id, nullptr, val) {
}

inline particle::IntegerDiagnosticData<particle::AtomicLockingPolicy>::IntegerDiagnosticData(uint16_t id, const char* name, IntType val) :
        AbstractIntegerDiagnosticData(id, name),
        val_(val) {
}

inline particle::AbstractIntegerDiagnosticData::IntType particle::IntegerDiagnosticData<particle::AtomicLockingPolicy>::operator++() {
    return (val_.fetch_add(1, std::memory_order_relaxed) + 1);
}

inline particle::AbstractIntegerDiagnosticData::IntType particle::IntegerDiagnosticData<particle::AtomicLockingPolicy>::operator++(int) {
    return val_.fetch_add(1, std::memory_order_relaxed);
}

inline particle::AbstractIntegerDiagnosticData::IntType particle::IntegerDiagnosticData<particle::AtomicLockingPolicy>::operator--() {
    return (val_.fetch_sub(1, std::memory_order_relaxed) - 1);
}

inline particle::AbstractIntegerDiagnosticData::IntType particle::IntegerDiagnosticData<particle::AtomicLockingPolicy>::operator--(int) {
    return val_.fetch_sub(1, std::memory_order_relaxed);
}

inline particle::AbstractIntegerDiagnosticData::IntType particle::IntegerDiagnosticData<particle::AtomicLockingPolicy>::operator+=(IntType val) {
    return (val_.fetch_add(val, std::memory_order_relaxed) + val);
}

inline particle::AbstractIntegerDiagnosticData::IntType particle::IntegerDiagnosticData<particle::AtomicLockingPolicy>::operator-=(IntType val) {
    return (val_.fetch_sub(val, std::memory_order_relaxed) - val);
}

inline particle::IntegerDiagnosticData<particle::AtomicLockingPolicy>& particle::IntegerDiagnosticData<particle::AtomicLockingPolicy>::operator=(IntType val) {
    val_.store(val, std::memory_order_relaxed);
    return *this;
}

inline particle::IntegerDiagnosticData<particle::AtomicLockingPolicy>::operator IntType() const {
    return val_.load(std::memory_order_relaxed);
}

inline int particle::IntegerDiagnosticData<particle::AtomicLockingPolicy>::get(IntType& val) {
    val = val_.load(std::memory_order_relaxed);
    return SYSTEM_ERROR_NONE;
}

template<typename EnumT, typename LockingPolicyT>
inline particle::EnumDiagnosticData<EnumT, LockingPolicyT>::EnumDiagnosticData(uint16_t id, EnumT val) :
        EnumDiagnosticData(id, nullptr, val) {
}

template<typename EnumT, typename LockingPolicyT>
inline particle::EnumDiagnosticData<EnumT, LockingPolicyT>::EnumDiagnosticData(uint16_t id, const char* name, EnumT val) :
        AbstractIntegerDiagnosticData(id, name),
        val_((IntType)val) {
}

template<typename EnumT, typename LockingPolicyT>
inline particle::EnumDiagnosticData<EnumT, LockingPolicyT>& particle::EnumDiagnosticData<EnumT, LockingPolicyT>::operator=(EnumT val) {
    const auto lock = this->lock();
    val_ = (IntType)val;
    this->unlock(lock);
    return *this;
}

template<typename EnumT, typename LockingPolicyT>
inline particle::EnumDiagnosticData<EnumT, LockingPolicyT>::operator EnumT() const {
    const auto lock = this->lock();
    const auto v = (EnumT)val_;
    this->unlock(lock);
    return v;
}

template<typename EnumT, typename LockingPolicyT>
inline int particle::EnumDiagnosticData<EnumT, LockingPolicyT>::get(IntType& val) {
    const auto lock = this->lock();
    val = (EnumT)val_;
    this->unlock(lock);
    return SYSTEM_ERROR_NONE;
}

template<typename EnumT>
inline particle::EnumDiagnosticData<EnumT, particle::AtomicLockingPolicy>::EnumDiagnosticData(uint16_t id, EnumT val) :
        EnumDiagnosticData(id, nullptr, val) {
}

template<typename EnumT>
inline particle::EnumDiagnosticData<EnumT, particle::AtomicLockingPolicy>::EnumDiagnosticData(uint16_t id, const char* name, EnumT val) :
        AbstractIntegerDiagnosticData(id, name),
        val_((IntType)val) {
}

template<typename EnumT>
inline particle::EnumDiagnosticData<EnumT, particle::AtomicLockingPolicy>& particle::EnumDiagnosticData<EnumT, particle::AtomicLockingPolicy>::operator=(EnumT val) {
    val_.store((IntType)val, std::memory_order_relaxed);
    return *this;
}

template<typename EnumT>
inline particle::EnumDiagnosticData<EnumT, particle::AtomicLockingPolicy>::operator EnumT() const {
    return (EnumT)val_.load(std::memory_order_relaxed);
}

template<typename EnumT>
inline int particle::EnumDiagnosticData<EnumT, particle::AtomicLockingPolicy>::get(IntType& val) {
    val = val_.load(std::memory_order_relaxed);
    return SYSTEM_ERROR_NONE;
}
