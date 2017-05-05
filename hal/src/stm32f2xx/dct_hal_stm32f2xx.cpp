#include "dct_hal_stm32f2xx.h"

#include "interrupts_hal.h"
#include "static_recursive_mutex.h"
#include "debug.h"

namespace {

StaticRecursiveMutex dctLock;

dct_lock_func_t dctLockFunc = nullptr;
dct_unlock_func_t dctUnlockFunc = nullptr;

#ifdef DEBUG_BUILD
int dctLockCounter = 0;
#endif

} // namespace

int dct_lock(int write) {
    if (dctLockFunc) {
        return dctLockFunc(write);
    }
    // Default implementation
    SPARK_ASSERT(!HAL_IsISR());
    const bool ok = dctLock.lock();
    SPARK_ASSERT(ok);
#ifdef DEBUG_BUILD
    ++dctLockCounter;
    SPARK_ASSERT(dctLockCounter == 1 || !write);
#endif
    return !ok;
}

int dct_unlock(int write) {
    if (dctUnlockFunc) {
        return dctUnlockFunc(write);
    }
    // Default implementation
    SPARK_ASSERT(!HAL_IsISR());
    const bool ok = dctLock.unlock();
    SPARK_ASSERT(ok);
#ifdef DEBUG_BUILD
    --dctLockCounter;
    SPARK_ASSERT(dctLockCounter == 0 || !write);
#endif
    return !ok;
}

void dct_set_lock_impl(dct_lock_func_t lock, dct_unlock_func_t unlock) {
    dctLockFunc = lock;
    dctUnlockFunc = unlock;
}
