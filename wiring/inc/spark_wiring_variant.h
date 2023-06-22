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
#include <type_traits>
#include <algorithm>
#include <charconv>
#include <cstdint>

#include "spark_wiring_string.h"
#include "spark_wiring_stream.h"
#include "spark_wiring_vector.h"
#include "spark_wiring_map.h"

#include "debug.h"

namespace particle {

class Variant;

typedef Vector<Variant> VariantArray;
typedef Map<String, Variant> VariantMap;

class Variant {
public:
    enum Type {
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

    Variant(const std::monostate&) :
            Variant() {
    }

    Variant(bool value) :
            v_(value) {
    }

    Variant(int8_t value) :
            v_(static_cast<int>(value)) {
    }

    Variant(uint8_t value) :
            v_(static_cast<int>(value)) {
    }

    Variant(int16_t value) :
            v_(static_cast<int>(value)) {
    }

    Variant(uint16_t value) :
            v_(static_cast<int>(value)) {
    }

    Variant(int value) :
            v_(value) {
    }

    Variant(unsigned value) :
            v_(static_cast<int64_t>(value)) {
    }

    Variant(int64_t value) :
            v_(value) {
    }

    Variant(double value) :
            v_(value) {
    }

    Variant(const char* value) :
            v_(String(value)) {
    }

    Variant(String value) :
            v_(std::move(value)) {
    }

    Variant(VariantArray value) :
            v_(std::move(value)) {
    }

    Variant(VariantMap value) :
            v_(std::move(value)) {
    }

    Variant(const Variant& variant) :
            v_(variant.v_) {
    }

    Variant(Variant&& variant) :
            Variant() {
        swap(*this, variant);
    }

    Type type() const {
        return static_cast<Type>(v_.index());
    }

    bool isNull() const {
        return is<std::monostate>();
    }

    bool isBool() const {
        return is<bool>();
    }

    bool isInt() const {
        return is<int>();
    }

    bool isInt64() const {
        return is<int64_t>();
    }

    bool isDouble() const {
        return is<double>();
    }

    bool isNumber() const {
        auto t = type();
        return t >= Type::INT && t <= Type::DOUBLE;
    }

    bool isString() const {
        return is<String>();
    }

    bool isArray() const {
        return is<VariantArray>();
    }

    bool isMap() const {
        return is<VariantMap>();
    }

    template<typename T>
    bool is() const {
        // It's not safe to call std::holds_alternative() with a type that is not one of the variant's
        // alternative types
        if constexpr (std::is_same_v<T, std::monostate> ||
                std::is_same_v<T, bool> ||
                std::is_same_v<T, int> ||
                std::is_same_v<T, int64_t> ||
                std::is_same_v<T, double> ||
                std::is_same_v<T, String> ||
                std::is_same_v<T, VariantArray> ||
                std::is_same_v<T, VariantMap>) {
            return std::holds_alternative<T>(v_);
        } else {
            return false;
        }
    }

    // Conversion methods
    bool toBool() const {
        return to<bool>();
    }

    int toInt() const {
        return to<int>();
    }

    int64_t toInt64() const {
        return to<int64_t>();
    }

    double toDouble() const {
        return to<double>();
    }

    String toString() const {
        return to<String>();
    }

    VariantArray toArray() const {
        return to<VariantArray>();
    }

    VariantMap toMap() const {
        return to<VariantMap>();
    }

    template<typename T>
    T to() const;

    template<typename T>
    T to(const T& defaultValue) const {
        if (!isConvertibleTo<T>()) {
            return defaultValue;
        }
        return to<T>();
    }

    template<typename T>
    bool isConvertibleTo() const;

    // Access to the underlying value
    bool& asBool() {
        return as<bool>();
    }

    int& asInt() {
        return as<int>();
    }

    int64_t& asInt64() {
        return as<int64_t>();
    }

    double& asDouble() {
        return as<double>();
    }

    String& asString() {
        return as<String>();
    }

    VariantArray& asArray() {
        return as<VariantArray>();
    }

    VariantMap& asMap() {
        return as<VariantMap>();
    }

    template<typename T>
    T& as() {
        if (!is<T>()) {
            v_ = to<T>();
        }
        return value<T>();
    }

    template<typename T>
    T& value() {
        return std::get<T>(v_);
    }

    template<typename T>
    const T& value() const {
        return std::get<T>(v_);
    }

    // Array operations
    Variant& append(Variant value) {
        bool ok = asArray().append(std::move(value));
        SPARK_ASSERT(ok);
        return *this;
    }

    Variant& prepend(Variant value) {
        bool ok = asArray().prepend(std::move(value));
        SPARK_ASSERT(ok);
        return *this;
    }

