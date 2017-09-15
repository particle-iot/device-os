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

class AbstractDiagnosticData {
protected:
    AbstractDiagnosticData(uint16_t id, diag_data_type type);
    AbstractDiagnosticData(uint16_t id, const char* name, diag_data_type type);

    virtual int get(char* data, size_t& size) = 0;

private:
    diag_source d_;

    static int callback(const diag_source* src, int cmd, void* data);
};

class AbstractIntegerDiagnosticData: public AbstractDiagnosticData {
protected:
    explicit AbstractIntegerDiagnosticData(uint16_t id, const char* name = nullptr);

    virtual int get(int32_t& val) = 0;

private:
    virtual int get(char* data, size_t& size) override; // AbstractDiagnosticData
};

class IntegerDiagnosticData: public AbstractIntegerDiagnosticData {
public:
    explicit IntegerDiagnosticData(uint16_t id, int32_t val = 0);
    IntegerDiagnosticData(uint16_t id, const char* name, int32_t val = 0);

    int32_t operator++();
    int32_t operator++(int);
    int32_t operator--();
    int32_t operator--(int);

    int32_t operator+=(int32_t val);
    int32_t operator-=(int32_t val);
    int32_t operator&=(int32_t val);
    int32_t operator|=(int32_t val);
    int32_t operator^=(int32_t val);

    IntegerDiagnosticData& operator=(int32_t val);
    operator int32_t() const;

private:
    std::atomic<int32_t> val_;

    virtual int get(int32_t& val) override; // AbstractIntegerDiagnosticData
};

const uint16_t DIAGNOSTIC_DATA_USER_ID = DIAG_SOURCE_USER;

} // namespace particle

inline particle::AbstractDiagnosticData::AbstractDiagnosticData(uint16_t id, diag_data_type type) :
        AbstractDiagnosticData(id, nullptr, type) {
}

inline particle::AbstractDiagnosticData::AbstractDiagnosticData(uint16_t id, const char* name, diag_data_type type) :
        d_{ sizeof(diag_source), id, type, name, 0 /* flags */, this, callback } {
    diag_register_source(&d_, nullptr);
}

inline particle::AbstractIntegerDiagnosticData::AbstractIntegerDiagnosticData(uint16_t id, const char* name) :
        AbstractDiagnosticData(id, name, DIAG_DATA_TYPE_INTEGER) {
}

inline particle::IntegerDiagnosticData::IntegerDiagnosticData(uint16_t id, int32_t val) :
        IntegerDiagnosticData(id, nullptr, val) {
}

inline particle::IntegerDiagnosticData::IntegerDiagnosticData(uint16_t id, const char* name, int32_t val) :
        AbstractIntegerDiagnosticData(id, name),
        val_(val) {
}

inline int32_t particle::IntegerDiagnosticData::operator++() {
    return (val_.fetch_add(1, std::memory_order_relaxed) + 1);
}

inline int32_t particle::IntegerDiagnosticData::operator++(int) {
    return val_.fetch_add(1, std::memory_order_relaxed);
}

inline int32_t particle::IntegerDiagnosticData::operator--() {
    return (val_.fetch_sub(1, std::memory_order_relaxed) - 1);
}

inline int32_t particle::IntegerDiagnosticData::operator--(int) {
    return val_.fetch_sub(1, std::memory_order_relaxed);
}

inline int32_t particle::IntegerDiagnosticData::operator+=(int32_t val) {
    return (val_.fetch_add(val, std::memory_order_relaxed) + val);
}

inline int32_t particle::IntegerDiagnosticData::operator-=(int32_t val) {
    return (val_.fetch_sub(val, std::memory_order_relaxed) - val);
}

inline int32_t particle::IntegerDiagnosticData::operator&=(int32_t val) {
    return (val_.fetch_and(val, std::memory_order_relaxed) & val);
}

inline int32_t particle::IntegerDiagnosticData::operator|=(int32_t val) {
    return (val_.fetch_or(val, std::memory_order_relaxed) | val);
}

inline int32_t particle::IntegerDiagnosticData::operator^=(int32_t val) {
    return (val_.fetch_xor(val, std::memory_order_relaxed) ^ val);
}

inline particle::IntegerDiagnosticData& particle::IntegerDiagnosticData::operator=(int32_t val) {
    val_.store(val, std::memory_order_relaxed);
    return *this;
}

inline particle::IntegerDiagnosticData::operator int32_t() const {
    return val_.load(std::memory_order_relaxed);
}

inline int particle::IntegerDiagnosticData::get(int32_t& val) {
    val = val_.load(std::memory_order_relaxed);
    return SYSTEM_ERROR_NONE;
}
