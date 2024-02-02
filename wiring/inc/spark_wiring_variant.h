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
#include "spark_wiring_vector.h"
#include "spark_wiring_map.h"

#include "debug.h"

class Stream;
class Print;

namespace spark {

class JSONValue;

} // namespace spark

namespace particle {

using spark::JSONValue;

class Variant;

/**
 * An array of `Variant` values.
 */
typedef Vector<Variant> VariantArray;

/**
 * A map of named `Variant` values.
 */
typedef Map<String, Variant> VariantMap;

namespace detail {

template<typename T>
struct IsComparableWithVariant {
    // TODO: This is not ideal as we'd like Variant to be comparable with any type as long as it's
    // comparable with one of the Variant's alternative types
    static constexpr bool value = std::is_same_v<T, std::monostate> || std::is_arithmetic_v<T> || std::is_same_v<T, const char*> ||
            std::is_same_v<T, String> || std::is_same_v<T, VariantArray> || std::is_same_v<T, VariantMap>;
};

} // namespace detail

/**
 * A class acting as a union for certain common data types.
 */
class Variant {
public:
    /**
     * Supported alternative types.
     */
    enum Type {
        NULL_, ///< Null type (`std::monostate`).
        BOOL, ///< `bool`.
        INT, ///< `int`.
        UINT, ///< `unsigned`.
        INT64, ///< `int64_t`.
        UINT64, ///< `uint64_t`.
        DOUBLE, ///< `double`.
        STRING, ///< `String`.
        ARRAY, ///< `VariantArray`.
        MAP ///< `VariantMap`.
    };

    /**
     * Construct a null variant.
     */
    Variant() = default;

    ///@{
    /**
     * Construct a variant with a value.
     *
     * @param val Value.
     */
    Variant(const std::monostate& val) :
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
#ifdef __LP64__
    Variant(long val) :
            v_(static_cast<int64_t>(val)) {
    }

    Variant(unsigned long val) :
            v_(static_cast<uint64_t>(val)) {
    }
#else
    Variant(long val) :
            v_(static_cast<int>(val)) {
    }

    Variant(unsigned long val) :
            v_(static_cast<unsigned>(val)) {
    }
#endif
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
    ///@}

    /**
     * Copy constructor.
     *
     * @param var Variant to copy.
     */
    Variant(const Variant& var) :
            v_(var.v_) {
    }

    /**
     * Move constructor.
     *
     * @param var Variant to move from.
     */
    Variant(Variant&& var) :
            Variant() {
        swap(*this, var);
    }

    /**
     * Get the type of the stored value.
     *
     * @return Value type.
     */
    Type type() const {
        return static_cast<Type>(v_.index());
    }

    ///@{
    /**
     * Check if the stored value is of the specified type.
     *
     * @return `true` if the value is of the specified type, otherwise `false`.
     */
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
    ///@}

    /**
     * Check if the stored value is of the specified type.
     *
     * The type `T` must be one of the supported alternative types (see the `Type` enum).
     *
     * @tparam T Value type.
     * @return `true` if the value is of the specified type, otherwise `false`.
     */
    template<typename T>
    bool is() const {
        static_assert(IsAlternativeType<T>::value, "The type specified is not one of the alternative types of Variant");
        return std::holds_alternative<T>(v_);
    }

    ///@{
    /**
     * Convert the stored value to a value of the specified type.
     *
     * See the description of the method `to(bool&)` for details on how the conversion is performed.
     *
     * @return Converted value.
     */
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
    ///@}

    ///@{
    /**
     * Convert the stored value to a value of the specified type.
     *
     * See the description of the method `to(bool&)` for details on how the conversion is performed.
     *
     * @param[out] ok Set to `true` if a conversion is defined for the current and target types,
     *         or to `false` otherwise.
     * @return Converted value.
     */
    bool toBool(bool& ok) const {
        return to<bool>(ok);
    }

    int toInt(bool& ok) const {
        return to<int>(ok);
    }

    unsigned toUInt(bool& ok) const {
        return to<unsigned>(ok);
    }

    int64_t toInt64(bool& ok) const {
        return to<int64_t>(ok);
    }

    uint64_t toUInt64(bool& ok) const {
        return to<uint64_t>(ok);
    }

    double toDouble(bool& ok) const {
        return to<double>(ok);
    }

    String toString(bool& ok) const {
        return to<String>(ok);
    }