    Variant at(int index) const {
        if (!isArray()) {
            return Variant();
        }
        auto& arr = value<VariantArray>();
        if (index < 0 || index >= arr.size()) {
            return Variant();
        }
        return arr.at(index);
    }

    // Map operations
    Variant& set(const char* key, Variant value) {
        bool ok = asMap().set(key, std::move(value));
        SPARK_ASSERT(ok);
        return *this;
    }

    Variant& set(const String& key, Variant value) {
        bool ok = asMap().set(key, std::move(value));
        SPARK_ASSERT(ok);
        return *this;
    }

    Variant get(const char* key) const {
        if (!isMap()) {
            return Variant();
        }
        return value<VariantMap>().get(key);
    }

    Variant get(const String& key) const {
        if (!isMap()) {
            return Variant();
        }
        return value<VariantMap>().get(key);
    }

    bool has(const char* key) const {
        if (!isMap()) {
            return false;
        }
        return value<VariantMap>().has(key);
    }

    bool has(const String& key) const {
        if (!isMap()) {
            return false;
        }
        return value<VariantMap>().has(key);
    }

    Vector<String> keys() const {
        if (!isMap()) {
            return Vector<String>();
        }
        return value<VariantMap>().keys();
    }

    int size() const {
        switch (type()) {
        case Type::STRING:
            return value<String>().length();
        case Type::ARRAY:
            return value<VariantArray>().size();
        case Type::MAP:
            return value<VariantMap>().size();
        default:
            return 0;
        }
    }

    bool isEmpty() const {
        switch (type()) {
        case Type::NULL_:
            return true; // A default-constructed Variant is empty
        case Type::STRING:
            return value<String>().length() == 0;
        case Type::ARRAY:
            return value<VariantArray>().isEmpty();
        case Type::MAP:
            return value<VariantMap>().isEmpty();
        default:
            return false;
        }
    }

    Variant& operator[](int index) {
        SPARK_ASSERT(isArray());
        return value<VariantArray>().at(index);
    }

    const Variant& operator[](int index) const {
        SPARK_ASSERT(isArray());
        return value<VariantArray>().at(index);
    }

    Variant& operator[](const char* key) {
        return asMap().operator[](key);
    }

    Variant& operator[](const String& key) {
        return asMap().operator[](key);
    }

    Variant& operator=(Variant variant) {
        swap(*this, variant);
        return *this;
    }

    bool operator==(const Variant& variant) const {
        return v_ == variant.v_;
    }

    bool operator!=(const Variant& variant) const {
        return !operator==(variant);
    }

    friend void swap(Variant& variant1, Variant& variant2) {
        using std::swap; // For ADL
        swap(variant1.v_, variant2.v_);
    }

private:
    using VariantType = std::variant<std::monostate, bool, int, int64_t, double, String, VariantArray, VariantMap>;

    VariantType v_;
};

namespace detail {

template<typename TargetT, typename EnableT = void>
class ConvertToVariantVisitor {
public:
    template<typename SourceT>
    TargetT operator()(const SourceT& val) const {
        return TargetT();
    }
};

// TargetT=std::monostate
template<>
class ConvertToVariantVisitor<std::monostate> {
public:
    // SourceT=std::monostate|bool|int|int64_t|double|String|VariantArray|VariantMap
    template<typename SourceT>
    std::monostate operator()(const SourceT& val) const {
        return std::monostate();
    }
};

// TargetT=bool
template<>
class ConvertToVariantVisitor<bool> {
public:
    bool operator()(const String& val) const {
        if (val == "true") {
            return true;
        }
        return false;
    }

    template<typename SourceT>
    bool operator()(const SourceT& val) const {
        if constexpr (std::is_arithmetic_v<SourceT>) { // SourceT=bool|int|int64_t|double
            return static_cast<bool>(val);
        } else { // SourceT=std::monostate|VariantArray|VariantMap
            return false;
        }
    }
};

// TargetT=int|int64_t|double
template<typename TargetT>
class ConvertToVariantVisitor<TargetT, std::enable_if_t<std::is_arithmetic_v<TargetT>>> {
public:
    TargetT operator()(const String& val) const {
        TargetT v = TargetT();
        auto end = val.c_str() + val.length();
        auto r = std::from_chars(val.c_str(), end, v);
        if (r.ec != std::errc() || r.ptr != end) {
            return TargetT();
        }
        return v;
    }

    template<typename SourceT>
    TargetT operator()(const SourceT& val) const {
        if constexpr (std::is_arithmetic_v<SourceT>) { // SourceT=bool|int|int64_t|double
            return static_cast<TargetT>(val);
        } else { // SourceT=std::monostate|VariantArray|VariantMap
            return TargetT();
        }
    }
};

// TargetT=String
template<>
class ConvertToVariantVisitor<String> {
public:
    String operator()(bool val) const {
        return val ? "true" : "false";
    }

