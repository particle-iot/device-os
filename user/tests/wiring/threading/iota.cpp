#include "Particle.h"
#include "unit-test/unit-test.h"

#if PLATFORM_THREADING
#include "system_threading.h"

#if 0 // pending semaphore functions expoded in the hal concurrent dynalib
class Semaphore
{
    os_semaphore_t semaphore;

public:

    Semaphore() : semaphore(nullptr) {}

    bool begin(unsigned max=1, unsigned initial=0) {
        return !os_semaphore_create(&semaphore, max, initial);
    }

    ~Semaphore() {
        os_semaphore_destroy(semaphore);
    }

    bool take(system_tick_t duration) {
        return !os_semaphore_take(semaphore, duration, false);
    }

    bool give() {
        return !os_semaphore_give(semaphore, false);
    }
};
#endif


class Semaphore
{
    os_queue_t queue;

public:

    Semaphore() : queue(nullptr) {}

    bool begin() {
        return !os_queue_create(&queue, 1, 1, nullptr);
    }

    ~Semaphore() {
        os_queue_destroy(queue, nullptr);
    }

    bool take(system_tick_t duration) {
        uint8_t value;
        return !os_queue_take(queue, &value, duration, nullptr);
    }

    /**
     * Returning false simply means the queue was already full and hit the maximum size
     */
    bool give() {
        uint8_t value = 0;
        return !os_queue_put(queue, &value, 0, nullptr);
    }
};


int executeOnSystemThread(std::function<void(void)> function) {
    ActiveObjectThreadQueue* sysThread = (ActiveObjectThreadQueue*)system_internal(1, nullptr);
    // todo - this method should have a return code for errors
    sysThread->invoke_async(function);
    return 0;
}


/**
 * Returns the iteration count, or a negative value on error.
 */
int runWhileSystemThreadBlocked(system_tick_t duration, std::function<void(void)> loop) {

    // create a local condition variable? initial value 0
    Semaphore start, finish;
    if (!start.begin() || !finish.begin()) {
        return -1;
    }

    // wait on the condition variable to be set
    // launch the cloud function
    const auto runner = [&] {
        start.give();
        HAL_Delay_Milliseconds(duration);
        finish.give();
    };

    executeOnSystemThread(runner);

    const system_tick_t waitForSystemThreadTimeout = 5*1000;
    if (!duration || !start.take(waitForSystemThreadTimeout)) {
        return -2;
    }

    system_tick_t starttime = millis();
    bool timeout = false;
    int count = 0;
    while (!timeout && !finish.take(0))
    {
        loop();
        timeout = (millis()-starttime)>(duration*2);
        count++;
    }

    if (timeout) {
        return -3;
    }

    return count;
}

test(01_THREADING_iota_enable_disable_updates_is_not_blocking) {
    const auto fn = [] {
            System.disableUpdates();
            System.enableUpdates();
    };

    int count = runWhileSystemThreadBlocked(500, FFL(fn));
    assertMore(count, 500);
}

#endif