    VariantArray toArray(bool& ok) const {
        return to<VariantArray>(ok);
    }

    VariantMap toMap(bool& ok) const {
        return to<VariantMap>(ok);
    }
    ///@}

    /**
     * Convert the stored value to a value of the specified type.
     *
     * See the description of the method `to(bool&)` for details on how the conversion is performed.
     *
     * @tparam T Target type.
     * @return Converted value.
     */
    template<typename T>
    T to() const {
        return std::visit(ConvertToVisitor<T>(), v_);
    }

    /**
     * Convert the stored value to a value of the specified type.
     *
     * The conversion is performed as follows:
     *
     * - If the type of the stored value is the same as the target type, a copy of the stored value
     *   is returned. To avoid copying, one of the methods that return a reference to the stored
     *   value can be used instead.
     *
     * - If the current and target types are numeric (or boolean), the conversion is performed as
     *   if `static_cast` is used. It is not checked whether the current value is within the range
     *   of the target type.
     *
     * - If one of the types is `String` and the other type is numeric (or boolean), a conversion
     *   to or from string is performed. When converted to a string, boolean values are represented
     *   as "true" or "false".
     *
     * - For the null, array and map types, only a trivial conversion to the same type is defined.
     *
     * - If no conversion is defined for the current and target types, a default-constructed value
     *   of the target type is returned.
     *
     * @param[out] ok Set to `true` if a conversion is defined for the current and target types,
     *         or to `false` otherwise.
     * @return Converted value.
     */
    template<typename T>
    T to(bool& ok) const {
        ConvertToVisitor<T> vis;
        T val = std::visit(vis, v_);
        ok = vis.ok;
        return val;
    }

    ///@{
    /**
     * Convert the stored value to a value of the specified type in place and get a reference to
     * the stored value.
     *
     * @return Reference to the stored value.
     */
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
    ///@}

    /**
     * Convert the stored value to a value of the specified type in place and get a reference to
     * the stored value.
     *
     * The type `T` must be one of the supported alternative types (see the `Type` enum).
     *
     * @tparam T Target type.
     * @return Reference to the stored value.
     */
    template<typename T>
    T& as() {
        static_assert(IsAlternativeType<T>::value, "The type specified is not one of the alternative types of Variant");
        if (!is<T>()) {
            v_ = to<T>();
        }
        return value<T>();
    }

    ///@{
    /**
     * Get a reference to the stored value.
     *
     * The type `T` must be one of the supported alternative types (see the `Type` enum). If the
     * stored value has a different type, the behavior is undefined.
     *
     * @tparam T Target type.
     * @return Reference to the stored value.
     */
    template<typename T>
    T& value() {
        static_assert(IsAlternativeType<T>::value, "The type specified is not one of the alternative types of Variant");
        return std::get<T>(v_);
    }

    template<typename T>
    const T& value() const {
        static_assert(IsAlternativeType<T>::value, "The type specified is not one of the alternative types of Variant");
        return std::get<T>(v_);
    }
    ///@}

    /**
     * Array operations.
     *
     * These methods are provided for convenience and behave similarly to the respective methods of
     * `VariantArray`.
     */
    ///@{

    /**
     * Append an element to the array.
     *
     * If the stored value is not an array, it is converted to an array in place prior to the
     * operation.
     *
     * @param val Element value.
     * @return `true` if the element was added, or `false` on a memory allocation error.
     */
    bool append(Variant val);

    /**
     * Prepend an element to the array.
     *
     * If the stored value is not an array, it is converted to an array in place prior to the
     * operation.
     *
     * @param val Element value.
     * @return `true` if the element was added, or `false` on a memory allocation error.
     */
    bool prepend(Variant val);

    /**
     * Insert an element into the array.
     *
     * If the stored value is not an array, it is converted to an array in place prior to the
     * operation.
     *
     * @param index Index at which to insert the element.
     * @param val Element value.
     * @return `true` if the element was added, or `false` on a memory allocation error.
     */
    bool insertAt(int index, Variant val);

    /**
     * Remove an element from the array.
     *
     * The method has no effect if the stored value is not an array.
     *
     * @param index Element index.
     */
    void removeAt(int index);

    /**
     * Get an element of the array.
     *
     * This method is inefficient if the element value has a complex type, such as `String`, as it
     * returns a copy of the value. Use `operator[](int)` to get a reference to the element value.
     *
     * A null variant is returned if the stored value is not an array.
     *
     * @param index Element index.
     * @return Element value.
     */
    Variant at(int index) const;
    ///@}

