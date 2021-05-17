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

#ifndef SPARK_WIRING_JSON_H
#define SPARK_WIRING_JSON_H

#include "spark_wiring_print.h"
#include "spark_wiring_string.h"

#include "jsmn.h"

#include <cstring>
#include <memory>

namespace spark {

namespace detail {

struct JSONData; // Parsed JSON data
typedef std::shared_ptr<JSONData> JSONDataPtr;

} // namespace spark::detail

enum JSONType {
    JSON_TYPE_INVALID,
    JSON_TYPE_NULL,
    JSON_TYPE_BOOL,
    JSON_TYPE_NUMBER,
    JSON_TYPE_STRING,
    JSON_TYPE_ARRAY,
    JSON_TYPE_OBJECT
};

class JSONString;
class JSONArrayIterator;
class JSONObjectIterator;

// Immutable JSON value
class JSONValue {
public:
    JSONValue(); // Constructs invalid value

    bool toBool() const;
    int toInt() const;
    double toDouble() const;
    JSONString toString() const;

    JSONType type() const;

    bool isNull() const;
    bool isBool() const;
    bool isNumber() const;
    bool isString() const;
    bool isArray() const;
    bool isObject() const;

    bool isValid() const;

    static JSONValue parse(char *json, size_t size);
    static JSONValue parseCopy(const char *json, size_t size);
    static JSONValue parseCopy(const char *json);

private:
    detail::JSONDataPtr d_;
    const jsmntok_t *t_; // Token representing this value

    JSONValue(const jsmntok_t *token, detail::JSONDataPtr data);

    static bool tokenize(const char *json, size_t size, jsmntok_t **tokens, size_t *count);
    static bool stringize(jsmntok_t *tokens, size_t count, char *json);
    static bool unescape(jsmntok_t *token, char *json);

    friend class JSONString;
    friend class JSONArrayIterator;
    friend class JSONObjectIterator;
};

class JSONString {
public:
    JSONString();
    explicit JSONString(const JSONValue &value);

    const char* data() const; // Returns null-terminated string

    size_t size() const;
    bool isEmpty() const;

    bool operator==(const char *str) const;
    bool operator!=(const char *str) const;
    bool operator==(const String &str) const;
    bool operator!=(const String &str) const;
    bool operator==(const JSONString &str) const;
    bool operator!=(const JSONString &str) const;

    explicit operator const char*() const;
    explicit operator String() const;

private:
    detail::JSONDataPtr d_;
    const char *s_;
    size_t n_;

    JSONString(const jsmntok_t *token, detail::JSONDataPtr data);

    friend class JSONValue;
    friend class JSONObjectIterator;
};

class JSONArrayIterator {
public:
    JSONArrayIterator();
    explicit JSONArrayIterator(const JSONValue &value);

    bool next();

    JSONValue value() const;

    size_t count() const; // Returns number of remaining elements

private:
    detail::JSONDataPtr d_;
    const jsmntok_t *t_, *v_;
    size_t n_;

    JSONArrayIterator(const jsmntok_t *token, detail::JSONDataPtr data);
};

class JSONObjectIterator {
public:
    JSONObjectIterator();
    explicit JSONObjectIterator(const JSONValue &value);

    bool next();

    JSONString name() const;
    JSONValue value() const;

    size_t count() const; // Returns number of remaining elements

private:
    detail::JSONDataPtr d_;
    const jsmntok_t *t_, *k_, *v_;
    size_t n_;

    JSONObjectIterator(const jsmntok_t *token, detail::JSONDataPtr data);
};

// Abstract JSON document writer
class JSONWriter {
public:
    JSONWriter();
    virtual ~JSONWriter() = default;

    JSONWriter& beginArray();
    JSONWriter& endArray();
    JSONWriter& beginObject();
    JSONWriter& endObject();
    JSONWriter& name(const char *name);
    JSONWriter& name(const char *name, size_t size);
    JSONWriter& name(const String &name);
    JSONWriter& value(bool val);
    JSONWriter& value(int val);
    JSONWriter& value(unsigned val);
    JSONWriter& value(long val);
    JSONWriter& value(unsigned long val);
    JSONWriter& value(double val, int precision);
    JSONWriter& value(double val);
    JSONWriter& value(const char *val);
    JSONWriter& value(const char *val, size_t size);
    JSONWriter& value(const String &val);
    JSONWriter& nullValue();

protected:
    virtual void write(const char *data, size_t size) = 0;
    virtual void printf(const char *fmt, ...);

private:
    enum State {
        BEGIN, // Beginning of a document or a compound value
        NEXT, // Expecting next element of a compound value
        VALUE // Expecting value of an object's property
    };

    State state_;

    void writeSeparator();
    void writeEscaped(const char *data, size_t size);
    void write(char c);
};

class JSONStreamWriter: public JSONWriter {
public:
    explicit JSONStreamWriter(Print &stream);

    Print* stream() const;

protected:
    virtual void write(const char *data, size_t size) override;

private:
    Print &strm_;
};

class JSONBufferWriter: public JSONWriter {
public:
    JSONBufferWriter(char *buf, size_t size);

    char* buffer() const;
    size_t bufferSize() const;

    size_t dataSize() const; // Returned value can be greater than buffer size

protected:
    virtual void write(const char *data, size_t size) override;
    virtual void printf(const char *fmt, ...) override;

private:
    char *buf_;
    size_t bufSize_, n_;
};

bool operator==(const char *str1, const JSONString &str2);
bool operator!=(const char *str1, const JSONString &str2);
bool operator==(const String &str1, const JSONString &str2);
bool operator!=(const String &str1, const JSONString &str2);

} // namespace spark

