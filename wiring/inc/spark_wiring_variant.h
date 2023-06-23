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
#include <limits>
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
        UINT,
        INT64,
        UINT64,
        DOUBLE,
        STRING,
        ARRAY,
        MAP
    };

    Variant() = default;

    Variant(const std::monostate&) :
            Variant() {
    }

    Variant(bool val) :
            v_(val) {
    }

    Variant(int val) :
            v_(val) {
    }

    Variant(unsigned val) :
            v_(val) {
    }

    Variant(long val) :
            v_(val) {
    }

    Variant(unsigned long val) :
            v_(val) {
    }

    Variant(long long val) :
            v_(val) {
    }

    Variant(unsigned long long val) :
            v_(val) {
    }

    Variant(double val) :
            v_(val) {
    }

    Variant(const char* val) :
            v_(String(val)) {
    }

    Variant(String val) :
            v_(std::move(val)) {
    }

    Variant(VariantArray val) :
            v_(std::move(val)) {
    }

    Variant(VariantMap val) :
            v_(std::move(val)) {
    }

    Variant(const Variant& var) :
            v_(var.v_) {
    }

    Variant(Variant&& var) :
            Variant() {
        swap(*this, var);
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

    bool isUInt() const {
        return is<unsigned>();
    }

    bool isInt64() const {
        return is<int64_t>();
    }

    bool isUInt64() const {
        return is<uint64_t>();
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
                std::is_same_v<T, unsigned> ||
                std::is_same_v<T, int64_t> ||
                std::is_same_v<T, uint64_t> ||
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

    unsigned toUInt() const {
        return to<unsigned>();
    }

    int64_t toInt64() const {
        return to<int64_t>();
    }

    uint64_t toUInt64() const {
        return to<uint64_t>();
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

    unsigned& asUInt() {
        return as<unsigned>();
    }

    int64_t& asInt64() {
        return as<int64_t>();
    }

    uint64_t& asUInt64() {
        return as<uint64_t>();
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
    bool append(Variant val) {
        auto& arr = asArray();
        if (!ensureCapacity(arr, 1)) {
            return false;
        }
        return arr.append(std::move(val));
    }

    bool prepend(Variant val) {
        auto& arr = asArray();
        if (!ensureCapacity(arr, 1)) {
            return false;
        }
        return arr.prepend(std::move(val));
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
    bool set(const char* key, Variant val) {
        auto& map = asMap();
        if (!ensureCapacity(map, 1)) {
            return false;
        }
        return map.set(key, std::move(val));
    }

    bool set(const String& key, Variant val) {
        auto& map = asMap();
        if (!ensureCapacity(map, 1)) {
            return false;
        }
        return map.set(key, std::move(val));
    }

    bool set(String&& key, Variant val) {
        auto& map = asMap();
        if (!ensureCapacity(map, 1)) {
            return false;
        }
        return map.set(std::move(key), std::move(val));
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

    Variant& operator=(Variant var) {
        swap(*this, var);
        return *this;
    }

    bool operator==(const Variant& var) const {
        return v_ == var.v_;
    }

    bool operator!=(const Variant& var) const {
        return !operator==(var);
    }

    bool operator==(const std::monostate&) const {
        return isNull();
    }

    bool operator!=(const std::monostate& val) const {
        return !operator==(val);
    }

    template<typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
    bool operator==(const T& val) const {
        return isConvertibleTo<T>() && to<T>() == val;
    }

    template<typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
    bool operator!=(const T& val) const {
        return !operator==(val);
    }

    bool operator==(const char* val) const {
        return isString() && value<String>() == val;
    }

    bool operator!=(const char* val) const {
        return !operator==(val);
    }

    bool operator==(const String& val) const {
        return isString() && value<String>() == val;
    }

    bool operator!=(const String& val) const {
        return !operator==(val);
    }

    bool operator==(const VariantArray& val) const {
        return isArray() && value<VariantArray>() == val;
    }

    bool operator!=(const VariantArray& val) const {
        return !operator==(val);
    }

    bool operator==(const VariantMap& val) const {
        return isMap() && value<VariantMap>() == val;
    }

    bool operator!=(const VariantMap& val) const {
        return !operator==(val);
    }

    friend void swap(Variant& var1, Variant& var2) {
        using std::swap; // For ADL
        swap(var1.v_, var2.v_);
    }

private:
    std::variant<std::monostate, bool, int, unsigned, int64_t, uint64_t, double, String, VariantArray, VariantMap> v_;

    template<typename ContainerT>
    static bool ensureCapacity(ContainerT& cont, int count) {
        int newSize = cont.size() + count;
        if (cont.capacity() >= newSize) {
            return true;
        }
        return cont.reserve(std::max(cont.capacity() * 3 / 2, newSize));
    }
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

template<>
class ConvertToVariantVisitor<std::monostate> {
public:
    template<typename SourceT>
    std::monostate operator()(const SourceT& val) const {
        return std::monostate();
    }
};

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
        if constexpr (std::is_arithmetic_v<SourceT>) {
            return static_cast<bool>(val);
        } else { // std::monostate, VariantArray, VariantMap
            return false;
        }
    }
};

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
        if constexpr (std::is_arithmetic_v<SourceT>) {
            return static_cast<TargetT>(val);
        } else { // std::monostate, VariantArray, VariantMap
            return TargetT();
        }
    }
};

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
        if constexpr (std::is_arithmetic_v<SourceT>) {
            char buf[32]; // Should be large enough for all relevant types
            auto r = std::to_chars(buf, buf + sizeof(buf), val);
            SPARK_ASSERT(r.ec == std::errc());
            return String(buf, r.ptr - buf);
        } else { // std::monostate, VariantArray, VariantMap
            return String();
        }
    }
};

