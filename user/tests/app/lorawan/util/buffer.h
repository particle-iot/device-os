#pragma once

#include <cstring>

#include <spark_wiring_vector.h>
#include <spark_wiring_error.h>

namespace particle::util {

class Buffer {
public:
    Buffer() = default;

    explicit Buffer(size_t size) :
            d_(size) {
    }

    Buffer(const char* data, size_t size) :
            d_(data, size) {
    }

    char* data() {
        return d_.data();
    }

    const char* data() const {
        return d_.data();
    }

    size_t size() const {
        return d_.size();
    }

    int resize(size_t size) {
        if (!d_.resize(size)) {
            return Error::NO_MEMORY;
        }
        return 0;
    }

    int slice(size_t pos, size_t size, Buffer& buf) const {
        if (pos > (size_t)d_.size()) {
            pos = d_.size();
        }
        if (pos + size > (size_t)d_.size()) {
            size = d_.size() - pos;
        }
        int r = buf.resize(size);
        if (r < 0) {
            return r;
        }
        std::memcpy(buf.data(), d_.data() + pos, size);
        return 0;
    }

    static int concat(const Buffer& buf1, const Buffer& buf2, Buffer& buf) {
        int r = buf.resize(buf1.size() + buf2.size());
        if (r < 0) {
            return r;
        }
        std::memcpy(buf.data(), buf1.data(), buf1.size());
        std::memcpy(buf.data() + buf1.size(), buf2.data(), buf2.size());
        return 0;
    }

private:
    Vector<char> d_;
};

} // namespace particle::util
