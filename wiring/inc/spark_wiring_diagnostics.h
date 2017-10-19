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
#include "spark_wiring_global.h"

#include "diagnostics.h"
#include "system_error.h"
#include "util.h"
#include "debug.h"

#include <functional>
#include <atomic>
#include <type_traits>

namespace particle {

typedef decltype(diag_source::id) DiagnosticDataId;

// Base abstract class for a diagnostic data source
class AbstractDiagnosticData {
public:
    static int get(DiagnosticDataId id, void* data, size_t& size);
    static int get(const diag_source* src, void* data, size_t& size);

    // This class is non-copyable
    AbstractDiagnosticData(const AbstractDiagnosticData&) = delete;
    AbstractDiagnosticData& operator=(const AbstractDiagnosticData&) = delete;

protected:
    AbstractDiagnosticData(DiagnosticDataId id, diag_type type);
    AbstractDiagnosticData(DiagnosticDataId id, const char* name, diag_type type);

    virtual int get(void* data, size_t& size) = 0;

private:
    diag_source d_;

    static int callback(const diag_source* src, int cmd, void* data);
};

// Base abstract class for a data source containing an integer value
class AbstractIntegerDiagnosticData: public AbstractDiagnosticData {
public:
    typedef int32_t IntType; // Underlying integer type

    static int get(DiagnosticDataId id, IntType& val);
    static int get(const diag_source* src, IntType& val);

protected:
    explicit AbstractIntegerDiagnosticData(DiagnosticDataId id, const char* name = nullptr);

    virtual int get(IntType& val) = 0;

private:
    virtual int get(void* data, size_t& size) override; // AbstractDiagnosticData
};

// Non-persistent storage policy
template<typename ValueT, typename KeyT>
class NonPersistentStorage {
public:
    explicit NonPersistentStorage(ValueT val, KeyT) :
            val_(std::move(val)) {
    }

    ValueT& value() {
        return val_;
    }

    const ValueT& value() const {
        return val_;
    }

    void save() {
    }

private:
    ValueT val_;
};

#if Wiring_RetainedMemory

// Storage policy using a retained memory section
template<typename KeyT, KeyT key>
struct RetainedStorage {
    template<typename ValueT, typename>
    class Type {
    public:
        explicit Type(ValueT val, KeyT) {
            const size_t check = checksum();
            if (s_check != check) {
                s_val = std::move(val);
                s_check = check;
            }
        }

        ValueT& value() {
            return s_val;
        }

        const ValueT& value() const {
            return s_val;
        }

        void save() {
            s_check = checksum();
        }

    private:
        static PARTICLE_RETAINED size_t s_check;
        static PARTICLE_RETAINED ValueT s_val;

        static size_t checksum() {
            size_t h = 0;
            combineHash(h, key);
            combineHash(h, s_val);
            return h;
        }
    };
};

#endif // Wiring_RetainedMemory

template<template<typename ValueT, typename KeyT> class StorageT = NonPersistentStorage,
        typename ConcurrencyT = NoConcurrency>
class IntegerDiagnosticData:
        public AbstractIntegerDiagnosticData,
        private StorageT<AbstractIntegerDiagnosticData::IntType, DiagnosticDataId>,
        private ConcurrencyT {
public:
    typedef StorageT<IntType, DiagnosticDataId> StorageType;
    typedef ConcurrencyT ConcurrencyType;

    explicit IntegerDiagnosticData(DiagnosticDataId id, IntType val = 0) :
            IntegerDiagnosticData(id, nullptr, val) {
    }

    IntegerDiagnosticData(DiagnosticDataId id, const char* name, IntType val = 0) :
            AbstractIntegerDiagnosticData(id, name),
            StorageType(val, id) {
    }

    IntType operator++() {
        const auto lock = this->lock();
        const IntType v = ++this->value();
        this->save();
        this->unlock(lock);
        return v;
    }

    IntType operator++(int) {
        const auto lock = this->lock();
        const IntType v = this->value()++;
        this->save();
        this->unlock(lock);
        return v;
    }

    IntType operator--() {
        const auto lock = this->lock();
        const IntType v = --this->value();
        this->save();
        this->unlock(lock);
        return v;
    }

    IntType operator--(int) {
        const auto lock = this->lock();
        const IntType v = this->value()--;
        this->save();
        this->unlock(lock);
        return v;
    }

    IntType operator+=(IntType val) {
        const auto lock = this->lock();
        const IntType v = (this->value() += val);
        this->save();
        this->unlock(lock);
        return v;
    }

    IntType operator-=(IntType val) {
        const auto lock = this->lock();
        const IntType v = (this->value() -= val);
        this->save();
        this->unlock(lock);
        return v;
    }

    IntegerDiagnosticData& operator=(IntType val) {
        const auto lock = this->lock();
        this->value() = val;
        this->save();
        this->unlock(lock);
        return *this;
    }

    operator IntType() const {
        const auto lock = this->lock();
        const IntType v = this->value();
        this->unlock(lock);
        return v;
    }

private:
    virtual int get(IntType& val) override { // AbstractIntegerDiagnosticData
        const auto lock = this->lock();
        val = this->value();
        this->unlock(lock);
        return SYSTEM_ERROR_NONE;
    }
};

template<>
class IntegerDiagnosticData<NonPersistentStorage, AtomicConcurrency>: public AbstractIntegerDiagnosticData {
public:
    typedef AtomicConcurrency ConcurrencyType;

