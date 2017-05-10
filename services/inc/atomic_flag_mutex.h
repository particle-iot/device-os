#pragma once

#include <atomic>
#include <mutex>

template <typename R, R(*spin)()>
class AtomicFlagMutex {
    std::atomic_flag flag = ATOMIC_FLAG_INIT;
public:
    void lock() {
        while (flag.test_and_set(std::memory_order_acquire)) {
            spin();
        }
    }

    void unlock() {
        flag.clear(std::memory_order_release);
    }
};
