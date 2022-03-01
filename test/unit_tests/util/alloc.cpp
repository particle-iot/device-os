#include "alloc.h"

#include "catch.h"

#include <algorithm>
#include <cstring>

test::Allocator::Allocator(size_t padding) :
        allocSize_(0),
        padding_(padding),
        failed_(false) {
}

void* test::Allocator::malloc(size_t size) {
    const std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (allocLimit_ && allocSize_ + size > *allocLimit_) {
        return nullptr;
    }
    Buffer buf(size, padding_);
    void* const ptr = buf.data();
    alloc_.insert(std::make_pair(ptr, std::move(buf)));
    allocSize_ += size;
    return ptr;
}

void* test::Allocator::calloc(size_t n, size_t size) {
    const std::lock_guard<std::recursive_mutex> lock(mutex_);
    size *= n;
    void* const ptr = this->malloc(size);
    if (ptr) {
        memset(ptr, 0, size);
    }
    return ptr;
}

void* test::Allocator::realloc(void* ptr, size_t size) {
    const std::lock_guard<std::recursive_mutex> lock(mutex_);
    try {
        size_t origSize = 0; // Original buffer size
        if (ptr) {
            const auto it = alloc_.find(ptr);
            if (it == alloc_.end()) {
                throw std::runtime_error("Invalid realloc() detected");
            }
            origSize = it->second.size();
        }
        void* const newPtr = this->malloc(size);
        if (newPtr && ptr) {
            memcpy(newPtr, ptr, std::min(size, origSize)); // Copy user data from the original buffer
            this->free(ptr);
        }
        return newPtr;
    } catch (const std::exception& e) {
        // realloc() and free() can be called in destructor, so we can't use CATCH_FAIL() here,
        // as it throws exceptions
        CATCH_WARN("test::Allocator: " << e.what());
        failed_ = true;
        return nullptr;
    }
}

void test::Allocator::free(void* ptr) {
    const std::lock_guard<std::recursive_mutex> lock(mutex_);
    try {
        if (!ptr) {
            return;
        }
        if (free_.count(ptr)) {
            throw std::runtime_error("Double free() detected");
        }
        const auto it = alloc_.find(ptr);
        if (it == alloc_.end()) {
            throw std::runtime_error("Invalid free() detected");
        }
        FreedBuffer f;
        f.buffer = std::move(it->second);
        f.data = (std::string)f.buffer; // User data before free() has been called
        alloc_.erase(it);
        const bool ok = f.buffer.isPaddingValid();
        free_.insert(std::make_pair(ptr, std::move(f)));
        allocSize_ -= f.buffer.size();
        if (!ok) {
            throw std::runtime_error("Buffer overflow detected");
        }
    } catch (const std::exception& e) {
        CATCH_WARN("test::Allocator: " << e.what());
        failed_ = true;
    }
}

void test::Allocator::check() {
    const std::lock_guard<std::recursive_mutex> lock(mutex_);
    // Check all previously allocated buffers
    if (!alloc_.empty()) {
        FAIL("test::Allocator: Memory leak detected");
    }
    for (auto it = free_.begin(); it != free_.end(); ++it) {
        const FreedBuffer& f = it->second;
        if (!f.buffer.isPaddingValid() || (std::string)f.buffer != f.data) {
            FAIL("test::Allocator: Memory corruption detected");
        }
    }
    if (failed_) {
        FAIL("test::Allocator: Memory check failed");
    }
}

void test::Allocator::reset() {
    const std::lock_guard<std::recursive_mutex> lock(mutex_);
    alloc_.clear();
    free_.clear();
    allocLimit_ = boost::none;
    allocSize_ = 0;
    failed_ = false;
}

size_t test::Allocator::allocSize() const {
    const std::lock_guard<std::recursive_mutex> lock(mutex_);
    return allocSize_;
}

test::Allocator& test::Allocator::allocLimit(size_t size) {
    const std::lock_guard<std::recursive_mutex> lock(mutex_);
    allocLimit_ = size;
    return *this;
}

test::Allocator& test::Allocator::noAllocLimit() {
    const std::lock_guard<std::recursive_mutex> lock(mutex_);
    allocLimit_ = boost::none;
    return *this;
}

test::Allocator* test::DefaultAllocator::instance() {
    static Allocator alloc;
    return &alloc;
}
