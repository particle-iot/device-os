#ifndef TEST_TOOLS_STREAM_H
#define TEST_TOOLS_STREAM_H

#include "spark_wiring_print.h"

#include "check.h"

#include <string>

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
