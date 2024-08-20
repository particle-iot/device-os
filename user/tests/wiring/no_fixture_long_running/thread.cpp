#include <atomic>

#include "application.h"
#include "random.h"
#include "unit-test/unit-test.h"

test(THREAD_01_delay_does_not_introduce_extra_latency_when_processing_app_events)
{
    // This test should only be run with threading enabled
    if (!system_thread_get_state(nullptr)) {
        skip();
        return;
    }

    struct TestData {
        system_tick_t eventSendTime;
        system_tick_t* maxEventDelay;
        int* eventRecvCount;

        static void callback(void* data) {
            auto now = HAL_Timer_Milliseconds();
            std::unique_ptr<TestData> d(static_cast<TestData*>(data));
            auto delay = now - d->eventSendTime;
            if (delay > *d->maxEventDelay) {
                *d->maxEventDelay = delay;
            }
            ++*d->eventRecvCount;
        }
    };

    system_tick_t maxEventDelay = 0;
    int eventSendCount = 0;
    int eventRecvCount = 0;

    volatile bool stop = false;
    volatile bool failed = false;

    Thread thread("test", [&]() {
        auto startTime = HAL_Timer_Milliseconds();
        system_tick_t now = 0;
        Random rand;
        do {
            HAL_Delay_Milliseconds((rand.gen<unsigned>() % 400) + 100);
            std::unique_ptr<TestData> d(new(std::nothrow) TestData());
            if (!d) {
                failed = true;
                break;
            }
            d->maxEventDelay = &maxEventDelay;
            d->eventRecvCount = &eventRecvCount;
            now = HAL_Timer_Milliseconds();
            d->eventSendTime = now;
            application_thread_invoke(TestData::callback, d.release(), nullptr);
            ++eventSendCount;
        } while (now - startTime < 10000);
        stop = true;
    });

    unsigned delayMillis = 0;
    system_tick_t elapsedMillis = 0;
    do {
        auto ms = (rand() % 2900) + 100;
        auto t1 = HAL_Timer_Milliseconds();
        delay(ms);
        elapsedMillis += HAL_Timer_Milliseconds() - t1;
        delayMillis += ms;
    } while (!stop);

    thread.join();
    Particle.process();

    assertFalse(!!failed);
    assertMoreOrEqual(elapsedMillis, delayMillis);
    assertLessOrEqual(elapsedMillis - delayMillis, 5);
    assertLessOrEqual(maxEventDelay, 2);
    assertMore(eventSendCount, 10);
    assertEqual(eventRecvCount, eventSendCount);
}
