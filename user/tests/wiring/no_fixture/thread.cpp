
#include "application.h"
#include "unit-test/unit-test.h"
#include "scope_guard.h"

#if HAL_PLATFORM_FREERTOS
#include <FreeRTOS.h>
#endif // HAL_PLATFORM_FREERTOS

namespace {

template <typename T>
bool isInRangeIncl(T value, T min, T max) {
    return (value >= min) && (value <= max);
}

const auto APPLICATION_WDT_STACK_SIZE = 2048;
const auto APPLICATION_WDT_TEST_RUNS = 10;

} // anonymous

#if PLATFORM_THREADING
#include "system_threading.h"

static uint32_t s_ram_free_before = 0;

Thread testThread;
test(THREAD_01_creation)
{
    s_ram_free_before = System.freeMemory();
    volatile bool threadRan = false;
    testThread = Thread("test", [&]() {
        threadRan = true;
    }, OS_THREAD_PRIORITY_DEFAULT, 4096);

    for(int tries = 5; !threadRan && tries >= 0; tries--) {
        delay(1);
    }

    testThread.dispose();

    assertTrue((bool)threadRan);
}

test(THREAD_02_thread_doesnt_leak_memory)
{
    // 1024 less to account for fragmentation and other allocations
    delay(1000);
    assertMoreOrEqual(System.freeMemory(), s_ram_free_before - 1024);
}

test(THREAD_03_SingleThreadedBlock)
{
    SINGLE_THREADED_BLOCK() {

    }
    SINGLE_THREADED_BLOCK() {

    }
}

test(THREAD_04_with_lock)
{
    WITH_LOCK(Serial) {

    }

    WITH_LOCK(Serial) {

    }

}

test(THREAD_05_try_lock)
{
    TRY_LOCK(Serial) {

    }
}

// With THREADING DISABLED, checks that behavior of Particle.process() is as expected
// when run from main loop and custom threads
test(THREAD_06_particle_process_behavior_when_threading_disabled)
{
    /* This test should only be run with threading disabled */
    if (system_thread_get_state(nullptr) == spark::feature::ENABLED) {
        skip();
        return;
    }

    // Make sure Particle is connected
    Particle.connect();
    waitFor(Particle.connected,HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME);
    assertTrue(Particle.connected());

    // Call disconnect from main thread
    Particle.disconnect();
    SCOPE_GUARD({
        // Make sure we restore cloud connection after exiting this test
        Particle.connect();
        waitFor(Particle.connected, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME);
        assertTrue(Particle.connected());
    });

    HAL_Delay_Milliseconds(2000); // wait sometime to confirm delay will not disconnect
    assertTrue(Particle.connected());

    // Run Particle.process from a custom thread
    Thread* th = new Thread("name", []{
            Particle.process();
        },OS_THREAD_PRIORITY_DEFAULT);
    assertTrue(bool(th));
    th->join();
    delete th;

    // Particle.process() should not do anything from custom thread, hence particle should be still connected
    assertTrue(Particle.connected());
    // Run Particle.process() from main thread
    Particle.process();
    HAL_Delay_Milliseconds(200);
    assertFalse(Particle.connected());
}

namespace {

volatile int test_val_fn1;
volatile int test_val_fn2;
static int test_val = 1;
void increment(void)
{
    test_val_fn1++;
}

void sys_particle_process_increment(void)
{
    Particle.process();
    test_val_fn2++;
}

void sys_thr_block(void)
{
    while(test_val) {
        HAL_Delay_Milliseconds(1);
    }
}

} // anonymous