    explicit IntegerDiagnosticData(DiagnosticDataId id, IntType val = 0) :
            IntegerDiagnosticData(id, nullptr, val) {
    }

    IntegerDiagnosticData(DiagnosticDataId id, const char* name, IntType val = 0) :
            AbstractIntegerDiagnosticData(id, name),
            val_(val) {
    }

    IntType operator++() {
        return (val_.fetch_add(1, std::memory_order_relaxed) + 1);
    }

    IntType operator++(int) {
        return val_.fetch_add(1, std::memory_order_relaxed);
    }

    IntType operator--() {
        return (val_.fetch_sub(1, std::memory_order_relaxed) - 1);
    }

    IntType operator--(int) {
        return val_.fetch_sub(1, std::memory_order_relaxed);
    }

    IntType operator+=(IntType val) {
        return (val_.fetch_add(val, std::memory_order_relaxed) + val);
    }

    IntType operator-=(IntType val) {
        return (val_.fetch_sub(val, std::memory_order_relaxed) - val);
    }

    IntegerDiagnosticData& operator=(IntType val) {
        val_.store(val, std::memory_order_relaxed);
        return *this;
    }

    operator IntType() const {
        return val_.load(std::memory_order_relaxed);
    }

private:
    std::atomic<IntType> val_;

    virtual int get(IntType& val) override { // AbstractIntegerDiagnosticData
        val = val_.load(std::memory_order_relaxed);
        return SYSTEM_ERROR_NONE;
    }
};

template<typename EnumT, template<typename ValueT, typename KeyT> class StorageT = NonPersistentStorage,
        typename ConcurrencyT = NoConcurrency>
class EnumDiagnosticData:
        public AbstractIntegerDiagnosticData,
        private StorageT<AbstractIntegerDiagnosticData::IntType, DiagnosticDataId>,
        private ConcurrencyT {
public:
    typedef EnumT EnumType;
    typedef StorageT<IntType, DiagnosticDataId> StorageType;
    typedef ConcurrencyT ConcurrencyType;

    EnumDiagnosticData(DiagnosticDataId id, EnumT val) :
            EnumDiagnosticData(id, nullptr, val) {
    }

    EnumDiagnosticData(DiagnosticDataId id, const char* name, EnumT val) :
            AbstractIntegerDiagnosticData(id, name),
            StorageType((IntType)val, id) {
    }

    EnumDiagnosticData& operator=(EnumT val) {
        const auto lock = this->lock();
        this->value() = (IntType)val;
        this->save();
        this->unlock(lock);
        return *this;
    }

    operator EnumT() const {
        const auto lock = this->lock();
        const EnumT v = (EnumT)this->value();
        this->unlock(lock);
        return v;
    }

private:
    virtual int get(IntType& val) override { // AbstractIntegerDiagnosticData
        const auto lock = this->lock();
        val = this->value();
        this->unlock(lock);
        return SYSTEM_ERROR_NONE;
    }
};

template<typename EnumT>
class EnumDiagnosticData<EnumT, NonPersistentStorage, AtomicConcurrency>: public AbstractIntegerDiagnosticData {
public:
    typedef EnumT EnumType;
    typedef AtomicConcurrency ConcurrencyType;

    EnumDiagnosticData(DiagnosticDataId id, EnumT val) :
            EnumDiagnosticData(id, nullptr, val) {
    }

    EnumDiagnosticData(DiagnosticDataId id, const char* name, EnumT val) :
            AbstractIntegerDiagnosticData(id, name),
            val_((IntType)val) {
    }

    EnumDiagnosticData& operator=(EnumT val) {
        val_.store((IntType)val, std::memory_order_relaxed);
        return *this;
    }

