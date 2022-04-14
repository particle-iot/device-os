/**
 ******************************************************************************
 * @file    appender.h
 * @author  Matthew McGowan
 ******************************************************************************
  Copyright (c) 2015 Particle Industries, Inc.  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************
 */

#ifndef APPENDER_H
#define APPENDER_H

#include <stdint.h>
#include <stddef.h>

typedef bool (*appender_fn)(void* appender, const uint8_t* data, size_t length);

#ifdef	__cplusplus

#include "varint.h"
#include "endian_util.h"

#include <cstring>

namespace particle {

/**
 * OO version of the appender function.
 */
class Appender {
public:
    virtual bool append(const uint8_t* data, size_t length)=0;

    bool appendString(const char* str, size_t size) {
        return append((const uint8_t*)str, size);
    }

    bool appendString(const char* str) {
        return append((const uint8_t*)str, strlen(str));
    }

    bool appendChar(char c) {
        return append((const uint8_t*)&c, 1);
    }

    bool appendInt8(int8_t val) {
        return append((const uint8_t*)&val, 1);
    }

    bool appendUInt8(uint8_t val) {
        return append(&val, 1);
    }

    bool appendInt16LE(int16_t val) {
        val = nativeToLittleEndian(val);
        return append((const uint8_t*)&val, sizeof(val));
    }

    bool appendInt16BE(int16_t val) {
        val = nativeToBigEndian(val);
        return append((const uint8_t*)&val, sizeof(val));
    }

    bool appendUInt16LE(uint16_t val) {
        val = nativeToLittleEndian(val);
        return append((const uint8_t*)&val, sizeof(val));
    }

    bool appendUInt16BE(uint16_t val) {
        val = nativeToBigEndian(val);
        return append((const uint8_t*)&val, sizeof(val));
    }

    bool appendInt32LE(int32_t val) {
        val = nativeToLittleEndian(val);
        return append((const uint8_t*)&val, sizeof(val));
    }

    bool appendInt32BE(int32_t val) {
        val = nativeToBigEndian(val);
        return append((const uint8_t*)&val, sizeof(val));
    }

    bool appendUInt32LE(uint32_t val) {
        val = nativeToLittleEndian(val);
        return append((const uint8_t*)&val, sizeof(val));
    }

    bool appendUInt32BE(uint32_t val) {
        val = nativeToBigEndian(val);
        return append((const uint8_t*)&val, sizeof(val));
    }

    bool appendUnsignedVarint(unsigned val) {
        char buf[maxUnsignedVarintSize<unsigned>()] = {};
        const size_t n = encodeUnsignedVarint(buf, sizeof(buf), val);
        return append((const uint8_t*)buf, n);
    }

    bool append(const char* str) { // Deprecated, use appendString()
        return appendString(str);
    }

    bool append(char c) { // Deprecated, use appendChar()
        return appendChar(c);
    }

    static bool callback(void* appender, const uint8_t* data, size_t length) { // appender_fn
        Appender* a = (Appender*)appender;
        return a->append(data, length);
    }
};

// Buffer appender that never fails and stores the actual size of the data
class BufferAppender: public Appender {
public:
    using Appender::append;

    BufferAppender(void* buf, size_t size) :
            buf_((char*)buf),
            bufSize_(size),
            dataSize_(0) {
    }

    virtual bool append(const uint8_t* data, size_t size) override {
        if (dataSize_ < bufSize_) {
            size_t n = bufSize_ - dataSize_;
            if (size < n) {
                n = size;
            }
            memcpy(buf_ + dataSize_, data, n);
        }
        dataSize_ += size;
        return true;
    }

    size_t skip(size_t size) {
        return dataSize_ += size;
    }

    char* buffer() const {
        return buf_;
    }

    size_t bufferSize() const {
        return bufSize_;
    }

    size_t dataSize() const {
        return dataSize_;
    }

private:
    char* buf_;
    size_t bufSize_;
    size_t dataSize_;
};

} // namespace particle

#endif // defined(__cplusplus)

#endif	/* APPENDER_H */
