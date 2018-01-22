#include <memory>
#include <random>
#include "tools/catch.h"
#include "hippomocks.h"
#include "simple_pool_allocator.h"

static const size_t DEFAULT_POOL_SIZE = 1024;

__attribute__((weak)) int HAL_disable_irq() {
    return 0;
}

__attribute__((weak)) void HAL_enable_irq(int st) {
}

namespace {

class TestSimpleAllocedPool : public SimpleAllocedPool {
public:
    TestSimpleAllocedPool(size_t size) :
        SimpleAllocedPool(size) {
    }

    TestSimpleAllocedPool(void* ptr, size_t size) :
        SimpleAllocedPool(size) {
    }

    size_t available() const {
        size_t sz = (begin_ + size_) - ptr_;
        for (BlockHeader* b = freeList_; b != nullptr; b = b->next) {
            sz += b->size;
        }

        return sz;
    }

    uint8_t* ptr() {
        return ptr_;
    }

    uint8_t* begin() {
        return ptr_;
    }

    using BlockHeaderT = SimpleBasePool::BlockHeader;

    BlockHeaderT* freeList() {
        return freeList_;
    }

    BlockHeaderT* freeListEnd() {
        return freeListEnd_;
    }
};

class TestSimpleStaticPool : public SimpleStaticPool {
public:
    TestSimpleStaticPool(void* ptr, size_t size) :
        SimpleStaticPool(ptr, size) {
    }

    size_t available() const {
        size_t sz = (begin_ + size_) - ptr_;
        for (BlockHeader* b = freeList_; b != nullptr; b = b->next) {
            sz += b->size;
        }

        return sz;
    }

    using BlockHeaderT = SimpleBasePool::BlockHeader;

    BlockHeaderT* freeList() {
        return freeList_;
    }

    BlockHeaderT* freeListEnd() {
        return freeListEnd_;
    }
};

} // anonymous

// Just in case add mocks
class Mocks {
public:
    Mocks() {
        // HAL_disable_irq()
        mocks_.OnCallFunc(HAL_disable_irq).Do([&]() -> int {
            return 0;
        });
        // HAL_enable_irq()
        mocks_.OnCallFunc(HAL_enable_irq).Do([&](int st) -> void{
        });
    }

private:
    MockRepository mocks_;
    std::map<uint32_t, uint32_t> bkpRegs_; // Backup registers
};

template<typename T>
void testPool(void* ptr, size_t size) {
    using Pool = T;

    SECTION("Pool construction") {
        Pool pool(ptr, size);

        SECTION("Single allocation/deallocation after construction") {
            void* p = pool.allocate(1);
            CHECK(p != nullptr);
            pool.deallocate(p);
            CHECK(pool.available() == size);
        }

        SECTION("Pool can be drained completely") {
            while(pool.allocate(1) != nullptr);
            CHECK(pool.available() < (sizeof(uintptr_t) * 3));
        }

        SECTION("Zero size allocation") {
            CHECK(pool.allocate(0) != nullptr);
        }

        SECTION("Allocated addressess are aligned") {
            std::random_device r;
            std::default_random_engine e1(r());
            std::uniform_int_distribution<size_t> uniform_dist(0, sizeof(uintptr_t));

            bool ok = true;
            while(ok) {
                void* p = pool.allocate(uniform_dist(e1));
                if (p == nullptr) {
                    ok = false;
                    break;
                }
                CHECK(((reinterpret_cast<uintptr_t>(p)) % sizeof(uintptr_t)) == 0);
            }
        }

        SECTION("LIFO deallocations") {
            std::vector<void*> blocks;

            bool ok = true;
            while (ok) {
                void* p = pool.allocate(1);
                if (p == nullptr) {
                    ok = false;
                    break;
                }

                blocks.push_back(p);
            }

            CHECK(blocks.size() > 0);

            while(!blocks.empty()) {
                pool.deallocate(blocks.back());
                blocks.pop_back();
            }

            CHECK(pool.available() == size);
            CHECK(pool.freeList() == nullptr);
            CHECK(pool.freeListEnd() == nullptr);
        }

        SECTION("Random order deallocations") {
            std::vector<void*> blocks;

            bool ok = true;
            while (ok) {
                void* p = pool.allocate(1);
                if (p == nullptr) {
                    ok = false;
                    break;
                }

                blocks.push_back(p);
            }

            CHECK(blocks.size() > 0);

            {
                std::random_device r;
                std::default_random_engine e1(r());

                while(!blocks.empty()) {
                    std::uniform_int_distribution<int> dist(0, blocks.size() - 1);
                    const int idx = dist(e1);
                    pool.deallocate(blocks[idx]);
                    blocks.erase(blocks.begin() + idx);
                }
            }

            CHECK(pool.available() == size);
            CHECK(pool.freeList() == nullptr);
            CHECK(pool.freeListEnd() == nullptr);
        }

        SECTION("Allocations from free list") {
            std::vector<void*> blocks;

            bool ok = true;
            while (ok) {
                void* p = pool.allocate(1);
                if (p == nullptr) {
                    ok = false;
                    break;
                }

                blocks.push_back(p);
            }

            CHECK(blocks.size() > 0);
            CHECK(pool.available() < (sizeof(uintptr_t) * 3));

            const size_t isize = blocks.size();
            for (unsigned i = 0; i < isize; i++) {
                void* po = blocks[0];
                pool.deallocate(po);
                blocks.erase(blocks.begin());
                CHECK(pool.allocate(3 * sizeof(uintptr_t)) == nullptr);
                void* p = pool.allocate(1);
                CHECK(p != nullptr);
                CHECK(p == po);
                blocks.push_back(p);
            }

            {
                std::random_device r;
                std::default_random_engine e1(r());

                while(!blocks.empty()) {
                    std::uniform_int_distribution<int> dist(0, blocks.size() - 1);
                    const int idx = dist(e1);
                    pool.deallocate(blocks[idx]);
                    blocks.erase(blocks.begin() + idx);
                }
            }

            CHECK(pool.available() == size);
            CHECK(pool.freeList() == nullptr);
            CHECK(pool.freeListEnd() == nullptr);
        }
    }

}

TEST_CASE("SimpleAllocedPool") {
    Mocks mocks;

    testPool<TestSimpleAllocedPool>(nullptr, DEFAULT_POOL_SIZE);
}

TEST_CASE("SimpleStaticPool") {
    Mocks mocks;

    std::vector<uint8_t> buf(DEFAULT_POOL_SIZE);

    testPool<TestSimpleStaticPool>(buf.data(), buf.size());
}