// With THREADING ENABLED, checks that behavior of Particle.process() is as expected
// when run from system, app, and custom threads
test(THREAD_07_particle_process_behavior_when_threading_enabled)
{
    /* This test should only be run with threading disabled */
    if (system_thread_get_state(nullptr) == spark::feature::DISABLED) {
        skip();
        return;
    }

    // Make sure Particle is connected
    Particle.connect();
    waitFor(Particle.connected,HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME);
    assertTrue(Particle.connected());

    // Schedule function on application thread that does not run for sometime because of hard delays
    test_val_fn1 = 0;
    ActiveObjectBase* app = (ActiveObjectBase*)system_internal(0, nullptr); // Returns application thread instance
    std::function<void(void)> fn = increment;
    app->invoke_async(fn);
    HAL_Delay_Milliseconds(10);
    assertEqual((int)test_val_fn1, 0);

    // Schedule particle.process on system thread
    test_val_fn2 = 0;
    ActiveObjectBase* system = (ActiveObjectBase*)system_internal(1, nullptr); // Returns system thread instance
    system->invoke_async(std::function<void()>(sys_particle_process_increment));
    HAL_Delay_Milliseconds(10);
    assertEqual((int)test_val_fn2, 1);

    // Schedule function on system thread that blocks for a variable (test_val)
    ActiveObjectBase* system2 = (ActiveObjectBase*)system_internal(1, nullptr); // Returns system thread instance
    system2->invoke_async(std::function<void()>(sys_thr_block));
    HAL_Delay_Milliseconds(10);

    // Call disconnect from main thread
    Particle.disconnect();
    SCOPE_GUARD({
        // Make sure we unblock the system thread and restore cloud connection after exiting this test
        test_val = 0;
        Particle.connect();
        // Address this comment before merging! Replace 20s with 9m?
        waitFor(Particle.connected,HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME);
        assertTrue(Particle.connected());
    });

    // Run Particle.process from a custom thread
    Thread* th = new Thread("name", []{
            Particle.process();
    },OS_THREAD_PRIORITY_DEFAULT);
    assertTrue(bool(th));
    th->join();
    delete th;
    // Particle.process() should not do anything from custom thread, hence particle should be still connected
    assertTrue(Particle.connected());

    assertEqual((int)test_val_fn1, 0);
    // The increment task that was added to the Application Queue via invoke_async is not guaranteed to be the first and only item in the Application Thread Queue
    // Call process() repeatedly in order to ensure that all potential messages in the queue are consumed, including the increment function
    while (Particle.process());

    assertEqual((int)test_val_fn1, 1);
    // Unblock system thread
    test_val = 0;
    HAL_Delay_Milliseconds(5000);
    assertFalse(Particle.connected());
}

test(THREAD_08_newlib_reent_impure_ptr_changes_on_context_switch)
{
    extern uintptr_t link_global_data_start;
    extern uintptr_t link_global_data_end;

    extern uintptr_t link_bss_location;
    extern uintptr_t link_bss_end;

    volatile bool threadRan = false;
    volatile struct _reent* threadImpure = nullptr;
    struct _reent* testImpure = _impure_ptr;

    assertFalse(isInRangeIncl((uintptr_t)testImpure, (uintptr_t)&link_global_data_start, (uintptr_t)&link_global_data_end));
    assertFalse(isInRangeIncl((uintptr_t)testImpure, (uintptr_t)&link_bss_location, (uintptr_t)&link_bss_end));

    testThread = Thread("test", [&]() {
        threadImpure = _impure_ptr;
        threadRan = true;
    }, OS_THREAD_PRIORITY_DEFAULT, 4096);

    for(int tries = 5; !threadRan && tries >= 0; tries--) {
        delay(100);
    }
    testThread.dispose();

    assertTrue((bool)threadRan);
    assertNotEqual((uintptr_t)threadImpure, (uintptr_t)nullptr);
    assertNotEqual((uintptr_t)_impure_ptr, (uintptr_t)nullptr);
    assertNotEqual((uintptr_t)_impure_ptr, (uintptr_t)threadImpure);
    assertFalse(isInRangeIncl((uintptr_t)threadImpure, (uintptr_t)&link_global_data_start, (uintptr_t)&link_global_data_end));
    assertFalse(isInRangeIncl((uintptr_t)threadImpure, (uintptr_t)&link_bss_location, (uintptr_t)&link_bss_end));
    assertEqual((uintptr_t)testImpure, (uintptr_t)_impure_ptr);
}