    /**
     * Map operations.
     *
     * These methods are provided for convenience and behave similarly to the respective methods of
     * `VariantMap`.
     */
    ///@{

    ///@{
    /**
     * Add or update an element in the map.
     *
     * If the stored value is not a map, it is converted to a map in place prior to the operation.
     *
     * @param key Element key.
     * @param val Element value.
     * @return `true` if the element was added or updated, or `false` on a memory allocation error.
     */
    bool set(const char* key, Variant val);
    bool set(const String& key, Variant val);
    bool set(String&& key, Variant val);
    ///@}

    ///@{
    /**
     * Remove an element from the map.
     *
     * If the stored value is not a map, the method has no effect and returns `false`.
     *
     * @param key Element key.
     * @return `true` if the element was removed, otherwise `false`.
     */
    bool remove(const char* key);
    bool remove(const String& key);
    ///@}

    ///@{
    /**
     * Get the value of an element in the map.
     *
     * This method is inefficient if the element value has a complex type, such as `String`, as it
     * returns a copy of the value. Use `operator[](const char*)` to get a reference to the element
     * value.
     *
     * A null variant is returned if the stored value is not a map or if an element with the given
     * key cannot be found.
     *
     * @param key Element key.
     * @return Element value.
     */
    Variant get(const char* key) const;
    Variant get(const String& key) const;
    ///@}

    ///@{
    /**
     * Check if the map contains an element with the given key.
     *
     * `false` is returned if the stored value is not a map.
     *
     * @param key Element key.
     * @return `true` if an element with the given key is found, otherwise `false`.
     */
    bool has(const char* key) const;
    bool has(const String& key) const;
    ///@}
    ///@}

    /**
     * Get the size of the stored value.
     *
     * Depending on the type of the stored value:
     *
     * - If `String`, the length of the string is returned.
     *
     * - If `VariantArray` or `VariantMap`, the number of elements stored in the respective
     *   container is returned.
     *
     * - In all other cases 0 is returned.
     *
     * @return Size of the stored value.
     */
    int size() const;

    /**
     * Check if the stored value is empty.
     *
     * Depending on the type of the stored value:
     *
     * - If `String`, `true` is returned if the string is empty.
     *
     * - If `VariantArray` or `VariantMap`, `true` is returned if the respective container is empty.
     *
     * - If the value is null, `true` is returned.
     *
     * - In all other cases `false` is returned.
     *
     * @return `true` if the stored value is empty, otherwise `false`.
     */
    bool isEmpty() const;

    /**
     * Convert the variant to JSON.
     *
     * @return JSON document.
     */
    String toJSON() const;

    ///@{
    /**
     * Get a reference to an element in the array.
     *
     * @note The device will panic if the stored value is not an array.
     *
     * @param index Element index.
     * @return Reference to the element value.
     */
    Variant& operator[](int index) {
        SPARK_ASSERT(isArray());
        return value<VariantArray>().at(index);
    }

    const Variant& operator[](int index) const {
        SPARK_ASSERT(isArray());
        return value<VariantArray>().at(index);
    }
    ///@}

    ///@{
    /**
     * Get a reference to the value of an element in the map.
     *
     * If the stored value is not a map, it is converted to a map in place prior to the operation.
     *
     * If an element with the given key cannot be found, it is created and added to the map.
     *
     * @note The device will panic if it fails to allocate memory for the new element. Use `set()`
     * or the methods provided by `VariantMap` if you need more control over how memory allocation
     * errors are handled.
     *
     * @param key Element key.
     * @return Reference to the element value.
     */
    Variant& operator[](const char* key) {
        return asMap().operator[](key);
    }

    Variant& operator[](const String& key) {
        return asMap().operator[](key);
    }

    Variant& operator[](String&& key) {
        return asMap().operator[](std::move(key));
    }
    ///@}

    /**
     * Assignment operator.
     *
     * @param var Variant to assign from.
     * @return This variant.
     */
    Variant& operator=(Variant var) {
        swap(*this, var);
        return *this;
    }

    /**
     * Comparison operators.
     *
     * Two variants are considered equal if their stored values are equal. Standard C++ type
     * promotion rules are used when comparing numeric values.
     */
    ///@{
    bool operator==(const Variant& var) const {
        return std::visit(AreEqualVisitor(), v_, var.v_);
    }

