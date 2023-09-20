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
#include <cmath>
#include <cstring>
#include <cctype>
#include <cerrno>

#include "spark_wiring_variant.h"

#include "spark_wiring_json.h"
#include "spark_wiring_stream.h"
#include "spark_wiring_error.h"

#include "endian_util.h"
#include "check.h"

namespace particle {

namespace {

class DecodingStream {
public:
    explicit DecodingStream(Stream& stream) :
            stream_(stream) {
    }

    int readUint8(uint8_t& val) {
        CHECK(read((char*)&val, sizeof(val)));
        return 0;
    }

    int readUint16Be(uint16_t& val) {
        CHECK(read((char*)&val, sizeof(val)));
        val = bigEndianToNative(val);
        return 0;
    }

    int readUint32Be(uint32_t& val) {
        CHECK(read((char*)&val, sizeof(val)));
        val = bigEndianToNative(val);
        return 0;
    }

    int readUint64Be(uint64_t& val) {
        CHECK(read((char*)&val, sizeof(val)));
        val = bigEndianToNative(val);
        return 0;
    }

    int read(char* data, size_t size) {
        size_t n = stream_.readBytes(data, size);
        if (n != size) {
            return Error::IO;
        }
        return 0;
    }

private:
    Stream& stream_;
};

class EncodingStream {
public:
    explicit EncodingStream(Print& stream) :
            stream_(stream) {
    }

    int writeUint8(uint8_t val) {
        CHECK(write((const char*)&val, sizeof(val)));
        return 0;
    }

    int writeUint16Be(uint16_t val) {
        val = nativeToBigEndian(val);
        CHECK(write((const char*)&val, sizeof(val)));
        return 0;
    }

    int writeUint32Be(uint32_t val) {
        val = nativeToBigEndian(val);
        CHECK(write((const char*)&val, sizeof(val)));
        return 0;
    }

    int writeUint64Be(uint64_t val) {
        val = nativeToBigEndian(val);
        CHECK(write((const char*)&val, sizeof(val)));
        return 0;
    }

    int writeFloatBe(float val) {
        uint32_t v;
        static_assert(sizeof(v) == sizeof(val));
        std::memcpy(&v, &val, sizeof(val));
        v = nativeToBigEndian(v);
        CHECK(write((const char*)&v, sizeof(v)));
        return 0;
    }

    int writeDoubleBe(double val) {
        uint64_t v;
        static_assert(sizeof(v) == sizeof(val));
        std::memcpy(&v, &val, sizeof(val));
        v = nativeToBigEndian(v);
        CHECK(write((const char*)&v, sizeof(v)));
        return 0;
    }

