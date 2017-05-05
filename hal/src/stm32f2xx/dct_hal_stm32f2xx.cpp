#include "dct_hal_stm32f2xx.h"

#include "interrupts_hal.h"
#include "static_recursive_mutex.h"
#include "debug.h"

namespace {

StaticRecursiveMutex dctLock;
bool dctLockDisabled = false;

#ifdef DEBUG_BUILD
int dctLockCounter = 0;
#endif

} // namespace

int dct_lock(int write) {
    if (dctLockDisabled) {
        return 0;
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
    if (dctLockDisabled) {
        return 0;
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

void dct_set_lock_enabled(int enabled) {
    dctLockDisabled = !enabled;
}
