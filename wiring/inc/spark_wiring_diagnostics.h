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

#include "spark_wiring_error.h"
#include "spark_wiring_global.h"

#include "diagnostics.h"

#include <atomic>

namespace particle {

// Base abstract class for a diagnostic data source
class DiagnosticData {
public:
    // Convenience methods invoking the getter callback
    static int get(uint16_t id, void* data, size_t& size);
    static int get(const diag_source* src, void* data, size_t& size);

    // This class is non-copyable
    DiagnosticData(const DiagnosticData&) = delete;
    DiagnosticData& operator=(const DiagnosticData&) = delete;

protected:
    DiagnosticData(uint16_t id, diag_type type);
    DiagnosticData(uint16_t id, const char* name, diag_type type);

    virtual int get(void* data, size_t& size) = 0;

private:
    diag_source d_;

    static int callback(const diag_source* src, int cmd, void* data);
};

// Base abstract class for a data source containing an integer value
class IntegerDiagnosticData: public DiagnosticData {
public:
    typedef int32_t IntType; // Underlying integer type

    // Convenience methods invoking the getter callback
    static int get(uint16_t id, int32_t& val);
    static int get(const diag_source* src, int32_t& val);

protected:
    explicit IntegerDiagnosticData(uint16_t id, const char* name = nullptr);

    virtual int get(int32_t& val) = 0;

private:
    virtual int get(void* data, size_t& size) override; // DiagnosticData
};

class SimpleIntegerDiagnosticData: public IntegerDiagnosticData {
public:
    explicit SimpleIntegerDiagnosticData(uint16_t id, int32_t val = 0);
    SimpleIntegerDiagnosticData(uint16_t id, const char* name, int32_t val = 0);

    int32_t operator++();
    int32_t operator++(int);
    int32_t operator--();
    int32_t operator--(int);
    int32_t operator+=(int32_t val);
    int32_t operator-=(int32_t val);

    SimpleIntegerDiagnosticData& operator=(int32_t val);
    operator int32_t() const;

private:
    int32_t val_;

    virtual int get(int32_t& val) override; // IntegerDiagnosticData
};

class AtomicIntegerDiagnosticData: public IntegerDiagnosticData {
public:
    explicit AtomicIntegerDiagnosticData(uint16_t id, int32_t val = 0);
    AtomicIntegerDiagnosticData(uint16_t id, const char* name, int32_t val = 0);

    int32_t operator++();
    int32_t operator++(int);
    int32_t operator--();
    int32_t operator--(int);
    int32_t operator+=(int32_t val);
    int32_t operator-=(int32_t val);

    AtomicIntegerDiagnosticData& operator=(int32_t val);
    operator int32_t() const;

private:
    std::atomic<int32_t> val_;

    virtual int get(int32_t& val) override; // IntegerDiagnosticData
};

template<typename T>
class SimpleEnumDiagnosticData: public IntegerDiagnosticData {
public:
    typedef T EnumType;

    explicit SimpleEnumDiagnosticData(uint16_t id, T val);
    SimpleEnumDiagnosticData(uint16_t id, const char* name, T val);

    SimpleEnumDiagnosticData& operator=(T val);
    operator T() const;

private:
    int32_t val_;

    virtual int get(int32_t& val) override; // IntegerDiagnosticData
};

template<typename T>
class AtomicEnumDiagnosticData: public IntegerDiagnosticData {
public:
    typedef T EnumType;

    explicit AtomicEnumDiagnosticData(uint16_t id, T val);
    AtomicEnumDiagnosticData(uint16_t id, const char* name, T val);

    AtomicEnumDiagnosticData& operator=(T val);
    operator T() const;

private:
    std::atomic<int32_t> val_;

    virtual int get(int32_t& val) override; // IntegerDiagnosticData
};

const uint16_t DIAGNOSTIC_DATA_USER_ID = DIAG_SOURCE_USER;

} // namespace particle

inline particle::DiagnosticData::DiagnosticData(uint16_t id, diag_type type) :
        DiagnosticData(id, nullptr, type) {
}

inline particle::DiagnosticData::DiagnosticData(uint16_t id, const char* name, diag_type type) :
        d_{ sizeof(diag_source) /* size */, id, (uint16_t)type, name, 0 /* flags */, this /* data */, callback } {
    diag_register_source(&d_, nullptr);
}

inline particle::IntegerDiagnosticData::IntegerDiagnosticData(uint16_t id, const char* name) :
        DiagnosticData(id, name, DIAG_TYPE_INT) {
}

inline particle::SimpleIntegerDiagnosticData::SimpleIntegerDiagnosticData(uint16_t id, int32_t val) :
        SimpleIntegerDiagnosticData(id, nullptr, val) {
}

inline particle::SimpleIntegerDiagnosticData::SimpleIntegerDiagnosticData(uint16_t id, const char* name, int32_t val) :
        IntegerDiagnosticData(id, name),
        val_(val) {
}

inline int32_t particle::SimpleIntegerDiagnosticData::operator++() {
    return ++val_;
}

