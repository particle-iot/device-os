#ifndef TEST_TOOLS_BUFFER_H
#define TEST_TOOLS_BUFFER_H

#include "check.h"

#include <boost/endian/conversion.hpp>

#include <string>

namespace test {

class Buffer: public Checkable<std::string, Buffer> {
public:
    static const size_t DEFAULT_PADDING = 16;

    explicit Buffer(size_t size = 0, size_t padding = DEFAULT_PADDING);
    explicit Buffer(const std::string& data, size_t padding = DEFAULT_PADDING);
    Buffer(const char* data, size_t size, size_t padding = DEFAULT_PADDING);

    char* data();
    const char* data() const;

    size_t size() const;
    bool isEmpty() const;

    template<typename T>
    T readLe();

    template<typename T>
    T readBe();

    template<typename T>
    T read();

    size_t readPos() const;

    bool isPaddingValid() const;

    explicit operator char*();
    explicit operator const char*() const;
    explicit operator std::string() const; // Required by Checkable mixin

private:
    std::string data_, padding_;
    size_t readPos_;
};

} // namespace test

inline char* test::Buffer::data() {
    return &data_.front() + padding_.size();
}

inline const char* test::Buffer::data() const {
    return &data_.front() + padding_.size();
}

inline size_t test::Buffer::size() const {
    return data_.size() - padding_.size() * 2;
}

inline bool test::Buffer::isEmpty() const {
    return (size() == 0);
}

inline test::Buffer::operator char*() {
    return data();
}

inline test::Buffer::operator const char*() const {
    return data();
}

inline test::Buffer::operator std::string() const {
    return data_.substr(padding_.size(), size());
}

template<typename T>
inline T test::Buffer::readLe() {
    return boost::endian::little_to_native(read<T>());
}

template<typename T>
inline T test::Buffer::readBe() {
    return boost::endian::big_to_native(read<T>());
}

template<typename T>
inline T test::Buffer::read() {
    REQUIRE((readPos_ + sizeof(T) <= size()));
    T val = 0;
    memcpy(&val, data() + readPos_, sizeof(T));
    readPos_ += sizeof(T);
    return val;
}

inline size_t test::Buffer::readPos() const {
    return readPos_;
}

#endif // TEST_TOOLS_BUFFER_H
