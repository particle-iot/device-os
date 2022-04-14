#ifndef TEST_TOOLS_ALLOC_H
#define TEST_TOOLS_ALLOC_H

#include "buffer.h"

#include <boost/optional.hpp>

#include <unordered_map>
#include <mutex>

namespace test {

class Allocator {
public:
    explicit Allocator(size_t padding = Buffer::DEFAULT_PADDING);

    void* malloc(size_t size);
    void* calloc(size_t n, size_t size);
    void* realloc(void* ptr, size_t size);
    void free(void* ptr);

    size_t allocSize() const;
    Allocator& allocLimit(size_t size);
    Allocator& noAllocLimit();

    void check();

    void reset();

    // This class is non-copyable
    Allocator(const Allocator&) = delete;
    Allocator& operator=(const Allocator&) = delete;

private:
    struct FreedBuffer {
        Buffer buffer;
        std::string data;
    };

    std::unordered_map<void*, Buffer> alloc_;
    std::unordered_map<void*, FreedBuffer> free_;
    boost::optional<size_t> allocLimit_;
    size_t allocSize_, padding_;
    bool failed_;

    mutable std::recursive_mutex mutex_;
};

class DefaultAllocator {
public:
    static void* malloc(size_t size);
    static void* calloc(size_t n, size_t size);
    static void* realloc(void* ptr, size_t size);
    static void free(void* ptr);

    static void check();

    static void reset();

private:
    static Allocator* instance();
};

} // namespace test

inline void* test::DefaultAllocator::malloc(size_t size) {
    return instance()->malloc(size);
}

inline void* test::DefaultAllocator::calloc(size_t n, size_t size) {
    return instance()->calloc(n, size);
}

inline void* test::DefaultAllocator::realloc(void* ptr, size_t size) {
    return instance()->realloc(ptr, size);
}

inline void test::DefaultAllocator::free(void* ptr) {
    instance()->free(ptr);
}

inline void test::DefaultAllocator::check() {
    instance()->check();
}

inline void test::DefaultAllocator::reset() {
    instance()->reset();
}

#endif // TEST_TOOLS_ALLOC_H
