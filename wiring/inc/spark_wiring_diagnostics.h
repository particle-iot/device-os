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

#include "spark_wiring_global.h"
#include "diagnostics.h"

namespace particle {

class DiagnosticData {
protected:
    DiagnosticData(uint16_t id, diag_source_type type);

    virtual int get(char* data, size_t* size) = 0;
    virtual void reset();

private:
    diag_source d_;

    static int getCallback(const diag_source* src, char* data, size_t* size);
    static void resetCallback(const diag_source* src);
};

class ScalarDiagnosticData: public DiagnosticData {
protected:
    explicit ScalarDiagnosticData(uint16_t id);

    virtual int get(int32_t* val) = 0;

    virtual int get(char* data, size_t* size) override; // DiagnosticData
};

template<typename T>
class SimpleScalarDiagnosticData: public ScalarDiagnosticData {
public:
    explicit SimpleScalarDiagnosticData(uint16_t id, T val = T());

    void setValue(T val);
    T value() const;

protected:
    virtual int get(int32_t* val) override; // ScalarDiagnosticData

private:
    volatile T val_;
};

} // namespace particle

inline particle::DiagnosticData::DiagnosticData(uint16_t id, diag_source_type type) :
        d_{ sizeof(diag_source), id, type, this, getCallback, resetCallback } {
    diag_register_source(&d_, nullptr);
}

inline void particle::DiagnosticData::reset() {
    // Default implementation does nothing
}

inline int particle::DiagnosticData::getCallback(const diag_source* src, char* data, size_t* size) {
    const auto d = static_cast<DiagnosticData*>(src->data);
    return d->get(data, size);
}

inline void particle::DiagnosticData::resetCallback(const diag_source* src) {
    const auto d = static_cast<DiagnosticData*>(src->data);
    return d->reset();
}

inline particle::ScalarDiagnosticData::ScalarDiagnosticData(uint16_t id) :
        DiagnosticData(id, DIAG_SOURCE_TYPE_SCALAR) {
}

inline int particle::ScalarDiagnosticData::get(char* data, size_t* size) {
    if (!data) {
        *size = sizeof(int32_t);
        return SYSTEM_ERROR_NONE;
    }
    if (*size < sizeof(int32_t)) {
        return SYSTEM_ERROR_TOO_LARGE;
    }
    const int ret = get((int32_t*)data);
    if (ret != SYSTEM_ERROR_NONE) {
        return ret;
    }
    *size = sizeof(int32_t);
    return SYSTEM_ERROR_NONE;
}

template<typename T>
inline particle::SimpleScalarDiagnosticData<T>::SimpleScalarDiagnosticData(uint16_t id, T val) :
        ScalarDiagnosticData(id),
        val_(val) {
}

template<typename T>
inline void particle::SimpleScalarDiagnosticData<T>::setValue(T val) {
    val_ = val;
}

template<typename T>
inline T particle::SimpleScalarDiagnosticData<T>::value() const {
    return val_;
}

template<typename T>
inline int particle::SimpleScalarDiagnosticData<T>::get(int32_t* val) {
    *val = val_;
    return SYSTEM_ERROR_NONE;
}
