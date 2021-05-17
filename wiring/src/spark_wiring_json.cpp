/*
 * Copyright (c) 2016 Particle Industries, Inc.  All rights reserved.
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

#include "spark_wiring_json.h"

#include <algorithm>

#include <cstdio>
#include <cstdlib>
#include <cstdarg>

namespace {

// Skips token and all its children tokens if any
const jsmntok_t* skipToken(const jsmntok_t *t) {
    size_t n = 1;
    do {
        if (t->type == JSMN_OBJECT) {
            n += t->size * 2; // Number of name and value tokens
        } else if (t->type == JSMN_ARRAY) {
            n += t->size; // Number of value tokens
        }
        ++t;
        --n;
    } while (n);
    return t;
}

bool hexToInt(const char *s, size_t size, uint32_t *val) {
    uint32_t v = 0;
    const char* const end = s + size;
    while (s != end) {
        uint32_t n = 0;
        const char c = *s;
        if (c >= '0' && c <= '9') {
            n = c - '0';
        } else if (c >= 'a' && c <= 'f') {
            n = c - 'a' + 10;
        } else if (c >= 'A' && c <= 'F') {
            n = c - 'A' + 10;
        } else {
            return false; // Error
        }
        v = (v << 4) | n;
        ++s;
    }
    *val = v;
    return true;
}

} // namespace

// spark::detail::JSONData
struct spark::detail::JSONData {
    jsmntok_t *tokens;
    char *json;
    bool freeJson;

    JSONData() :
            tokens(nullptr),
            json(nullptr),
            freeJson(false) {
    }

    ~JSONData() {
        delete[] tokens;
        if (freeJson) {
            delete[] json;
        }
    }
};

// spark::JSONValue
spark::JSONValue::JSONValue(const jsmntok_t *t, detail::JSONDataPtr d) :
        JSONValue() {
    if (t) {
        t_ = t;
        d_ = d;
    }
}

bool spark::JSONValue::toBool() const {
    switch (type()) {
    case JSON_TYPE_BOOL: {
        const char* const s = d_->json + t_->start;
        return *s == 't';
    }
    case JSON_TYPE_NUMBER: {
        const char* const s = d_->json + t_->start;
        return strcmp(s, "0") != 0 && strcmp(s, "0.0") != 0;
    }
    case JSON_TYPE_STRING: {
        const char* const s = d_->json + t_->start;
        if (*s == '\0' || strcmp(s, "false") == 0 || strcmp(s, "0") == 0 || strcmp(s, "0.0") == 0) {
            return false; // Empty string, "false", "0" or "0.0"
        }
        return true; // Any other string
    }
    default:
        return false;
    }
}

int spark::JSONValue::toInt() const {
    switch (type()) {
    case JSON_TYPE_BOOL: {
        const char* const s = d_->json + t_->start;
        return *s == 't';
    }
    case JSON_TYPE_NUMBER:
    case JSON_TYPE_STRING: {
        // toInt() may produce incorrect results for floating point numbers, since we want to keep
        // compile-time dependency on strtod() optional
        const char* const s = d_->json + t_->start;
        return strtol(s, nullptr, 10);
    }
    default:
        return 0;
    }
}

double spark::JSONValue::toDouble() const {
    switch (type()) {
    case JSON_TYPE_BOOL: {
        const char* const s = d_->json + t_->start;
        return *s == 't';
    }
    case JSON_TYPE_NUMBER:
    case JSON_TYPE_STRING: {
        const char* const s = d_->json + t_->start;
        return strtod(s, nullptr);
    }
    default:
        return 0.0;
    }
}

spark::JSONType spark::JSONValue::type() const {
    if (!t_) {
        return JSON_TYPE_INVALID;
    }
    switch (t_->type) {
    case JSMN_PRIMITIVE: {
        const char c = d_->json[t_->start];
        if (c == '-' || (c >= '0' && c <= '9')) {
            return JSON_TYPE_NUMBER;
        } else if (c == 't' || c == 'f') { // Literal names are always in lower case
            return JSON_TYPE_BOOL;
        } else if (c == 'n') {
            return JSON_TYPE_NULL;
        }
        return JSON_TYPE_INVALID;
    }
    case JSMN_STRING:
        return JSON_TYPE_STRING;
    case JSMN_ARRAY:
        return JSON_TYPE_ARRAY;
    case JSMN_OBJECT:
        return JSON_TYPE_OBJECT;
    default:
        return JSON_TYPE_INVALID;
    }
}

spark::JSONValue spark::JSONValue::parse(char *json, size_t size) {
    detail::JSONDataPtr d(new(std::nothrow) detail::JSONData);
    if (!d) {
        return JSONValue();
    }
    size_t tokenCount = 0;
    if (!tokenize(json, size, &d->tokens, &tokenCount)) {
        return JSONValue();
    }
    const jsmntok_t *t = d->tokens; // Root token
    if (t->type == JSMN_PRIMITIVE) {
        // RFC 7159 allows JSON document to consist of a single primitive value, such as a number.
        // In this case, original data is copied to a larger buffer to ensure room for term. null
        // character (see stringize() method)
        d->json = new(std::nothrow) char[size + 1];
        if (!d->json) {
            return JSONValue();
        }
        memcpy(d->json, json, size);
        d->freeJson = true; // Set ownership flag
    } else {
        d->json = json;
    }
    if (!stringize(d->tokens, tokenCount, d->json)) {
        return JSONValue();
    }
    return JSONValue(t, d);
}

spark::JSONValue spark::JSONValue::parseCopy(const char *json, size_t size) {
    detail::JSONDataPtr d(new(std::nothrow) detail::JSONData);
    if (!d) {
        return JSONValue();
    }
    size_t tokenCount = 0;
    if (!tokenize(json, size, &d->tokens, &tokenCount)) {
        return JSONValue();
    }
    d->json = new(std::nothrow) char[size + 1];
    if (!d->json) {
        return JSONValue();
    }
    memcpy(d->json, json, size); // TODO: Copy only token data
    d->freeJson = true;
    if (!stringize(d->tokens, tokenCount, d->json)) {
        return JSONValue();
    }
    return JSONValue(d->tokens, d);
}

bool spark::JSONValue::tokenize(const char *json, size_t size, jsmntok_t **tokens, size_t *count) {
    jsmn_parser parser;
    parser.size = sizeof(jsmn_parser);
    jsmn_init(&parser, nullptr);
    const int n = jsmn_parse(&parser, json, size, nullptr, 0, nullptr); // Get number of tokens
    if (n <= 0) {
        return false; // Parsing error
    }
    std::unique_ptr<jsmntok_t[]> t(new(std::nothrow) jsmntok_t[n]);
    if (!t) {
        return false;
    }
    jsmn_init(&parser, nullptr); // Reset parser
    if (jsmn_parse(&parser, json, size, t.get(), n, nullptr) <= 0) {
        return false;
    }
    *tokens = t.release();
    *count = n;
    return true;
}

bool spark::JSONValue::stringize(jsmntok_t *t, size_t count, char *json) {
    const jsmntok_t* const end = t + count;
    while (t != end) {
        if (t->type == JSMN_STRING) {
            if (!unescape(t, json)) {
                return false; // Malformed string
            }
            json[t->end] = '\0';
        } else if (t->type == JSMN_PRIMITIVE) {
            json[t->end] = '\0';
        }
        ++t;
    }
    return true;
}

bool spark::JSONValue::unescape(jsmntok_t *t, char *json) {
    char *str = json + t->start; // Destination string
    const char* const end = json + t->end; // End of the source string
    const char *s1 = str; // Beginning of an unescaped sequence
    const char *s = s1;
    while (s != end) {
        if (*s == '\\') {
            if (s != s1) {
                const size_t n = s - s1;
                memmove(str, s1, n); // Shift preceeding characters
                str += n;
                s1 = s;
            }
            ++s;
            if (s == end) {
                return false; // Unexpected end of string
            }
            if (*s == 'u') { // Arbitrary character, e.g. "\u001f"
                ++s;
                if (end - s < 4) {
                    return false; // Unexpected end of string
                }
                uint32_t u = 0; // Unicode code point or UTF-16 surrogate pair
                if (!hexToInt(s, 4, &u)) {
                    return false; // Invalid escaped sequence
                }
                if (u <= 0x7f) { // Processing only code points within the basic latin block
                    *str = u;
                    ++str;
                    s1 += 6; // Skip escaped sequence
                }
                s += 4;
            } else {
                switch (*s) {
                case '"':
                case '\\':
                case '/':
                    *str = *s;
                    break;
                case 'b': // Backspace
                    *str = 0x08;
                    break;
                case 't': // Tab
                    *str = 0x09;
                    break;
                case 'n': // Line feed
                    *str = 0x0a;
                    break;
                case 'f': // Form feed
                    *str = 0x0c;
                    break;
                case 'r': // Carriage return
                    *str = 0x0d;
                    break;
                default:
                    return false; // Invalid escaped sequence
                }
                ++str;
                ++s;
                s1 = s; // Skip escaped sequence
            }
        } else {
            ++s;
        }
    }
    if (s != s1) {
        const size_t n = s - s1;
        memmove(str, s1, n); // Shift remaining characters
        str += n;
    }
    t->end = str - json; // Update string length
    return true;
}

// spark::JSONString
spark::JSONString::JSONString(const jsmntok_t *t, detail::JSONDataPtr d) :
        JSONString() {
    if (t && (t->type == JSMN_STRING || t->type == JSMN_PRIMITIVE)) {
        if (t->type != JSMN_PRIMITIVE || d->json[t->start] != 'n') { // Nulls are treated as empty strings
            s_ = d->json + t->start;
            n_ = t->end - t->start;
        }
        d_ = d;
    }
}

bool spark::JSONString::operator==(const String &str) const {
    return n_ == str.length() && strncmp(s_, str.c_str(), n_) == 0;
}

bool spark::JSONString::operator==(const JSONString &str) const {
    return n_ == str.n_ && strncmp(s_, str.s_, n_) == 0;
}

// spark::JSONObjectIterator
spark::JSONObjectIterator::JSONObjectIterator(const jsmntok_t *t, detail::JSONDataPtr d) :
        JSONObjectIterator() {
    if (t && t->type == JSMN_OBJECT) {
        t_ = t + 1; // First property's name
        n_ = t->size; // Number of properties
        d_ = d;
    }
}

bool spark::JSONObjectIterator::next() {
    if (!n_) {
        return false;
    }
    k_ = t_; // Name
    ++t_;
    v_ = t_; // Value
    --n_;
    if (n_) {
        t_ = skipToken(t_);
    }
    return true;
}

// spark::JSONArrayIterator
spark::JSONArrayIterator::JSONArrayIterator(const jsmntok_t *t, detail::JSONDataPtr d) :
        JSONArrayIterator() {
    if (t && t->type == JSMN_ARRAY) {
        t_ = t + 1; // First element
        n_ = t->size; // Number of elements
        d_ = d;
    }
}

bool spark::JSONArrayIterator::next() {
    if (!n_) {
        return false;
    }
    v_ = t_;
    --n_;
    if (n_) {
        t_ = skipToken(t_);
    }
    return true;
}

// spark::JSONWriter
spark::JSONWriter& spark::JSONWriter::beginArray() {
    writeSeparator();
    write('[');
    state_ = BEGIN;
    return *this;
}

spark::JSONWriter& spark::JSONWriter::endArray() {
    write(']');
    state_ = NEXT;
    return *this;
}

spark::JSONWriter& spark::JSONWriter::beginObject() {
    writeSeparator();
    write('{');
    state_ = BEGIN;
    return *this;
}

spark::JSONWriter& spark::JSONWriter::endObject() {
    write('}');
    state_ = NEXT;
    return *this;
}

spark::JSONWriter& spark::JSONWriter::name(const char *name, size_t size) {
    writeSeparator();
    writeEscaped(name, size);
    state_ = VALUE;
    return *this;
}

spark::JSONWriter& spark::JSONWriter::value(bool val) {
    writeSeparator();
    if (val) {
        write("true", 4);
    } else {
        write("false", 5);
    }
    state_ = NEXT;
    return *this;
}

spark::JSONWriter& spark::JSONWriter::value(int val) {
    writeSeparator();
    printf("%d", val);
    state_ = NEXT;
    return *this;
}

spark::JSONWriter& spark::JSONWriter::value(unsigned val) {
    writeSeparator();
    printf("%u", val);
    state_ = NEXT;
    return *this;
}

spark::JSONWriter& spark::JSONWriter::value(long val) {
    writeSeparator();
    printf("%ld", val);
    state_ = NEXT;
    return *this;
}

spark::JSONWriter& spark::JSONWriter::value(unsigned long val) {
    writeSeparator();
    printf("%lu", val);
    state_ = NEXT;
    return *this;
}

spark::JSONWriter& spark::JSONWriter::value(double val, int precision) {
    writeSeparator();
    printf("%.*lf", precision, val);
    state_ = NEXT;
    return *this;
}

spark::JSONWriter& spark::JSONWriter::value(double val) {
    writeSeparator();
    printf("%g", val);
    state_ = NEXT;
    return *this;
}

spark::JSONWriter& spark::JSONWriter::value(const char *val, size_t size) {
    writeSeparator();
    writeEscaped(val, size);
    state_ = NEXT;
    return *this;
}

spark::JSONWriter& spark::JSONWriter::nullValue() {
    writeSeparator();
    write("null", 4);
    state_ = NEXT;
    return *this;
}

void spark::JSONWriter::printf(const char *fmt, ...) {
    char buf[16];
    va_list args;
    va_start(args, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    if ((size_t)n >= sizeof(buf)) {
        char buf[n + 1]; // Use larger buffer
        va_start(args, fmt);
        n = vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);
        if (n > 0) {
            write(buf, n);
        }
    } else if (n > 0) {
        write(buf, n);
    }
}

void spark::JSONWriter::writeSeparator() {
    switch (state_) {
    case NEXT:
        write(',');
        break;
    case VALUE:
        write(':');
        break;
    default:
        break;
    }
}

void spark::JSONWriter::writeEscaped(const char *str, size_t size) {
    write('"');
    const char* const end = str + size;
    const char *s = str;
    while (s != end) {
        const char c = *s;
        if (c == '"' || c == '\\' || (c >= 0 && c <= 0x1f)) {
            write(str, s - str); // Write preceeding characters
            write('\\');
            switch (c) {
            case '"':
            case '\\':
                write(c);
                break;
            case 0x08: // Backspace
                write('b');
                break;
            case 0x09: // Tab
                write('t');
                break;
            case 0x0a: // Line feed
                write('n');
                break;
            case 0x0c: // Form feed
                write('f');
                break;
            case 0x0d: // Carriage return
                write('r');
                break;
            default:
                // All other control characters are written in hex, e.g. "\u001f"
                printf("u%04x", (unsigned)c);
                break;
            }
            str = s + 1;
        }
        ++s;
    }
    if (s != str) {
        write(str, s - str); // Write remaining characters
    }
    write('"');
}

// spark::JSONBufferWriter
void spark::JSONBufferWriter::write(const char *data, size_t size) {
    if (n_ < bufSize_) {
        memcpy(buf_ + n_, data, std::min(size, bufSize_ - n_));
    }
    n_ += size;
}

void spark::JSONBufferWriter::printf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    const int n = vsnprintf(buf_ + n_, (n_ < bufSize_) ? bufSize_ - n_ : 0, fmt, args);
    va_end(args);
    n_ += n;
}