// spark::JSONValue
inline spark::JSONValue::JSONValue() :
        t_(nullptr) {
}

inline spark::JSONString spark::JSONValue::toString() const {
    return JSONString(t_, d_);
}

inline bool spark::JSONValue::isNull() const {
    return type() == JSON_TYPE_NULL;
}

inline bool spark::JSONValue::isBool() const {
    return type() == JSON_TYPE_BOOL;
}

inline bool spark::JSONValue::isNumber() const {
    return type() == JSON_TYPE_NUMBER;
}

inline bool spark::JSONValue::isString() const {
    return type() == JSON_TYPE_STRING;
}

inline bool spark::JSONValue::isArray() const {
    return type() == JSON_TYPE_ARRAY;
}

inline bool spark::JSONValue::isObject() const {
    return type() == JSON_TYPE_OBJECT;
}

inline bool spark::JSONValue::isValid() const {
    return type() != JSON_TYPE_INVALID;
}

inline spark::JSONValue spark::JSONValue::parseCopy(const char *json) {
    return parseCopy(json, strlen(json));
}

// spark::JSONString
inline spark::JSONString::JSONString() :
        s_(""),
        n_(0) {
}

inline spark::JSONString::JSONString(const JSONValue &value) :
        JSONString(value.t_, value.d_) {
}

inline const char* spark::JSONString::data() const {
    return s_;
}

inline size_t spark::JSONString::size() const {
    return n_;
}

inline bool spark::JSONString::isEmpty() const {
    return !n_;
}

inline bool spark::JSONString::operator==(const char *str) const {
    return strcmp(s_, str) == 0;
}

inline bool spark::JSONString::operator!=(const char *str) const {
    return !operator==(str);
}

inline bool spark::JSONString::operator!=(const String &str) const {
    return !operator==(str);
}

inline bool spark::JSONString::operator!=(const JSONString &str) const {
    return !operator==(str);
}

inline spark::JSONString::operator const char*() const {
    return s_;
}

inline spark::JSONString::operator String() const {
    return String(s_, n_);
}

// spark::JSONArrayIterator
inline spark::JSONArrayIterator::JSONArrayIterator() :
        t_(nullptr),
        v_(nullptr),
        n_(0) {
}

inline spark::JSONArrayIterator::JSONArrayIterator(const JSONValue &value) :
        JSONArrayIterator(value.t_, value.d_) {
}

inline spark::JSONValue spark::JSONArrayIterator::value() const {
    return JSONValue(v_, d_);
}

inline size_t spark::JSONArrayIterator::count() const {
    return n_;
}

// spark::JSONObjectIterator
inline spark::JSONObjectIterator::JSONObjectIterator() :
        t_(nullptr),
        k_(nullptr),
        v_(nullptr),
        n_(0) {
}

inline spark::JSONObjectIterator::JSONObjectIterator(const JSONValue &value) :
        JSONObjectIterator(value.t_, value.d_) {
}

inline spark::JSONString spark::JSONObjectIterator::name() const {
    return JSONString(k_, d_);
}

inline spark::JSONValue spark::JSONObjectIterator::value() const {
    return JSONValue(v_, d_);
}

inline size_t spark::JSONObjectIterator::count() const {
    return n_;
}

// spark::JSONWriter
inline spark::JSONWriter::JSONWriter() :
        state_(BEGIN) {
}

inline spark::JSONWriter& spark::JSONWriter::name(const char *name) {
    return this->name(name, strlen(name));
}

inline spark::JSONWriter& spark::JSONWriter::name(const String &name) {
    return this->name(name.c_str(), name.length());
}

inline spark::JSONWriter& spark::JSONWriter::value(const char *val) {
    return value(val, strlen(val));
}

inline spark::JSONWriter& spark::JSONWriter::value(const String &val) {
    return value(val.c_str(), val.length());
}

inline void spark::JSONWriter::write(char c) {
    write(&c, 1);
}

// spark::JSONStreamWriter
inline spark::JSONStreamWriter::JSONStreamWriter(Print &stream) :
        strm_(stream) {
}

inline Print* spark::JSONStreamWriter::stream() const {
    return &strm_;
}

inline void spark::JSONStreamWriter::write(const char *data, size_t size) {
    strm_.write((const uint8_t*)data, size);
}

// spark::JSONBufferWriter
inline spark::JSONBufferWriter::JSONBufferWriter(char *buf, size_t size) :
        buf_(buf),
        bufSize_(size),
        n_(0) {
}

inline char* spark::JSONBufferWriter::buffer() const {
    return buf_;
}

inline size_t spark::JSONBufferWriter::bufferSize() const {
    return bufSize_;
}

inline size_t spark::JSONBufferWriter::dataSize() const {
    return n_;
}

// spark::
inline bool spark::operator==(const char *str1, const JSONString &str2) {
    return str2 == str1;
}

inline bool spark::operator!=(const char *str1, const JSONString &str2) {
    return str2 != str1;
}

inline bool spark::operator==(const String &str1, const JSONString &str2) {
    return str2 == str1;
}

inline bool spark::operator!=(const String &str1, const JSONString &str2) {
    return str2 != str1;
}

#endif // SPARK_WIRING_JSON_H