    int write(const char* data, size_t size) {
        size_t n = stream_.write((const uint8_t*)data, size);
        if (n != size) {
            int err = stream_.getWriteError();
            return (err < 0) ? err : Error::IO;
        }
        return 0;
    }

private:
    Print& stream_;
};

struct CborHead {
    uint64_t arg;
    int type;
    int detail;
};

int readAndAppendToString(DecodingStream& stream, size_t size, String& str) {
    if (!str.reserve(str.length() + size)) {
        return Error::NO_MEMORY;
    }
    char buf[128];
    while (size > 0) {
        size_t n = std::min(size, sizeof(buf));
        CHECK(stream.read(buf, n));
        str.concat(buf, n);
        size -= n;
    }
    return 0;
}

int readCborHead(DecodingStream& stream, CborHead& head) {
    uint8_t b;
    CHECK(stream.readUint8(b));
    head.type = b >> 5;
    head.detail = b & 0x1f;
    if (head.detail < 24) {
        head.arg = head.detail;
    } else {
        switch (head.detail) {
        case 24: { // 1-byte argument
            uint8_t v;
            CHECK(stream.readUint8(v));
            head.arg = v;
            break;
        }
        case 25: { // 2-byte argument
            uint16_t v;
            CHECK(stream.readUint16Be(v));
            head.arg = v;
            break;
        }
        case 26: { // 4-byte argument
            uint32_t v;
            CHECK(stream.readUint32Be(v));
            head.arg = v;
            break;
        }
        case 27: { // 8-byte argument
            CHECK(stream.readUint64Be(head.arg));
            break;
        }
        case 31: { // Indefinite length indicator or stop code
            if (head.type == 0 /* Unsigned integer */ || head.type == 1 /* Negative integer */ || head.type == 6 /* Tagged item */) {
                return Error::BAD_DATA;
            }
            head.arg = 0;
            break;
        }
        default: // Reserved (28-30)
            return Error::BAD_DATA;
        }
    }
    return 0;
}

int writeCborHeadWithArgument(EncodingStream& stream, int type, uint64_t arg) {
    type <<= 5;
    if (arg < 24) {
        CHECK(stream.writeUint8(arg | type));
    } else if (arg <= 0xff) {
        CHECK(stream.writeUint8(24 /* 1-byte argument */ | type));
        CHECK(stream.writeUint8(arg));
    } else if (arg <= 0xffff) {
        CHECK(stream.writeUint8(25 /* 2-byte argument */ | type));
        CHECK(stream.writeUint16Be(arg));
    } else if (arg <= 0xffffffffu) {
        CHECK(stream.writeUint8(26 /* 4-byte argument */ | type));
        CHECK(stream.writeUint32Be(arg));
    } else {
        CHECK(stream.writeUint8(27 /* 8-byte argument */ | type));
        CHECK(stream.writeUint64Be(arg));
    }
    return 0;
}

int writeCborUnsignedInteger(EncodingStream& stream, uint64_t val) {
    CHECK(writeCborHeadWithArgument(stream, 0 /* Unsigned integer */, val));
    return 0;
}

int writeCborSignedInteger(EncodingStream& stream, int64_t val) {
    if (val < 0) {
        val = -(val + 1);
        CHECK(writeCborHeadWithArgument(stream, 1 /* Negative integer */, val));
    } else {
        CHECK(writeCborHeadWithArgument(stream, 0 /* Unsigned integer */, val));
    }
    return 0;
}

int readCborString(DecodingStream& stream, const CborHead& head, String& str) {
    String s;
    if (head.detail == 31 /* Indefinite length */) {
        for (;;) {
            CborHead h;
            CHECK(readCborHead(stream, h));
            if (h.type == 7 /* Misc. items */ && h.detail == 31 /* Stop code */) {
                break;
            }
            if (h.type != 3 /* Text string */ || h.detail == 31 /* Indefinite length */) { // Chunks of indefinite length are not permitted
                return Error::BAD_DATA;
            }
            if (h.arg > std::numeric_limits<unsigned>::max()) {
                return Error::OUT_OF_RANGE;
            }
            CHECK(readAndAppendToString(stream, h.arg, s));
        }
    } else {
        if (head.arg > std::numeric_limits<unsigned>::max()) {
            return Error::OUT_OF_RANGE;
        }
        CHECK(readAndAppendToString(stream, head.arg, s));
    }
    str = std::move(s);
    return 0;
}

int writeCborString(EncodingStream& stream, const String& str) {
    CHECK(writeCborHeadWithArgument(stream, 3 /* Text string */, str.length()));
    CHECK(stream.write(str.c_str(), str.length()));
    return 0;
}

int encodeToCbor(EncodingStream& stream, const Variant& var) {
    switch (var.type()) {
    case Variant::NULL_: {
        CHECK(stream.writeUint8(0xf6 /* null */)); // See RFC 8949, Appendix B
        break;
    }
    case Variant::BOOL: {
        auto v = var.value<bool>();
        CHECK(stream.writeUint8(v ? 0xf5 /* true */ : 0xf4 /* false */));
        break;
    }
    case Variant::INT: {
        CHECK(writeCborSignedInteger(stream, var.value<int>()));
        break;
    }
    case Variant::UINT: {
        CHECK(writeCborUnsignedInteger(stream, var.value<unsigned>()));
        break;
    }
    case Variant::INT64: {
        CHECK(writeCborSignedInteger(stream, var.value<int64_t>()));
        break;
    }
    case Variant::UINT64: {
        CHECK(writeCborUnsignedInteger(stream, var.value<uint64_t>()));
        break;
    }
    case Variant::DOUBLE: {
        double d = var.value<double>();
        float f = d;
        if (f == d) {
            // Encoding with a smaller precision than that of float is not supported
            CHECK(stream.writeUint8(0xfa /* Single-precision */));
            CHECK(stream.writeFloatBe(f));
        } else {
            CHECK(stream.writeUint8(0xfb /* Double-precision */));
            CHECK(stream.writeDoubleBe(d));
        }
        break;
    }
    case Variant::STRING: {
        CHECK(writeCborString(stream, var.value<String>()));
        break;
    }
    case Variant::ARRAY: {
        auto& arr = var.value<VariantArray>();
        CHECK(writeCborHeadWithArgument(stream, 4 /* Array */, arr.size()));
        for (auto& v: arr) {
            CHECK(encodeToCbor(stream, v));
        }
        break;
    }
    case Variant::MAP: {
        auto& entries = var.value<VariantMap>().entries();
        CHECK(writeCborHeadWithArgument(stream, 5 /* Map */, entries.size()));
        for (auto& e: entries) {
            CHECK(writeCborString(stream, e.first));
            CHECK(encodeToCbor(stream, e.second));
        }
        break;
    }
    default: // Unreachable
        return Error::INTERNAL;
    }
    return 0;
}

int decodeFromCbor(DecodingStream& stream, const CborHead& head, Variant& var) {
    switch (head.type) {
    case 0: { // Unsigned integer
        if (head.arg <= std::numeric_limits<unsigned>::max()) {
            var = (unsigned)head.arg; // 32-bit
        } else {
            var = head.arg; // 64-bit
        }
        break;
    }
    case 1: { // Negative integer
        if (head.arg > (uint64_t)std::numeric_limits<int64_t>::max()) {
            return Error::OUT_OF_RANGE;
        }
        int64_t v = -(int64_t)head.arg - 1;
        if (v >= std::numeric_limits<int>::min()) {
            var = (int)v; // 32-bit
        } else {
            var = v; // 64-bit
        }
        break;
    }
    case 2: { // Byte string
        return Error::NOT_SUPPORTED; // Not supported
    }
    case 3: { // Text string
        String s;
        CHECK(readCborString(stream, head, s));
        var = std::move(s);
        break;
    }
    case 4: { // Array
        VariantArray arr;
        int len = -1;
        if (head.detail != 31 /* Indefinite length */) {
            if (head.arg > (uint64_t)std::numeric_limits<int>::max()) {
                return Error::OUT_OF_RANGE;
            }
            len = head.arg;
            if (!arr.reserve(len)) {
                return Error::NO_MEMORY;
            }
        }
        for (;;) {
            if (len >= 0 && arr.size() == len) {
                break;
            }
            CborHead h;
            CHECK(readCborHead(stream, h));
            if (h.type == 7 /* Misc. items */ && h.detail == 31 /* Stop code */) {
                if (len >= 0) {
                    return Error::BAD_DATA; // Unexpected stop code
                }
                break;
            }
            Variant v;
            CHECK(decodeFromCbor(stream, h, v));
            if (!arr.append(std::move(v))) {
                return Error::NO_MEMORY;
            }
        }
        var = std::move(arr);
        break;
    }
    case 5: { // Map
        VariantMap map;
        int len = -1;
        if (head.detail != 31 /* Indefinite length */) {
            if (head.arg > (uint64_t)std::numeric_limits<int>::max()) {
                return Error::OUT_OF_RANGE;
            }
            len = head.arg;
            if (!map.reserve(len)) {
                return Error::NO_MEMORY;
            }
        }
        for (;;) {
            if (len >= 0 && map.size() == len) {
                break;
            }
            CborHead h;
            CHECK(readCborHead(stream, h));
            if (h.type == 7 /* Misc. items */ && h.detail == 31 /* Stop code */) {
                if (len >= 0) {
                    return Error::BAD_DATA; // Unexpected stop code
                }
                break;
            }
            if (h.type != 3 /* Text string */) {
                return Error::NOT_SUPPORTED; // Non-string keys are not supported
            }
            String k;
            CHECK(readCborString(stream, h, k));
            Variant v;
            CHECK(readCborHead(stream, h));
            CHECK(decodeFromCbor(stream, h, v));
            if (!map.set(std::move(k), std::move(v))) {
                return Error::NO_MEMORY;
            }
        }
        var = std::move(map);
        break;
    }
    case 6: { // Tagged item
        // Skip all tags
        CborHead h;
        do {
            CHECK(readCborHead(stream, h));
        } while (h.type == 6 /* Tagged item */);
        CHECK(decodeFromCbor(stream, h, var));
        break;
    }
    case 7: { // Misc. items
        switch (head.detail) {
        case 20: { // false
            var = false;
            break;
        }
        case 21: { // true
            var = true;
            break;
        }
        case 22: { // null
            var = Variant();
            break;
        }
        case 25: { // Half-precision
            // This code was taken from RFC 8949, Appendix D
            uint16_t half = head.arg;
            unsigned exp = (half >> 10) & 0x1f;
            unsigned mant = half & 0x03ff;
            double val = 0;
            if (exp == 0) {
                val = std::ldexp(mant, -24);
            } else if (exp != 31) {
                val = std::ldexp(mant + 1024, exp - 25);
            } else {
                val = (mant == 0) ? INFINITY : NAN;
            }
            if (half & 0x8000) {
                val = -val;
            }
            var = val;
            break;
        }
        case 26: { // Single-precision
            uint32_t v = head.arg;
            float val;
            static_assert(sizeof(val) == sizeof(v));
            std::memcpy(&val, &v, sizeof(v));
            var = val;
            break;
        }
        case 27: { // Double-precision
            double val;
            static_assert(sizeof(val) == sizeof(head.arg));
            std::memcpy(&val, &head.arg, sizeof(head.arg));
            var = val;
            break;
        }
        default:
            if ((head.detail >= 28 && head.detail <= 31) || // Reserved (28-30) or unexpected stop code (31)
                    (head.detail == 24 && head.arg < 32)) { // Invalid simple value
                return Error::BAD_DATA;
            }
            return Error::NOT_SUPPORTED; // Unassigned simple value (0-19, 32-255) or undefined (23)
        }
        break;
    }
    default: // Unreachable
        return Error::INTERNAL;
    }
    return 0;
}

int decodeFromJson(const JSONValue& val, Variant& var) {
    switch (val.type()) {
    case JSONType::JSON_TYPE_INVALID: {
        return Error::INVALID_ARGUMENT;
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
            return Error::NO_MEMORY;
        }
        var = std::move(s);
        break;
    }
    case JSONType::JSON_TYPE_ARRAY: {
        JSONArrayIterator it(val);
        auto& arr = var.asArray();
        if (!arr.reserve(it.count())) {
            return Error::NO_MEMORY;
        }
        while (it.next()) {
            Variant v;
            CHECK(decodeFromJson(it.value(), v));
            arr.append(std::move(v));
        }
        break;
    }
    case JSONType::JSON_TYPE_OBJECT: {
        JSONObjectIterator it(val);
        auto& map = var.asMap();
        if (!map.reserve(it.count())) {
            return Error::NO_MEMORY;
        }
        while (it.next()) {
            JSONString jsonKey = it.name();
            String k(jsonKey);
            if (k.length() != jsonKey.size()) {
                return Error::NO_MEMORY;
            }
            Variant v;
            CHECK(decodeFromJson(it.value(), v));
            map.set(std::move(k), std::move(v));
        }
        break;
    }
    default: // Unreachable
        return Error::INTERNAL;
    }
    return 0;
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

bool Variant::insertAt(int index, Variant val) {
    auto& arr = asArray();
    if (!ensureCapacity(arr, 1)) {
        return false;
    }
    if (index < 0) {
        index = 0;
    } else if (index > arr.size()) {
        index = arr.size();
    }
    return arr.insert(index, std::move(val));
}

void Variant::removeAt(int index) {
    if (!isArray()) {
        return;
    }
    auto& arr = value<VariantArray>();
    if (index < 0 || index >= arr.size()) {
        return;
    }
    arr.removeAt(index);
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

bool Variant::remove(const char* key) {
    if (!isMap()) {
        return false;
    }
    return value<VariantMap>().remove(key);
}

bool Variant::remove(const String& key) {
    if (!isMap()) {
        return false;
    }
    return value<VariantMap>().remove(key);
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
    int r = decodeFromJson(val, v);
    if (r < 0) {
        return Variant();
    }
    return v;
}

int encodeToCBOR(const Variant& var, Print& stream) {
    EncodingStream s(stream);
    CHECK(encodeToCbor(s, var));
    return 0;
}

int decodeFromCBOR(Variant& var, Stream& stream) {
    DecodingStream s(stream);
    CborHead h;
    CHECK(readCborHead(s, h));
    CHECK(decodeFromCbor(s, h, var));
    return 0;
}

} // namespace particle