    operator EnumT() const {
        return (EnumT)val_.load(std::memory_order_relaxed);
    }

private:
    std::atomic<IntType> val_;

    virtual int get(IntType& val) override { // AbstractIntegerDiagnosticData
        val = val_.load(std::memory_order_relaxed);
        return SYSTEM_ERROR_NONE;
    }
};

// Convenience typedefs
typedef IntegerDiagnosticData<NonPersistentStorage, NoConcurrency> SimpleIntegerDiagnosticData;
typedef IntegerDiagnosticData<NonPersistentStorage, AtomicConcurrency> AtomicIntegerDiagnosticData;

template<typename EnumT>
using SimpleEnumDiagnosticData = EnumDiagnosticData<EnumT, NonPersistentStorage, NoConcurrency>;

template<typename EnumT>
using AtomicEnumDiagnosticData = EnumDiagnosticData<EnumT, NonPersistentStorage, AtomicConcurrency>;

#if Wiring_RetainedMemory

template<DiagnosticDataId id, typename ConcurrencyT = NoConcurrency>
class RetainedIntegerDiagnosticData:
        public IntegerDiagnosticData<RetainedStorage<DiagnosticDataId, id>::template Type, ConcurrencyT> {
public:
    using BaseType = IntegerDiagnosticData<RetainedStorage<DiagnosticDataId, id>::template Type, ConcurrencyT>;
    using typename BaseType::IntType;

    explicit RetainedIntegerDiagnosticData(IntType val = 0) :
            BaseType(id, val) {
    }

    explicit RetainedIntegerDiagnosticData(const char* name, IntType val = 0) :
            BaseType(id, name, val) {
    }
};

template<typename EnumT, DiagnosticDataId id, typename ConcurrencyT = NoConcurrency>
class RetainedEnumDiagnosticData:
        public EnumDiagnosticData<EnumT, RetainedStorage<DiagnosticDataId, id>::template Type, ConcurrencyT> {
public:
    using BaseType = EnumDiagnosticData<EnumT, RetainedStorage<DiagnosticDataId, id>::template Type, ConcurrencyT>;

    explicit RetainedEnumDiagnosticData(EnumT val) :
            BaseType(id, val) {
    }

    RetainedEnumDiagnosticData(const char* name, EnumT val) :
            BaseType(id, name, val) {
    }
};

#endif // Wiring_RetainedMemory

inline AbstractDiagnosticData::AbstractDiagnosticData(DiagnosticDataId id, diag_type type) :
        AbstractDiagnosticData(id, nullptr, type) {
}

inline AbstractDiagnosticData::AbstractDiagnosticData(DiagnosticDataId id, const char* name, diag_type type) :
        d_{ sizeof(diag_source), 0 /* flags */, id, (uint16_t)type, name, this /* data */, callback } {
    diag_register_source(&d_, nullptr);
}

inline int AbstractDiagnosticData::get(DiagnosticDataId id, void* data, size_t& size) {
    const diag_source* src = nullptr;
    const int ret = diag_get_source(id, &src, nullptr);
    if (ret != SYSTEM_ERROR_NONE) {
        return ret;
    }
    return get(src, data, size);
}

inline int AbstractDiagnosticData::get(const diag_source* src, void* data, size_t& size) {
    SPARK_ASSERT(src && src->callback);
    diag_source_get_cmd_data d = { sizeof(diag_source_get_cmd_data), 0 /* reserved */, data, size };
    const int ret = src->callback(src, DIAG_SOURCE_CMD_GET, &d);
    if (ret == SYSTEM_ERROR_NONE) {
        size = d.data_size;
    }
    return ret;
}

inline int AbstractDiagnosticData::callback(const diag_source* src, int cmd, void* data) {
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

inline AbstractIntegerDiagnosticData::AbstractIntegerDiagnosticData(DiagnosticDataId id, const char* name) :
        AbstractDiagnosticData(id, name, DIAG_TYPE_INT) {
}

inline int AbstractIntegerDiagnosticData::get(DiagnosticDataId id, IntType& val) {
    const diag_source* src = nullptr;
    const int ret = diag_get_source(id, &src, nullptr);
    if (ret != SYSTEM_ERROR_NONE) {
        return ret;
    }
    return get(src, val);
}

inline int AbstractIntegerDiagnosticData::get(const diag_source* src, IntType& val) {
    SPARK_ASSERT(src->type == DIAG_TYPE_INT);
    size_t size = sizeof(IntType);
    return AbstractDiagnosticData::get(src, &val, size);
}

inline int AbstractIntegerDiagnosticData::get(void* data, size_t& size) {
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

} // namespace particle
