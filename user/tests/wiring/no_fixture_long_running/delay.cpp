#include <algorithm>
#include <cmath>

#include "application.h"
#include "unit-test/unit-test.h"

test(DELAY_01_init) {
    Network.off();
}

test(DELAY_02_accuracy_is_within_tolerance) {
    const unsigned minDelay = 2; // ms
    const unsigned maxDelay = 10; // ms
    const unsigned callCount = 1000;

    Vector<unsigned> variations;
    assertTrue(variations.resize(callCount));

    for (unsigned i = 0; i < callCount; ++i) {
        auto duration = rand() % (maxDelay - 1) + minDelay;
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
    auto dtPct = variations[std::floor(callCount * 0.995)]; // Nth percentile
    auto dtMax = variations.last();

    assertLess(dtPct, 100); // us
    assertLess(dtMax, 1000); // us

    pushMailboxMsg(String::format("%u,%u", dtPct, dtMax), 5000 /* wait */);
}
