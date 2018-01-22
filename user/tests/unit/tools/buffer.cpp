#include "buffer.h"

#include "random.h"

#include <cstring>

// test::Buffer
test::Buffer::Buffer(size_t size, size_t padding) :
        data_(size + padding * 2, 0),
        padding_(randomBytes(padding)), // Generate padding bytes
        readPos_(0) {
    memcpy(&data_.front(), padding_.data(), padding);
    memcpy(&data_.front() + padding, randomBytes(size).data(), size); // Initialize buffer with random data
    memcpy(&data_.front() + padding + size, padding_.data(), padding);
}

test::Buffer::Buffer(const std::string& data, size_t padding) :
        Buffer(data.size(), padding) {
    memcpy(&data_.front() + padding, data.data(), data.size());
}

test::Buffer::Buffer(const char* data, size_t size, size_t padding) :
        Buffer(size, padding) {
    memcpy(&data_.front() + padding, data, size);
}

bool test::Buffer::isPaddingValid() const {
    return memcmp(data_.data(), padding_.data(), padding_.size()) == 0 &&
            memcmp(data_.data() + data_.size() - padding_.size(), padding_.data(), padding_.size()) == 0;
}
