#include "dct_hal_stm32f2xx.h"

#include "interrupts_hal.h"
#include "static_recursive_mutex.h"
#include "debug.h"

namespace {

StaticRecursiveMutex dctLock;
volatile bool dctLockDisabled = false;

#ifdef DEBUG_BUILD
#define DCT_LOCK_TIMEOUT 10000
int dctLockCounter = 0;
#else
#define DCT_LOCK_TIMEOUT 0 // Wait indefinitely
#endif

} // namespace

int dct_lock(int write) {
    if (dctLockDisabled) {
        return 0;
    }
    SPARK_ASSERT(!HAL_IsISR());
    const bool ok = dctLock.lock(DCT_LOCK_TIMEOUT);
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
