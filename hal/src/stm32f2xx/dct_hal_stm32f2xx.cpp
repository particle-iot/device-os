#include "dct_hal_stm32f2xx.h"

#include "core_hal_stm32f2xx.h"
#include "interrupts_hal.h"
#include "static_recursive_mutex.h"
#include "debug.h"

namespace {

StaticRecursiveMutex dctLock;

#ifdef DEBUG_BUILD
#define DCT_LOCK_TIMEOUT 30000
int dctLockCounter = 0;
#else
#define DCT_LOCK_TIMEOUT 0 // Wait indefinitely
#endif

} // namespace

int dct_lock(int write) {
    if (!rtos_started) {
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
    if (!rtos_started) {
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
