#include <cstdint>
#include <stddef.h>

#ifndef SIMPLE_POOL_PREFER_SMALLER_BLOCK
#define SIMPLE_POOL_PREFER_SMALLER_BLOCK (1)
#endif

class SimpleBasePool {
public:
    void* allocate(size_t size) {
        void* p = nullptr;
        const size_t alignedSize = aligned(sizeof(BlockHeader) + size);
        if ((size_ - (ptr_ - begin_)) >= alignedSize) {
            BlockHeader* b = reinterpret_cast<BlockHeader*>(ptr_);
            b->size = alignedSize;
            b->next = nullptr;
            p = ptr_ + sizeof(BlockHeader);
            ptr_ += alignedSize;
        } else {
            BlockHeader* candidate = nullptr;
            BlockHeader* prev = nullptr;
            for (BlockHeader* b = freeList_, *pr = nullptr; b != nullptr; pr = b, b = b->next) {
                if (b->size >= alignedSize) {
#if SIMPLE_POOL_PREFER_SMALLER_BLOCK == 1
                    // Prefer smallest block
                    if (candidate == nullptr || candidate->size > b->size) {
#else
                    // Prefer rightmost block
                    if (true) {
#endif // SIMPLE_POOL_PREFER_SMALLER_BLOCK
                        candidate = b;
                        prev = pr;
                    }
                }
            }
            if (candidate != nullptr) {
                // Remove from free list
                if (prev != nullptr) {
                    prev->next = candidate->next;
                } else {
                    freeList_ = candidate->next;
                    prev = freeList_;
                }
                if (candidate == freeListEnd_) {
                    freeListEnd_ = prev;
                }
                candidate->next = nullptr;
                p = reinterpret_cast<void*>(candidate->data);
            }
        }
        return p;
    }

    void deallocate(void* p) {
        if (p == nullptr) {
            return;
        }
        BlockHeader* block = reinterpret_cast<BlockHeader*>(reinterpret_cast<uint8_t*>(p) - sizeof(BlockHeader));
        if ((reinterpret_cast<uint8_t*>(block) + block->size) == ptr_) {
            ptr_ -= block->size;
        } else {
            BlockHeader* ins = nullptr;
            for(BlockHeader* b = freeList_; b != nullptr; b = b->next) {
                if (block > b) {
                    ins = b;
                }
            }
            if (ins == nullptr) {
                if (freeList_ == nullptr) {
                    freeListEnd_ = block;
                }
                block->next = freeList_;
                freeList_ = block;
            } else {
                block->next = ins->next;
                ins->next = block;
                if (ins == freeListEnd_) {
                    freeListEnd_ = block;
                }
            }
        }
        shrink();
    }

    virtual ~SimpleBasePool() = default;

protected:
    SimpleBasePool(void* location, size_t size) :
        begin_(reinterpret_cast<uint8_t*>(location)),
        size_(size),
        ptr_(begin_) {
    }

    uint8_t* begin_;
    size_t size_;

// Leaving these protected to simplify unit testing
// private:

    struct BlockHeader {
        BlockHeader* next;
        uintptr_t size;
        uint8_t data[0];
    };

    static_assert(sizeof(BlockHeader) % sizeof(uintptr_t) == 0, "SimpleBasePool: size of header should be a multiple of uintptr_t");

    static size_t aligned(size_t sz) {
        return (sz + (sizeof(uintptr_t) - (sz % sizeof(uintptr_t))));
    }

    BlockHeader* getPrevFree(BlockHeader* block) const {
        for (BlockHeader* b = freeList_; b != nullptr; b = b->next) {
            if (b->next == block) {
                return b;
            }
        }
        return nullptr;
    }

    void shrink() {
        while (freeListEnd_ != nullptr && (reinterpret_cast<uint8_t*>(freeListEnd_) + freeListEnd_->size) == ptr_) {
            ptr_ -= freeListEnd_->size;
            freeListEnd_ = getPrevFree(freeListEnd_);
            if (freeListEnd_ == nullptr) {
                freeList_ = nullptr;
            } else {
                freeListEnd_->next = nullptr;
            }
        }
    }

    uint8_t* ptr_;

    BlockHeader* freeList_ = nullptr;
    BlockHeader* freeListEnd_ = nullptr;
};

class SimpleAllocedPool : public SimpleBasePool {
public:
    SimpleAllocedPool(size_t size) :
        SimpleBasePool(reinterpret_cast<void*>(new uint8_t[size]), size) {
    }

    SimpleAllocedPool(void* ptr, size_t size) :
        SimpleAllocedPool(size) {
    }

    virtual ~SimpleAllocedPool() {
        delete[] begin_;
    }
};

class SimpleStaticPool : public SimpleBasePool {
public:
    SimpleStaticPool(void* ptr, size_t size) :
        SimpleBasePool(ptr, size) {
    }
};