// todo - test for SingleThreadedSection



volatile int timeout_called = 0;
void timeout()
{
    timeout_called++;
}

void waitForComplete(ApplicationWatchdog& wd)
{
    while (!wd.isComplete()) {
        HAL_Delay_Milliseconds(0);
    }
}

test(APPLICATION_WATCHDOG_01_fires_timeout)
{
    s_ram_free_before = System.freeMemory();
    timeout_called = 0;
    ApplicationWatchdog wd(50, timeout);
    HAL_Delay_Milliseconds(60);

    assertEqual((int)timeout_called, 1);
    waitForComplete(wd);
}

test(APPLICATION_WATCHDOG_02_doesnt_fire_when_app_checks_in)
{
    for (int x=0; x<APPLICATION_WDT_TEST_RUNS; x++) {
        timeout_called = 0;
        unsigned t = 100;
        uint32_t startTime;
        // LOG_DEBUG(INFO, "S %d", millis());
        ApplicationWatchdog wd(t, timeout, APPLICATION_WDT_STACK_SIZE);
        startTime = millis();
        // this for() loop should consume more than t(seconds), about 2x as much
        for (int i=0; i<10; i++) {
            HAL_Delay_Milliseconds(t/5);
            assertEqual((int)timeout_called, 0);
            application_checkin();
            os_thread_yield();
        }
        // LOG_DEBUG(INFO, "TIME: %d, R %d:%s", millis()-startTime, x, timeout_called?"pass":"fail");
        waitForComplete(wd);
        uint32_t endTime = millis();
        assertEqual((int)timeout_called, 1);
        const auto expectedLow = t * 3; // should be t*3
        const auto expectedHigh = t * 4;
        assertMoreOrEqual(endTime - startTime, expectedLow - (expectedLow / 50)); // -2%
        assertLessOrEqual(endTime - startTime, expectedHigh + (expectedHigh / 50)); // +2%
        // LOG_DEBUG(INFO, "E %d",endTime-startTime);
    }
}

test(APPLICATION_WATCHDOG_03_doesnt_leak_memory)
{
    // Give the system some time to get the resources back
    delay(500);

    // Before Photon/P1 Thread fixes were introduced, we would lose approximately 20k
    // of RAM due to 10 allocations of ApplicationWatchdog in APPLICATION_WATCHDOG_02.
    // Taking fragmentation and other potential allocations into consideration, 10k seems like
    // a good testing value.
    assertMoreOrEqual(System.freeMemory(), s_ram_free_before - ((APPLICATION_WDT_STACK_SIZE * APPLICATION_WDT_TEST_RUNS)/2));
}

#if defined(configMUTEX_MULTI_STEP_PRIORITY_DISINHERITANCE) && configMUTEX_MULTI_STEP_PRIORITY_DISINHERITANCE

struct ThreadPriority {
    uint32_t base;
    uint32_t prio;
};

ThreadPriority getThreadPriority() {
    os_thread_dump_info_t info = {};

    os_thread_dump(os_thread_current(nullptr), [](os_thread_dump_info_t* info, void* ptr) -> os_result_t {
        if (info) {
            memcpy(ptr, info, sizeof(*info));
        }
        return 0;
    }, &info);
    ThreadPriority p;
    p.base = info.base_priority;
    p.prio = info.priority;
    return p;
}