    bool operator!=(const Variant& var) const {
        return !operator==(var);
    }

    template<typename T, typename std::enable_if_t<detail::IsComparableWithVariant<T>::value, int> = 0>
    bool operator==(const T& val) const {
        return std::visit(IsEqualToVisitor(val), v_);
    }

    template<typename T, typename std::enable_if_t<detail::IsComparableWithVariant<T>::value, int> = 0>
    bool operator!=(const T& val) const {
        return !operator==(val);
    }
    ///@}

    /**
     * Parse a variant from JSON.
     *
     * @param json JSON document.
     * @return Variant.
     */
    static Variant fromJSON(const char* json);

    /**
     * Convert a JSON value to a variant.
     *
     * @param val JSON value.
     * @return Variant.
     */
    static Variant fromJSON(const JSONValue& val);

    friend void swap(Variant& var1, Variant& var2) {
        using std::swap; // For ADL
        swap(var1.v_, var2.v_);
    }

private:
    template<typename TargetT, typename EnableT = void>
    struct ConvertToVisitor {
        bool ok = false;

        template<typename SourceT>
        TargetT operator()(const SourceT& val) {
            return TargetT();
        }
    };

    // Compares a Variant with a value
    template<typename FirstT>
    struct IsEqualToVisitor {
        const FirstT& first;

        explicit IsEqualToVisitor(const FirstT& first) :
                first(first) {
        }

        template<typename SecondT>
        bool operator()(const SecondT& second) const {
            return areEqual(first, second);
        }
    };

    // Compares two Variants
    struct AreEqualVisitor {
        template<typename FirstT, typename SecondT>
        bool operator()(const FirstT& first, const SecondT& second) const {
            return areEqual(first, second);
        }
    };

    template<typename T>
    struct IsAlternativeType {
        static constexpr bool value = false;
    };

    typedef std::variant<std::monostate, bool, int, unsigned, int64_t, uint64_t, double, String, VariantArray, VariantMap> VariantType;

    VariantType v_;

    template<typename FirstT, typename SecondT>
    static bool areEqual(const FirstT& first, const SecondT& second) {
        // TODO: Use std::equality_comparable_with (requires C++20)
        if constexpr (std::is_same_v<FirstT, SecondT> ||
                (std::is_arithmetic_v<FirstT> && std::is_arithmetic_v<SecondT>) ||
                (std::is_same_v<FirstT, String> && std::is_same_v<SecondT, const char*>) ||
                (std::is_same_v<FirstT, const char*> && std::is_same_v<SecondT, String>)) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-compare"
            return first == second;
#pragma GCC diagnostic pop
        } else {
            return false;
        }
    }

    template<typename ContainerT>
    static bool ensureCapacity(ContainerT& cont, int count) {
        int newSize = cont.size() + count;
        if (cont.capacity() >= newSize) {
            return true;
        }
        return cont.reserve(std::max(newSize, cont.capacity() * 3 / 2));
    }
};

namespace detail {

// As of GCC 10, std::to_chars and std::from_chars don't support floating point types. Note that the
// substitution functions below behave differently from the standard ones. They're tailored for the
// needs of the Variant class and are only defined so that we can quickly drop them when <charconv>
// API is fully supported by the current version of GCC
#if !defined(__cpp_lib_to_chars) || defined(UNIT_TEST)

std::to_chars_result to_chars(char* first, char* last, double value);
std::from_chars_result from_chars(const char* first, const char* last, double& value);

#endif // !defined(__cpp_lib_to_chars) || defined(UNIT_TEST)

template<typename T>
inline std::to_chars_result to_chars(char* first, char* last, const T& value) {
    return std::to_chars(first, last, value);
}

template<typename T>
inline std::from_chars_result from_chars(const char* first, const char* last, T& value) {
    return std::from_chars(first, last, value);
}

} // namespace detail

template<>
struct Variant::ConvertToVisitor<std::monostate> {
    bool ok = false;

    std::monostate operator()(const std::monostate&) {
        ok = true;
        return std::monostate();
    }

    template<typename SourceT>
    std::monostate operator()(const SourceT& val) {
        return std::monostate();
    }
};

template<>
class Variant::ConvertToVisitor<bool> {
public:
    bool ok = false;

