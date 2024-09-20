#include <algorithm>
#include <limits>
#include <cmath>

#include "system_threading.h"
#include "scope_guard.h"

#include "application.h"
#include "unit-test/unit-test.h"

namespace {

os_semaphore_t g_sysThreadSuspendSem = nullptr;
int g_sysThreadSuspendCount = 0;

void suspendSystemThread() {
    if (!system_thread_get_state(nullptr /* reserved */)) {
        return;
    }
    if (++g_sysThreadSuspendCount > 1) {
        return;
    }
    if (!g_sysThreadSuspendSem) {
        int r = os_semaphore_create(&g_sysThreadSuspendSem, 0xffffffffu /* max_count */, 0 /* initial_count */);
        SPARK_ASSERT(r == 0);
    }
    auto sysThread = (ActiveObjectBase*)system_internal(1 /* item */, nullptr /* reserved */);
    std::function fn = [&]() {
        int r = os_semaphore_take(g_sysThreadSuspendSem, 0xffffffffu /* timeout */, false /* reserved */);
        SPARK_ASSERT(r == 0);
    };
    sysThread->invoke_async(std::move(fn));
    HAL_Delay_Milliseconds(1);
}

void resumeSystemThread() {
    if (!system_thread_get_state(nullptr /* reserved */)) {
        return;
    }
    if (--g_sysThreadSuspendCount > 0) {
        return;
    }
    SPARK_ASSERT(g_sysThreadSuspendCount == 0);
    int r = os_semaphore_give(g_sysThreadSuspendSem, false /* reserved */);
    SPARK_ASSERT(r == 0);
}

} // namespace

test(DELAY_01_init) {
    Network.off();
}

test(DELAY_02_accuracy_is_within_tolerance) {
    const unsigned callCount = 1000;

    Vector<unsigned> variations;
    assertTrue(variations.resize(callCount));

    for (unsigned i = 0; i < callCount; ++i) {
        auto duration = rand() % 9 + 2; // 2-10ms
        auto t1 = micros();
        delay(duration);
        auto dt = micros() - t1;

        assertMoreOrEqual(dt, duration * 1000); // delay() must never return early
        variations[i] = dt - duration * 1000;

        Particle.process();

        // Wait for the beginning of a tick and then for some arbitrary time within that tick
        HAL_Delay_Milliseconds(1);
        delayMicroseconds(rand() % 1000);
    }

    std::sort(variations.begin(), variations.end());
    auto dtPct95 = variations[std::floor(callCount * 0.95)]; // 95th percentile variation, us
    auto dtPct99 = variations[std::floor(callCount * 0.99)]; // 99th percentile variation, us
    auto dtMax = variations.last(); // Maximum variation, us (1 out of 1000)
    // String temp = String::format("%u:%u:%u:%u:%u:%u:%u:%u:%u:%u:%u",
    //     variations[callCount-100],
    //     variations[callCount-90],
    //     variations[callCount-80],
    //     variations[callCount-70],
    //     variations[callCount-60],
    //     variations[callCount-50],
    //     variations[callCount-40],
    //     variations[callCount-30],
    //     variations[callCount-20],
    //     variations[callCount-10],
    //     variations[callCount-1]);

    assertLess(dtPct95, 100); // 95th percentile
    assertLess(dtPct99, 200); // 99th percentile
    assertLess(dtMax, 3000); // 1 out of 1000

    // DEBUG: Can replace dtMax with temp string of more data (which also includes dtMax)
    // pushMailboxMsg(String::format("%u,%u,%s", dtPct95, dtPct99, temp.c_str()), 5000 /* wait */);
    pushMailboxMsg(String::format("%u,%u,%u", dtPct95, dtPct99, dtMax), 5000 /* wait */);
}

test(DELAY_03_app_events_are_processed_at_expected_rate_in_threaded_mode) {
    if (!system_thread_get_state(nullptr /* reserved */)) {
        skip();
        return;
    }

    // Prevent the system from sending events to the app while the test is running
    suspendSystemThread();
    SCOPE_GUARD({
        resumeSystemThread();
    });

    // Flush the event queue
    while (Particle.process()) {
    }

    auto appThread = (ActiveObjectBase*)system_internal(0 /* item */, nullptr /* reserved */);

    auto lastEventTime = millis();
    unsigned minEventInterval = std::numeric_limits<unsigned>::max();
    unsigned maxEventInterval = 0;
    unsigned eventCount = 0;
    bool running = true;

    std::function<void()> fn;
    fn = [&]() {
        if (!running) {
            return;
        }
        auto now = millis();
        auto dt = now - lastEventTime;
        lastEventTime = now;
        if (dt < minEventInterval) {
            minEventInterval = dt;
        }
        if (dt > maxEventInterval) {
            maxEventInterval = dt;
        }
        ++eventCount;
        appThread->invoke_async(fn);
    };

    appThread->invoke_async(fn);

    auto startTime = millis();
    do {
        delay(rand() % 3000);
    } while (millis() - startTime < 10000);

    running = false;
    while (Particle.process()) {
    }

    assertMoreOrEqual(minEventInterval, 999); // The maintained interval is not sub-tick precise
    assertLessOrEqual(maxEventInterval, 1005);
    assertMoreOrEqual(eventCount, 9);
}
