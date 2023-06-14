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

#include <variant>
#include <cstdint>

#include "spark_wiring_string.h"
#include "spark_wiring_stream.h"
#include "spark_wiring_vector.h"
#include "spark_wiring_map.h"

namespace particle {

class Variant;

typedef Vector<Variant> VariantArray;
typedef Map<Variant, Variant> VariantMap;

class Variant {
public:
    enum Type {
        // INVALID,
        NULL_,
        BOOL,
        INT,
        INT64,
        DOUBLE,
        STRING,
        ARRAY,
        MAP
    };

    Variant() = default;

    Variant(bool value);
    Variant(char value);
    Variant(unsigned char value);
    Variant(short value);
    Variant(unsigned short value);
    Variant(int value);
    Variant(unsigned value);
    Variant(long value);
    Variant(unsigned long value);
    Variant(long long value);
    Variant(unsigned long long value);
    Variant(double value);
    Variant(const char* value);
    Variant(String value);
    Variant(VariantArray value);
    Variant(VariantMap value);

    Variant(const Variant& variant);
    Variant(Variant&& variant);

    Type type() const;

    // bool isValid() const;
    bool isNull() const;
    bool isBool() const;
    bool isInt() const;
    bool isInt64() const;
    bool isDouble() const;
    bool isString() const;
    bool isArray() const;
    bool isMap() const;

    bool toBool() const;
    int toInt() const;
    int64_t toInt64() const;
    double toDouble() const;
    String toString() const;
    VariantArray toArray() const;
    VariantMap toMap() const;

    // TODO: Add helper methods for in-place modification of array, map and string variants -
    // see the Vector, Map and String classes respectively

    String toJson() const;
    void toJson(Print& stream) const;

    template<typename T>
    T valueAs();

    template<typename T>
    T& valueRef();

    template<typename T>
    const T& valueRef() const;

    Variant& operator=(Variant variant);

    bool operator==(const Variant& variant) const;
    bool operator<(const Variant& variant) const;

    // Conversion operators
    operator bool() const;
    operator char() const;
    operator unsigned char() const;
    operator short() const;
    operator unsigned short() const;
    operator int() const;
    operator unsigned() const;
    operator int64_t() const;
    operator double() const;
    operator const char*() const;
    operator String() const;
    operator VariantArray() const;
    operator VariantMap() const;

    static Variant fromJson(const char* str);
    static Variant fromJson(Stream& stream);

    friend void swap(Variant& variant1, Variant& variant2);

private:
    using VariantType = std::variant<std::monostate, bool, int, int64_t, double, String, VariantArray, VariantMap>;

    VariantType v_;
};

} // namespace particle
