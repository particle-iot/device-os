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

#include <limits>
#include <cstdio>
#include <cstdlib>
#include <cctype>
#include <cerrno>

#include "spark_wiring_variant.h"

#include "spark_wiring_json.h"
#include "spark_wiring_print.h"

namespace particle {

namespace {

bool variantFromJSON(const JSONValue& val, Variant& var) {
    switch (val.type()) {
    case JSONType::JSON_TYPE_INVALID: {
        return false;
    }
    case JSONType::JSON_TYPE_NULL: {
        var = Variant();
        break;
    }
    case JSONType::JSON_TYPE_BOOL: {
        var = val.toBool();
        break;
    }
    case JSONType::JSON_TYPE_NUMBER: {
        // Internally, JSONValue stores a numeric value as a pointer to its original string representation
        // so conversion to a string is cheap
        JSONString s = val.toString();
        // Try parsing as int
        int num = 0;
        auto r = detail::from_chars(s.data(), s.data() + s.size(), num);
        if (r.ec != std::errc() || r.ptr != s.data() + s.size()) {
            // Parse as double
            double num = 0;
            r = detail::from_chars(s.data(), s.data() + s.size(), num);
            if (r.ec != std::errc() || r.ptr != s.data() + s.size()) {
                return false;
            }
            var = num;
        } else {
            var = num;
        }
        break;
    }
    case JSONType::JSON_TYPE_STRING: {
        JSONString jsonStr = val.toString();
        String s(jsonStr);
        if (s.length() != jsonStr.size()) {
            return false;
        }
        var = std::move(s);
        break;
    }
    case JSONType::JSON_TYPE_ARRAY: {
        JSONArrayIterator it(val);
        auto& arr = var.asArray();
        if (!arr.reserve(it.count())) {
            return false;
        }
        while (it.next()) {
            Variant v;
            if (!variantFromJSON(it.value(), v)) {
                return false;
            }
            arr.append(std::move(v));
        }
        break;
    }
    case JSONType::JSON_TYPE_OBJECT: {
        JSONObjectIterator it(val);
        auto& map = var.asMap();
        if (!map.reserve(it.count())) {
            return false;
        }
        while (it.next()) {
            JSONString jsonKey = it.name();
            String k(jsonKey);
            if (k.length() != jsonKey.size()) {
                return false;
            }
            Variant v;
            if (!variantFromJSON(it.value(), v)) {
                return false;
            }
            map.set(std::move(k), std::move(v));
        }
        break;
    }
    default:
        return false;
    }
    return true;
}

} // namespace

namespace detail {

#if !defined(__cpp_lib_to_chars) || defined(UNIT_TEST)

std::to_chars_result to_chars(char* first, char* last, double value) {
    std::to_chars_result res;
    int n = std::snprintf(first, last - first, "%g", value);
    if (n < 0 || n >= last - first) {
        res.ec = std::errc::value_too_large;
        res.ptr = last;
    } else {
        res.ec = std::errc();
        res.ptr = first + n;
    }
    return res;
}

std::from_chars_result from_chars(const char* first, const char* last, double& value) {
    std::from_chars_result res;
    if (last > first) {
        char* end = nullptr;
        errno = 0;
        double v = strtod(first, &end);
        if (errno == ERANGE) {
            res.ec = std::errc::result_out_of_range;
            res.ptr = end;
        } else if (end == first || std::isspace((unsigned char)*first)) {
            res.ec = std::errc::invalid_argument;
            res.ptr = first;
        } else {
            res.ec = std::errc();
            res.ptr = end;
            value = v;
        }
    } else {
        res.ec = std::errc::invalid_argument;
        res.ptr = first;
    }
    return res;
}

#endif // !defined(__cpp_lib_to_chars) || defined(UNIT_TEST)

} // namespace detail

bool Variant::append(Variant val) {
    auto& arr = asArray();
    if (!ensureCapacity(arr, 1)) {
        return false;
    }
    return arr.append(std::move(val));
}

bool Variant::prepend(Variant val) {
    auto& arr = asArray();
    if (!ensureCapacity(arr, 1)) {
        return false;
    }
    return arr.prepend(std::move(val));
}

Variant Variant::at(int index) const {
    if (!isArray()) {
        return Variant();
    }
    auto& arr = value<VariantArray>();
    if (index < 0 || index >= arr.size()) {
        return Variant();
    }
    return arr.at(index);
}

bool Variant::set(const char* key, Variant val) {
    auto& map = asMap();
    if (!ensureCapacity(map, 1)) {
        return false;
    }
    return map.set(key, std::move(val));
}

bool Variant::set(const String& key, Variant val) {
    auto& map = asMap();
    if (!ensureCapacity(map, 1)) {
        return false;
    }
    return map.set(key, std::move(val));
}

bool Variant::set(String&& key, Variant val) {
    auto& map = asMap();
    if (!ensureCapacity(map, 1)) {
        return false;
    }
    return map.set(std::move(key), std::move(val));
}

Variant Variant::get(const char* key) const {
    if (!isMap()) {
        return Variant();
    }
    return value<VariantMap>().get(key);
}

Variant Variant::get(const String& key) const {
    if (!isMap()) {
        return Variant();
    }
    return value<VariantMap>().get(key);
}

bool Variant::has(const char* key) const {
    if (!isMap()) {
        return false;
    }
    return value<VariantMap>().has(key);
}

bool Variant::has(const String& key) const {
    if (!isMap()) {
        return false;
    }
    return value<VariantMap>().has(key);
}

int Variant::size() const {
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

bool Variant::isEmpty() const {
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

String Variant::toJSON() const {
    String s;
    OutputStringStream stream(s);
    stream.print(*this);
    if (stream.getWriteError()) {
        return String();
    }
    return s;
}

Variant Variant::fromJSON(const char* json) {
    return fromJSON(JSONValue::parseCopy(json));
}

Variant Variant::fromJSON(const JSONValue& val) {
    Variant v;
    if (!variantFromJSON(val, v)) {
        return Variant();
    }
    return v;
}

} // namespace particle