template<>
class ConvertToVariantVisitor<VariantArray> {
public:
    VariantArray operator()(const VariantArray& val) const {
        return val;
    }

    template<typename SourceT>
    VariantArray operator()(const SourceT& val) const {
        return VariantArray();
    }
};

template<>
class ConvertToVariantVisitor<VariantMap> {
public:
    VariantMap operator()(const VariantMap& val) const {
        return val;
    }

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

template<>
class IsConvertibleVariantVisitor<std::monostate> {
public:
    bool operator()(const std::monostate&) const {
        return true;
    }

    template<typename SourceT>
    bool operator()(const SourceT& val) const {
        return false;
    }
};

template<>
class IsConvertibleVariantVisitor<bool> {
public:
    bool operator()(const String& val) const {
        return val == "true" || val == "false";
    }

    template<typename SourceT>
    bool operator()(const SourceT& val) const {
        if constexpr (std::is_arithmetic_v<SourceT>) {
            return true;
        } else { // std::monostate, VariantArray, VariantMap
            return false;
        }
    }
};

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
        if constexpr (std::is_arithmetic_v<SourceT>) {
            return true; // TODO: Check the range of TargetT
        } else { // std::monostate, VariantArray, VariantMap
            return false;
        }
    }
};

template<>
class IsConvertibleVariantVisitor<String> {
public:
    bool operator()(const String& val) const {
        return true;
    }

    template<typename SourceT>
    bool operator()(const SourceT& val) const {
        if constexpr (std::is_arithmetic_v<SourceT>) {
            return true;
        } else { // std::monostate, VariantArray, VariantMap
            return false;
        }
    }
};

template<>
class IsConvertibleVariantVisitor<VariantArray> {
public:
    bool operator()(const VariantArray& val) const {
        return true;
    }

    template<typename SourceT>
    bool operator()(const SourceT& val) const {
        return false;
    }
};

template<>
class IsConvertibleVariantVisitor<VariantMap> {
public:
    bool operator()(const VariantMap& val) const {
        return true;
    }

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

inline bool operator==(const std::monostate& val, const Variant& var) {
    return var == val;
}

inline bool operator!=(const std::monostate& val, const Variant& var) {
    return var != val;
}

template<typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
inline bool operator==(const T& val, const Variant& var) {
    return var == val;
}

template<typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
inline bool operator!=(const T& val, const Variant& var) {
    return var != val;
}

inline bool operator==(const char* str, const Variant& var) {
    return var == str;
}

inline bool operator!=(const char* str, const Variant& var) {
    return var != str;
}

inline bool operator==(const String& str, const Variant& var) {
    return var == str;
}

inline bool operator!=(const String& str, const Variant& var) {
    return var != str;
}

inline bool operator==(const VariantArray& arr, const Variant& var) {
    return var == arr;
}

inline bool operator!=(const VariantArray& arr, const Variant& var) {
    return var != arr;
}

inline bool operator==(const VariantMap& map, const Variant& var) {
    return var == map;
}

inline bool operator!=(const VariantMap& map, const Variant& var) {
    return var != map;
}

} // namespace particle
