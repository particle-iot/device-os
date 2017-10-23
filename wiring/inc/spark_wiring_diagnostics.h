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
#include "combine_hash.h"
#include "underlying_type.h"
#include "debug.h"

#include <atomic>

#define PARTICLE_RETAINED_INTEGER_DIAGNOSTIC_DATA(_var, _id, _name, _val, ...) \
        PARTICLE_RETAINED ::particle::RetainedIntegerDiagnosticDataStorage _storage##_id; \
        ::particle::PersistentIntegerDiagnosticData<decltype(_storage##_id), ##__VA_ARGS__> _var(_storage##_id, _id, _val);

#define PARTICLE_RETAINED_ENUM_DIAGNOSTIC_DATA(_var, _id, _name, _val, ...) \
        PARTICLE_RETAINED ::particle::RetainedEnumDiagnosticDataStorage<decltype(_val)> _storage##_id; \
        ::particle::PersistentEnumDiagnosticData<decltype(_val), decltype(_storage##_id), ##__VA_ARGS__> _var(_storage##_id, _id, _val);

namespace particle {

typedef decltype(diag_source::id) DiagnosticDataId;

// Base abstract class for a diagnostic data source
class AbstractDiagnosticData {
public:
    DiagnosticDataId id() const;

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

template<typename ConcurrencyT = NoConcurrency>
class IntegerDiagnosticData:
        public AbstractIntegerDiagnosticData,
        private ConcurrencyT {
public:
    explicit IntegerDiagnosticData(DiagnosticDataId id, IntType val = 0) :
            IntegerDiagnosticData(id, nullptr, val) {
    }

    IntegerDiagnosticData(DiagnosticDataId id, const char* name, IntType val = 0) :
            AbstractIntegerDiagnosticData(id, name),
            val_(val) {
    }

    IntType operator++() {
        const auto lock = ConcurrencyT::lock();
        const IntType v = ++val_;
        ConcurrencyT::unlock(lock);
        return v;
    }

    IntType operator++(int) {
        const auto lock = ConcurrencyT::lock();
        const IntType v = val_++;
        ConcurrencyT::unlock(lock);
        return v;
    }

    IntType operator--() {
        const auto lock = ConcurrencyT::lock();
        const IntType v = --val_;
        ConcurrencyT::unlock(lock);
        return v;
    }

    IntType operator--(int) {
        const auto lock = ConcurrencyT::lock();
        const IntType v = val_--;
        ConcurrencyT::unlock(lock);
        return v;
    }

    IntType operator+=(IntType val) {
        const auto lock = ConcurrencyT::lock();
        const IntType v = (val_ += val);
        ConcurrencyT::unlock(lock);
        return v;
    }

    IntType operator-=(IntType val) {
        const auto lock = ConcurrencyT::lock();
        const IntType v = (val_ -= val);
        ConcurrencyT::unlock(lock);
        return v;
    }

    IntegerDiagnosticData& operator=(IntType val) {
        const auto lock = ConcurrencyT::lock();
        val_ = val;
        ConcurrencyT::unlock(lock);
        return *this;
    }

    operator IntType() const {
        const auto lock = ConcurrencyT::lock();
        const IntType v = val_;
        ConcurrencyT::unlock(lock);
        return v;
    }

private:
    IntType val_;

    virtual int get(IntType& val) override { // AbstractIntegerDiagnosticData
        const auto lock = ConcurrencyT::lock();
        val = val_;
        ConcurrencyT::unlock(lock);
        return SYSTEM_ERROR_NONE;
    }
};

template<>
class IntegerDiagnosticData<AtomicConcurrency>: public AbstractIntegerDiagnosticData {
public:
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

template<typename StorageT, typename ConcurrencyT = NoConcurrency>
class PersistentIntegerDiagnosticData:
        public AbstractIntegerDiagnosticData,
        private ConcurrencyT {
public:
    PersistentIntegerDiagnosticData(StorageT& storage, DiagnosticDataId id, IntType val = 0) :
            PersistentIntegerDiagnosticData(storage, id, nullptr, val) {
    }

    PersistentIntegerDiagnosticData(StorageT& storage, DiagnosticDataId id, const char* name, IntType val = 0) :
            AbstractIntegerDiagnosticData(id, name),
            storage_(storage) {
        storage_.init(id, val);
    }

    IntType operator++() {
        const auto lock = ConcurrencyT::lock();
        const IntType v = ++storage_.value();
        storage_.update(id());
        ConcurrencyT::unlock(lock);
        return v;
    }

    IntType operator++(int) {
        const auto lock = ConcurrencyT::lock();
        const IntType v = storage_.value()++;
        storage_.update(id());
        ConcurrencyT::unlock(lock);
        return v;
    }

    IntType operator--() {
        const auto lock = ConcurrencyT::lock();
        const IntType v = --storage_.value();
        storage_.update(id());
        ConcurrencyT::unlock(lock);
        return v;
    }

    IntType operator--(int) {
        const auto lock = ConcurrencyT::lock();
        const IntType v = storage_.value()--;
        storage_.update(id());
        ConcurrencyT::unlock(lock);
        return v;
    }

    IntType operator+=(IntType val) {
        const auto lock = ConcurrencyT::lock();
        const IntType v = (storage_.value() += val);
        storage_.update(id());
        ConcurrencyT::unlock(lock);
        return v;
    }

    IntType operator-=(IntType val) {
        const auto lock = ConcurrencyT::lock();
        const IntType v = (storage_.value() -= val);
        storage_.update(id());
        ConcurrencyT::unlock(lock);
        return v;
    }

    PersistentIntegerDiagnosticData& operator=(IntType val) {
        const auto lock = ConcurrencyT::lock();
        storage_.value() = val;
        storage_.update(id());
        ConcurrencyT::unlock(lock);
        return *this;
    }

    operator IntType() const {
        const auto lock = ConcurrencyT::lock();
        const IntType v = storage_.value();
        ConcurrencyT::unlock(lock);
        return v;
    }

private:
    StorageT& storage_;

    virtual int get(IntType& val) override { // AbstractIntegerDiagnosticData
        const auto lock = ConcurrencyT::lock();
        val = storage_.value();
        ConcurrencyT::unlock(lock);
        return SYSTEM_ERROR_NONE;
    }
};

template<typename EnumT, typename ConcurrencyT = NoConcurrency>
class EnumDiagnosticData:
        public AbstractIntegerDiagnosticData,
        private ConcurrencyT {
public:
    typedef EnumT EnumType;

    EnumDiagnosticData(DiagnosticDataId id, EnumT val) :
            EnumDiagnosticData(id, nullptr, val) {
    }

    EnumDiagnosticData(DiagnosticDataId id, const char* name, EnumT val) :
            AbstractIntegerDiagnosticData(id, name),
            val_((IntType)val) {
    }

    EnumDiagnosticData& operator=(EnumT val) {
        const auto lock = ConcurrencyT::lock();
        val_ = (IntType)val;
        ConcurrencyT::unlock(lock);
        return *this;
    }

    operator EnumT() const {
        const auto lock = ConcurrencyT::lock();
        const EnumT v = (EnumT)val_;
        ConcurrencyT::unlock(lock);
        return v;
    }

private:
    IntType val_;

    virtual int get(IntType& val) override { // AbstractIntegerDiagnosticData
        const auto lock = ConcurrencyT::lock();
        val = (IntType)val_;
        ConcurrencyT::unlock(lock);
        return SYSTEM_ERROR_NONE;
    }
};

template<typename EnumT>
class EnumDiagnosticData<EnumT, AtomicConcurrency>: public AbstractIntegerDiagnosticData {
public:
    typedef EnumT EnumType;

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

template<typename EnumT, typename StorageT, typename ConcurrencyT = NoConcurrency>
class PersistentEnumDiagnosticData:
        public AbstractIntegerDiagnosticData,
        private ConcurrencyT {
public:
    typedef EnumT EnumType;
    typedef typename StorageT::ValueType ValueType;

    PersistentEnumDiagnosticData(StorageT& storage, DiagnosticDataId id, EnumT val) :
            PersistentEnumDiagnosticData(storage, id, nullptr, val) {
    }

    PersistentEnumDiagnosticData(StorageT& storage, DiagnosticDataId id, const char* name, EnumT val) :
            AbstractIntegerDiagnosticData(id, name),
            storage_(storage) {
        storage_.init(id, (ValueType)val);
    }

    PersistentEnumDiagnosticData& operator=(EnumT val) {
        const auto lock = ConcurrencyT::lock();
        storage_.value() = (ValueType)val;
        storage_.update(id());
        ConcurrencyT::unlock(lock);
        return *this;
    }

    operator EnumT() const {
        const auto lock = ConcurrencyT::lock();
        const EnumT v = (EnumT)storage_.value();
        ConcurrencyT::unlock(lock);
        return v;
    }

private:
    StorageT& storage_;

    virtual int get(IntType& val) override { // AbstractIntegerDiagnosticData
        const auto lock = ConcurrencyT::lock();
        val = (IntType)storage_.value();
        ConcurrencyT::unlock(lock);
        return SYSTEM_ERROR_NONE;
    }
};

template<typename ValueT>
class RetainedDiagnosticDataStorage {
public:
    typedef ValueT ValueType;

    void init(DiagnosticDataId id, ValueT val) {
        const size_t check = checksum(id);
        if (check_ != check) {
            val_ = std::move(val);
            check_ = check;
        }
    }

    void update(DiagnosticDataId id) {
        check_ = checksum(id);
    }

    ValueT& value() {
        return val_;
    }

    const ValueT& value() const {
        return val_;
    }

private:
    ValueT val_;
    size_t check_;

    size_t checksum(DiagnosticDataId id) const {
        size_t h = 0;
        combineHash(h, id);
        combineHash(h, (typename UnderlyingType<ValueT>::Type)val_);
        return h;
    }
};

// Convenience typedefs
typedef IntegerDiagnosticData<NoConcurrency> SimpleIntegerDiagnosticData;
typedef IntegerDiagnosticData<AtomicConcurrency> AtomicIntegerDiagnosticData;
typedef RetainedDiagnosticDataStorage<AbstractIntegerDiagnosticData::IntType> RetainedIntegerDiagnosticDataStorage;

template<typename EnumT>
using SimpleEnumDiagnosticData = EnumDiagnosticData<EnumT, NoConcurrency>;

template<typename EnumT>
using AtomicEnumDiagnosticData = EnumDiagnosticData<EnumT, AtomicConcurrency>;

template<typename EnumT>
using RetainedEnumDiagnosticDataStorage = RetainedDiagnosticDataStorage<EnumT>;

inline AbstractDiagnosticData::AbstractDiagnosticData(DiagnosticDataId id, diag_type type) :
        AbstractDiagnosticData(id, nullptr, type) {
}

inline AbstractDiagnosticData::AbstractDiagnosticData(DiagnosticDataId id, const char* name, diag_type type) :
        d_{ sizeof(diag_source), 0 /* flags */, id, (uint16_t)type, name, this /* data */, callback } {
    diag_register_source(&d_, nullptr);
}

inline DiagnosticDataId AbstractDiagnosticData::id() const {
    return d_.id;
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