    String operator()(const String& val) const {
        return val;
    }

    template<typename SourceT>
    String operator()(const SourceT& val) const {
        if constexpr (std::is_arithmetic_v<SourceT>) { // SourceT=int|int64_t|double
            char buf[32]; // Large enough for all relevant types
            auto r = std::to_chars(buf, buf + sizeof(buf), val);
            SPARK_ASSERT(r.ec == std::errc());
            return String(buf, r.ptr - buf);
        } else { // SourceT=std::monostate|VariantArray|VariantMap
            return String();
        }
    }
};

// TargetT=VariantArray
template<>
class ConvertToVariantVisitor<VariantArray> {
public:
    VariantArray operator()(const VariantArray& val) const {
        return val;
    }

    // SourceT=std::monostate|bool|int|int64_t|double|String|VariantMap
    template<typename SourceT>
    VariantArray operator()(const SourceT& val) const {
        return VariantArray();
    }
};

// TargetT=VariantMap
template<>
class ConvertToVariantVisitor<VariantMap> {
public:
    VariantMap operator()(const VariantMap& val) const {
        return val;
    }

    // SourceT=std::monostate|bool|int|int64_t|double|String|VariantArray
    template<typename SourceT>
    VariantMap operator()(const SourceT& val) const {
        return VariantMap();
    }
};

template<typename TargetT, typename EnableT = void>
class IsConvertibleVariantVisitor {
public:
    template<typename SourceT>
    bool operator()(const SourceT& val) const {
        return false;
    }
};

// TargetT=std::monostate
template<>
class IsConvertibleVariantVisitor<std::monostate> {
public:
    bool operator()(const std::monostate&) const {
        return true;
    }

    // SourceT=bool|int|int64_t|double|String|VariantArray|VariantMap
    template<typename SourceT>
    bool operator()(const SourceT& val) const {
        return false;
    }
};

// TargetT=bool
template<>
class IsConvertibleVariantVisitor<bool> {
public:
    bool operator()(const String& val) const {
        return val == "true" || val == "false";
    }

    template<typename SourceT>
    bool operator()(const SourceT& val) const {
        if constexpr (std::is_arithmetic_v<SourceT>) { // SourceT=bool|int|int64_t|double
            return true;
        } else { // SourceT=std::monostate|VariantArray|VariantMap
            return false;
        }
    }
};

// TargetT=int|int64_t|double
template<typename TargetT>
class IsConvertibleVariantVisitor<TargetT, std::enable_if_t<std::is_arithmetic_v<TargetT>>> {
public:
    bool operator()(const String& val) const {
        TargetT v = TargetT();
        auto end = val.c_str() + val.length();
        auto r = std::from_chars(val.c_str(), end, v);
        return r.ec == std::errc() && r.ptr == end;
    }

    template<typename SourceT>
    bool operator()(const SourceT& val) const {
        if constexpr (std::is_arithmetic_v<SourceT>) { // SourceT=bool|int|int64_t|double
            return true; // TODO: Check the range of TargetT
        } else { // SourceT=std::monostate|VariantArray|VariantMap
            return false;
        }
    }
};

// TargetT=String
template<>
class IsConvertibleVariantVisitor<String> {
public:
    bool operator()(const String& val) const {
        return true;
    }

    template<typename SourceT>
    bool operator()(const SourceT& val) const {
        if constexpr (std::is_arithmetic_v<SourceT>) { // SourceT=bool|int|int64_t|double
            return true;
        } else { // SourceT=std::monostate|VariantArray|VariantMap
            return false;
        }
    }
};

// TargetT=VariantArray
template<>
class IsConvertibleVariantVisitor<VariantArray> {
public:
    bool operator()(const VariantArray& val) const {
        return true;
    }

    // SourceT=std::monostate|bool|int|int64_t|double|String|VariantMap
    template<typename SourceT>
    bool operator()(const SourceT& val) const {
        return false;
    }
};

// TargetT=VariantMap
template<>
class IsConvertibleVariantVisitor<VariantMap> {
public:
    bool operator()(const VariantMap& val) const {
        return true;
    }

    // SourceT=std::monostate|bool|int|int64_t|double|String|VariantArray
    template<typename SourceT>
    bool operator()(const SourceT& val) const {
        return false;
    }
};

} // namespace detail

template<typename T>
inline T Variant::to() const {
    return std::visit(detail::ConvertToVariantVisitor<T>(), v_);
}

template<typename T>
inline bool Variant::isConvertibleTo() const {
    return std::visit(detail::IsConvertibleVariantVisitor<T>(), v_);
}

} // namespace particle
