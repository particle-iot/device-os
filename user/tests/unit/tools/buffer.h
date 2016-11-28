#ifndef TEST_TOOLS_BUFFER_H
#define TEST_TOOLS_BUFFER_H

#include "check.h"

#include <string>

namespace test {

class Buffer: public Checkable<std::string, Buffer> {
public:
    static const size_t DEFAULT_PADDING = 16;

    explicit Buffer(size_t size = 0, size_t padding = DEFAULT_PADDING);

    char* data();
    const char* data() const;
    size_t size() const;

    bool isPaddingValid() const;

    explicit operator char*();
    explicit operator const char*() const;
    explicit operator std::string() const; // Required by Checkable mixin

private:
    std::string d_, p_;
};

} // namespace test

inline char* test::Buffer::data() {
    return &d_.front() + p_.size();
}

inline const char* test::Buffer::data() const {
    return &d_.front() + p_.size();
}

inline size_t test::Buffer::size() const {
    return d_.size() - p_.size() * 2;
}

inline test::Buffer::operator char*() {
    return data();
}

inline test::Buffer::operator const char*() const {
    return data();
}

inline test::Buffer::operator std::string() const {
    return d_.substr(p_.size(), size());
}

#endif // TEST_TOOLS_BUFFER_H
