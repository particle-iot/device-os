#include "buffer.h"

#include "random.h"

#include <cstring>

// test::Buffer
test::Buffer::Buffer(size_t size, size_t padding) :
        d_(size + padding * 2, 0),
        p_(randomBytes(padding)) { // Generate padding bytes
    memcpy(&d_.front(), p_.data(), padding);
    memcpy(&d_.front() + padding, randomBytes(size).data(), size); // Initialize buffer with random data
    memcpy(&d_.front() + padding + size, p_.data(), padding);
}

bool test::Buffer::isPaddingValid() const {
    return memcmp(d_.data(), p_.data(), p_.size()) == 0 &&
            memcmp(d_.data() + d_.size() - p_.size(), p_.data(), p_.size()) == 0;
}
