#pragma once

#include "system_task.h"
#include "test_malloc.h"

#include "tools/alloc.h"

#include "hippomocks.h"

namespace test {

class HeapAllocator: public Allocator {
public:
    template<typename... ArgsT>
    explicit HeapAllocator(MockRepository* mocks, ArgsT&&... args) :
            Allocator(std::forward<ArgsT>(args)...) {
        mocks->OnCallFunc(t_malloc).Do([this](size_t size) {
            return this->malloc(size);
        });
        mocks->OnCallFunc(t_calloc).Do([this](size_t count, size_t size) {
            return this->calloc(count, size);
        });
        mocks->OnCallFunc(t_realloc).Do([this](void* ptr, size_t size) {
            return this->realloc(ptr, size);
        });
        mocks->OnCallFunc(t_free).Do([this](void* ptr) {
            this->free(ptr);
        });
    }
};

class PoolAllocator: public Allocator {
public:
    template<typename... ArgsT>
    explicit PoolAllocator(MockRepository* mocks, ArgsT&&... args) :
            Allocator(std::forward<ArgsT>(args)...) {
        mocks->OnCallFunc(system_pool_alloc).Do([this](size_t size, void* reserved) {
            return this->malloc(size);
        });
        mocks->OnCallFunc(system_pool_free).Do([this](void* ptr, void* reserved) {
            this->free(ptr);
        });
    }
};

} // namespace test