test(CONCURRENT_MUTEX_01_priority_inheritance_two_threads)
{
    struct State {
        State() = default;
        ~State() = default;
        Mutex mutex1;
        Mutex mutex2;
        bool state1 = false;
        bool state2 = false;
        volatile int done = 0;
    };

    State state;
    const auto startPriority = OS_THREAD_PRIORITY_DEFAULT + 1;

    Thread lowPriorityThread("low", [](void* ptr) -> void {
        auto state = (State*)ptr;
        SCOPE_GUARD({
            ++state->done;
        });

        // Acquire first mutex
        std::unique_lock<Mutex> mutex1(state->mutex1);
        // Check that we are at the base priority and nothing has affected us
        auto prio = getThreadPriority();
        assertEqual(prio.base, prio.prio);

        // Keep holding the first mutex and acquire the second one
        std::unique_lock<Mutex> mutex2(state->mutex2);
        // Somewhere at this point the second thread will try to acquire the second mutex
        // and because we are holding it and are of lower priority, our priority
        // should be bumped to that of thread2.
        delay(1000);

        // Verify that our priority has been bumped to that of thread2
        prio = getThreadPriority();
        assertEqual(prio.prio, prio.base + 1);

        // Unlock mutex2, this should allow the second thread to finally
        // acquire it.
        mutex2.unlock();

        // After we've released mutex2 and let the second thread to
        // acquire it, our priority should be dropped to the 'normal' or base priority
        // Validate that
        prio = getThreadPriority();
        assertEqual(prio.prio, prio.base);

        // Everything is good

        state->state1 = true;
    }, &state, startPriority);
    Thread highPriorityThread("high", [](void* ptr) -> void {
        auto state = (State*)ptr;
        SCOPE_GUARD({
            ++state->done;
        });

        // Wait a bit for thread1 to acquire both mutexes
        delay(500);

        // Check that we are at the base priority and nothing has affected us
        auto prio = getThreadPriority();
        assertEqual(prio.base, prio.prio);

        // Attempt to acquire the second mutex, at this point the first thread
        // should be holding it. Because we are at higher priority, the priority
        // of the first thread should be bumped.
        std::unique_lock<Mutex> mutex2(state->mutex2);

        // We've finally acquired the mutex and at this point the priority
        // of the first thread should have dropped back to the base priority

        // Our priority should stay the same throughout all of this (base)
        prio = getThreadPriority();
        assertEqual(prio.base, prio.prio);

        // Unlock second mutex
        mutex2.unlock();

        // Nothing should have changed, we should still be at our own base priority
        prio = getThreadPriority();
        assertEqual(prio.base, prio.prio);

        // All done
        state->state2 = true;
    }, &state, startPriority + 1);

    for (auto start = millis(); millis() - start <= 2000;) {
        if (state.done == 2) {
            break;
        }
        delay(100);
    }

    assertTrue(state.state1);
    assertTrue(state.state2);
}

