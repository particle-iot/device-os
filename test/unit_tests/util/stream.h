#ifndef TEST_TOOLS_STREAM_H
#define TEST_TOOLS_STREAM_H

#include "spark_wiring_stream.h"

#include "check.h"

#include <algorithm>
#include <string>
#include <cstring>

namespace test {

class OutputStream:
        public Print,
        public Checkable<std::string, OutputStream> {
public:
    OutputStream() = default;

    const char* data() const;
    size_t size() const;

    virtual size_t write(const uint8_t *data, size_t size) override; // Print
    virtual size_t write(uint8_t byte) override; // Print

    explicit operator std::string() const; // Required by Checkable mixin

private:
    std::string s_;
};

class Stream: public ::Stream {
public:
    explicit Stream(std::string data = std::string()) :
            data_(std::move(data)),
            readPos_(0) {
    }

    size_t readBytes(char* data, size_t size) override {
        size_t n = std::min(size, data_.size() - readPos_);
        std::memcpy(data, data_.data() + readPos_, n);
        readPos_ += n;
        return n;
    }

    int read() override {
        uint8_t b;
        size_t n = readBytes((char*)&b, 1);
        if (n != 1) {
            return -1;
        }
        return b;
    }

    int peek() override {
        if (data_.size() - readPos_ == 0) {
            return -1;
        }
        return (uint8_t)data_.at(readPos_);
    }

    int available() override {
        return data_.size() - readPos_;
    }

    size_t write(const uint8_t* data, size_t size) override {
        data_.append((const char*)data, size);
        return size;
    }

    size_t write(uint8_t b) override {
        return write(&b, 1);
    }

    void flush() override {
    }

    const std::string& data() const {
        return data_;
    }

private:
    std::string data_;
    size_t readPos_;
};

} // namespace test

// test::OutputStream
inline const char* test::OutputStream::data() const {
    return s_.c_str();
}

inline size_t test::OutputStream::size() const {
    return s_.size();
}

inline size_t test::OutputStream::write(const uint8_t *data, size_t size) {
    s_.append((const char*)data, size);
    return size;
}

inline size_t test::OutputStream::write(uint8_t byte) {
    return write(&byte, 1);
}

inline test::OutputStream::operator std::string() const {
    return s_;
}

#endif // TEST_TOOLS_STREAM_H