    bool operator()(const String& val) {
        if (val == "true") {
            ok = true;
            return true;
        }
        if (val == "false") {
            ok = true;
            return false;
        }
        return false;
    }

    template<typename SourceT>
    bool operator()(const SourceT& val) {
        if constexpr (std::is_arithmetic_v<SourceT>) {
            ok = true;
            return static_cast<bool>(val);
        } else { // std::monostate, VariantArray, VariantMap
            return false;
        }
    }
};

template<typename TargetT>
struct Variant::ConvertToVisitor<TargetT, std::enable_if_t<std::is_arithmetic_v<TargetT>>> {
    bool ok = false;

    TargetT operator()(const String& val) {
        TargetT v;
        auto end = val.c_str() + val.length();
        auto r = detail::from_chars(val.c_str(), end, v);
        if (r.ec != std::errc() || r.ptr != end) {
            return TargetT();
        }
        ok = true;
        return v;
    }

    template<typename SourceT>
    TargetT operator()(const SourceT& val) {
        if constexpr (std::is_arithmetic_v<SourceT>) {
            ok = true;
            return static_cast<TargetT>(val);
        } else { // std::monostate, VariantArray, VariantMap
            return TargetT();
        }
    }
};

template<>
struct Variant::ConvertToVisitor<String> {
    bool ok = false;

    String operator()(bool val) {
        ok = true;
        return val ? "true" : "false";
    }

    String operator()(const String& val) {
        ok = true;
        return val;
    }

    template<typename SourceT>
    String operator()(const SourceT& val) {
        if constexpr (std::is_arithmetic_v<SourceT>) {
            char buf[32]; // Large enough for all relevant types
            auto r = detail::to_chars(buf, buf + sizeof(buf), val);
            SPARK_ASSERT(r.ec == std::errc());
            ok = true;
            return String(buf, r.ptr - buf);
        } else { // std::monostate, VariantArray, VariantMap
            return String();
        }
    }
};

template<>
struct Variant::ConvertToVisitor<VariantArray> {
    bool ok = false;

    VariantArray operator()(const VariantArray& val) {
        ok = true;
        return val;
    }

    template<typename SourceT>
    VariantArray operator()(const SourceT& val) {
        return VariantArray();
    }
};

template<>
struct Variant::ConvertToVisitor<VariantMap> {
    bool ok = false;

    VariantMap operator()(const VariantMap& val) {
        ok = true;
        return val;
    }

    template<typename SourceT>
    VariantMap operator()(const SourceT& val) const {
        return VariantMap();
    }
};

template<>
struct Variant::IsAlternativeType<std::monostate> {
    static const bool value = true;
};

template<>
struct Variant::IsAlternativeType<bool> {
    static const bool value = true;
};

template<>
struct Variant::IsAlternativeType<int> {
    static const bool value = true;
};

template<>
struct Variant::IsAlternativeType<unsigned> {
    static const bool value = true;
};

template<>
struct Variant::IsAlternativeType<int64_t> {
    static const bool value = true;
};

template<>
struct Variant::IsAlternativeType<uint64_t> {
    static const bool value = true;
};

template<>
struct Variant::IsAlternativeType<double> {
    static const bool value = true;
};

template<>
struct Variant::IsAlternativeType<String> {
    static const bool value = true;
};

template<>
struct Variant::IsAlternativeType<VariantArray> {
    static const bool value = true;
};

template<>
struct Variant::IsAlternativeType<VariantMap> {
    static const bool value = true;
};

template<typename T, typename std::enable_if_t<detail::IsComparableWithVariant<T>::value, int> = 0>
inline bool operator==(const T& val, const Variant& var) {
    return var == val;
}

template<typename T, typename std::enable_if_t<detail::IsComparableWithVariant<T>::value, int> = 0>
inline bool operator!=(const T& val, const Variant& var) {
    return var != val;
}

/**
 * Encode a variant to CBOR.
 *
 * @param var Variant.
 * @param stream Output stream.
 * @return 0 on success, otherwise an error code defined by `Error::Type`.
 */
int encodeToCBOR(const Variant& var, Print& stream);

/**
 * Decode a variant from CBOR.
 *
 * @param[out] var Variant.
 * @param stream Input stream.
 * @return 0 on success, otherwise an error code defined by `Error::Type`.
 */
int decodeFromCBOR(Variant& var, Stream& stream);

} // namespace particle