test(CONCURRENT_MUTEX_02_priority_inheritance_three_threads)
{
    struct State {
        State() = default;
        ~State() = default;
        Mutex mutex1;
        Mutex mutex2;
        volatile int state = 0;
        bool state1 = false;
        bool state2 = false;
        bool state3 = false;
        volatile int done = 0;
    };

    State state;
    const auto startPriority = OS_THREAD_PRIORITY_DEFAULT + 1;

    Thread lowPriorityThread("low", [](void* ptr) -> void {
        auto state = (State*)ptr;
        SCOPE_GUARD({
            ++state->done;
        });

        // Acquire first mutex
        std::unique_lock<Mutex> mutex1(state->mutex1);
        // Check that we are at the base priority and nothing has affected us
        auto prio = getThreadPriority();
        assertEqual(prio.base, prio.prio);

        // Keep holding the first mutex and acquire the second one
        std::unique_lock<Mutex> mutex2(state->mutex2);
        // Somewhere at this point the high priority thread will try to acquire the mutex2
        // and middle priority thread will try to acquire the mutex1, our priority should
        // be bumped to that of high priority thread.
        state->state = 1;
        delay(2000);

        // Verify that our priority has been bumped to that of middle priority thread
        prio = getThreadPriority();
        assertEqual(prio.prio, prio.base + 1);

        state->state = 2;
        delay(2000);

        // Verify that our priority has been bumped to that of high priority thread
        prio = getThreadPriority();
        assertEqual(prio.prio, prio.base + 2);
        state->state = 3;

        // Unlock mutex2, this should allow the high priority thread to finally
        // acquire it, and our priority should be decreased to middle priority
        mutex2.unlock();
        delay(1000);
        prio = getThreadPriority();
        assertEqual(prio.prio, prio.base + 1);

        // Unlock mutex1, this should allow the middle priority thread to finally
        // acquire it, and our priority should be decreased to low priority
        mutex1.unlock();
        delay(1000);
        prio = getThreadPriority();
        assertEqual(prio.prio, prio.base);

        // Everything is good

        state->state1 = true;
    }, &state, startPriority);
    Thread middlePriorityThread("middle", [](void* ptr) -> void {
        auto state = (State*)ptr;
        SCOPE_GUARD({
            ++state->done;
        });

        auto start = millis();

        // Wait a bit for low priority thread to acquire both mutexes
        while (state->state == 0 && millis() - start <= 5000) {
            delay(10);
        }

        // Check that we are at the base priority and nothing has affected us
        auto prio = getThreadPriority();
        assertEqual(prio.base, prio.prio);

        // Attempt to acquire the mutex1, at this point the low priority thread
        // should be holding it. Because we are at higher priority, the priority
        // of the low priority thread should be bumped.
        std::unique_lock<Mutex> mutex1(state->mutex1);

        // We've finally acquired the mutex and at this point the priority
        // of the low priority thread should have dropped back to the base priority

        // Our priority should stay the same throughout all of this (base)
        prio = getThreadPriority();
        assertEqual(prio.base, prio.prio);

        // Unlock second mutex
        mutex1.unlock();

        // Nothing should have changed, we should still be at our own base priority
        prio = getThreadPriority();
        assertEqual(prio.base, prio.prio);

        // All done
        state->state2 = true;
    }, &state, startPriority + 1);
    Thread highPriorityThread("high", [](void* ptr) -> void {
        auto state = (State*)ptr;
        SCOPE_GUARD({
            ++state->done;
        });

        auto start = millis();

        // Wait a bit for low priority thread to acquire both mutexes
        while (state->state == 0 && millis() - start <= 5000) {
            delay(10);
        }

        // Check that we are at the base priority and nothing has affected us
        auto prio = getThreadPriority();
        assertEqual(prio.base, prio.prio);

        // Wait for middle thread to attempt to acquire the mutex
        while (state->state == 1) {
            delay(10);
        }

        // Attempt to acquire the second mutex, at this point the low priority thread
        // should be holding it. Because we are at higher priority, the priority
        // of the low priority thread should be bumped.
        std::unique_lock<Mutex> mutex2(state->mutex2);

        // We've finally acquired the mutex and at this point the priority
        // of the low priority thread should have dropped back to the base priority

        // Our priority should stay the same throughout all of this (base)
        prio = getThreadPriority();
        assertEqual(prio.base, prio.prio);

        // Unlock second mutex
        mutex2.unlock();

        // Nothing should have changed, we should still be at our own base priority
        prio = getThreadPriority();
        assertEqual(prio.base, prio.prio);

        // All done
        state->state3 = true;
    }, &state, startPriority + 2);

    for (auto start = millis(); millis() - start <= 10000;) {
        if (state.done == 3) {
            break;
        }
        delay(100);
    }

    assertTrue(state.state1);
    assertTrue(state.state2);
    assertTrue(state.state3);
}

#endif // defined(configMUTEX_MULTI_STEP_PRIORITY_DISINHERITANCE) && configMUTEX_MULTI_STEP_PRIORITY_DISINHERITANCE

#endif