inline int32_t particle::SimpleIntegerDiagnosticData::operator++(int) {
    return val_++;
}

inline int32_t particle::SimpleIntegerDiagnosticData::operator--() {
    return --val_;
}

inline int32_t particle::SimpleIntegerDiagnosticData::operator--(int) {
    return val_--;
}

inline int32_t particle::SimpleIntegerDiagnosticData::operator+=(int32_t val) {
    return (val_ += val);
}

inline int32_t particle::SimpleIntegerDiagnosticData::operator-=(int32_t val) {
    return (val_ -= val);
}

inline particle::SimpleIntegerDiagnosticData& particle::SimpleIntegerDiagnosticData::operator=(int32_t val) {
    val_ = val;
    return *this;
}

inline particle::SimpleIntegerDiagnosticData::operator int32_t() const {
    return val_;
}

inline int particle::SimpleIntegerDiagnosticData::get(int32_t& val) {
    val = val_;
    return SYSTEM_ERROR_NONE;
}

inline particle::AtomicIntegerDiagnosticData::AtomicIntegerDiagnosticData(uint16_t id, int32_t val) :
        AtomicIntegerDiagnosticData(id, nullptr, val) {
}

inline particle::AtomicIntegerDiagnosticData::AtomicIntegerDiagnosticData(uint16_t id, const char* name, int32_t val) :
        IntegerDiagnosticData(id, name),
        val_(val) {
}

inline int32_t particle::AtomicIntegerDiagnosticData::operator++() {
    return (val_.fetch_add(1, std::memory_order_relaxed) + 1);
}

inline int32_t particle::AtomicIntegerDiagnosticData::operator++(int) {
    return val_.fetch_add(1, std::memory_order_relaxed);
}

inline int32_t particle::AtomicIntegerDiagnosticData::operator--() {
    return (val_.fetch_sub(1, std::memory_order_relaxed) - 1);
}

inline int32_t particle::AtomicIntegerDiagnosticData::operator--(int) {
    return val_.fetch_sub(1, std::memory_order_relaxed);
}

inline int32_t particle::AtomicIntegerDiagnosticData::operator+=(int32_t val) {
    return (val_.fetch_add(val, std::memory_order_relaxed) + val);
}

inline int32_t particle::AtomicIntegerDiagnosticData::operator-=(int32_t val) {
    return (val_.fetch_sub(val, std::memory_order_relaxed) - val);
}

inline particle::AtomicIntegerDiagnosticData& particle::AtomicIntegerDiagnosticData::operator=(int32_t val) {
    val_.store(val, std::memory_order_relaxed);
    return *this;
}

inline particle::AtomicIntegerDiagnosticData::operator int32_t() const {
    return val_.load(std::memory_order_relaxed);
}

inline int particle::AtomicIntegerDiagnosticData::get(int32_t& val) {
    val = val_.load(std::memory_order_relaxed);
    return SYSTEM_ERROR_NONE;
}

template<typename T>
inline particle::SimpleEnumDiagnosticData<T>::SimpleEnumDiagnosticData(uint16_t id, T val) :
        SimpleEnumDiagnosticData(id, nullptr, val) {
}

template<typename T>
inline particle::SimpleEnumDiagnosticData<T>::SimpleEnumDiagnosticData(uint16_t id, const char* name, T val) :
        IntegerDiagnosticData(id, name),
        val_((int32_t)val) {
}

template<typename T>
inline particle::SimpleEnumDiagnosticData<T>& particle::SimpleEnumDiagnosticData<T>::operator=(T val) {
    val_ = (int32_t)val;
    return *this;
}

template<typename T>
inline particle::SimpleEnumDiagnosticData<T>::operator T() const {
    return (T)val_;
}

template<typename T>
inline int particle::SimpleEnumDiagnosticData<T>::get(int32_t& val) {
    val = val_;
    return SYSTEM_ERROR_NONE;
}

template<typename T>
inline particle::AtomicEnumDiagnosticData<T>::AtomicEnumDiagnosticData(uint16_t id, T val) :
        AtomicEnumDiagnosticData(id, nullptr, val) {
}

template<typename T>
inline particle::AtomicEnumDiagnosticData<T>::AtomicEnumDiagnosticData(uint16_t id, const char* name, T val) :
        IntegerDiagnosticData(id, name),
        val_((int32_t)val) {
}

template<typename T>
inline particle::AtomicEnumDiagnosticData<T>& particle::AtomicEnumDiagnosticData<T>::operator=(T val) {
    val_.store((int32_t)val, std::memory_order_relaxed);
    return *this;
}

template<typename T>
inline particle::AtomicEnumDiagnosticData<T>::operator T() const {
    return (T)val_.load(std::memory_order_relaxed);
}

template<typename T>
inline int particle::AtomicEnumDiagnosticData<T>::get(int32_t& val) {
    val = val_.load(std::memory_order_relaxed);
    return SYSTEM_ERROR_NONE;
}
