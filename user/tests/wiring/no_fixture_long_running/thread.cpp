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
        system_tick_t sendTime;
        system_tick_t* maxDelay;
        int* recvCount;

        static void callback(void* data) {
            auto recvTime = HAL_Timer_Milliseconds();
            std::unique_ptr<TestData> d(static_cast<TestData*>(data));
            auto delay = recvTime - d->sendTime;
            if (delay > *d->maxDelay) {
                *d->maxDelay = delay;
            }
            ++*d->recvCount;
        }
    };

    system_tick_t maxDelay = 0;
    int sendCount = 0;
    int recvCount = 0;

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
            d->maxDelay = &maxDelay;
            d->recvCount = &recvCount;
            now = HAL_Timer_Milliseconds();
            d->sendTime = now;
            ++sendCount;
            application_thread_invoke(TestData::callback, d.release(), nullptr);
        } while (now - startTime < 10000);
        stop = true;
    });

    do {
        delay((rand() % 2900) + 100);
    } while (!stop);

    thread.join();
    Particle.process();

    assertFalse(!!failed);
    assertLessOrEqual(maxDelay, 2);
    assertMore(sendCount, 10);
    assertEqual(recvCount, sendCount);
}
