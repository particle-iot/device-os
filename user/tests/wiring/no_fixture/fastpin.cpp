#include "application.h"
#include "unit-test/unit-test.h"

test(FASTPIN_01_MaxDuration_PinSet) {
    // Attempt pinSetFast numerous times and check speed.

#if HAL_PLATFORM_GEN == 3
#if PLATFORM_ID == PLATFORM_ESOMX
    const pin_t pin = D5;
#else
    const pin_t pin = D7;
#endif // PLATFORM_ID == PLATFORM_ESOMX

#if HAL_PLATFORM_RTL872X
    const uint32_t MAX_DURATION_PINSET_NS = 3020 * 110 / 100; // 10% higher than measured (includes for() loop overhead)
#else
    const uint32_t MAX_DURATION_PINSET_NS = 1030 * 110 / 100; // 10% higher than measured (includes for() loop overhead)
#endif // HAL_PLATFORM_RTL872X
#else
#error "No gpio fastpin timing benchmark yet measured for this platform"
#endif // HAL_PLATFORM_GEN == 3

    SCOPE_GUARD ({
        pinMode(pin, INPUT);
    });

    const uint32_t NUM_ITERATIONS = 100;
    uint32_t start, finish;
    pinMode(pin, OUTPUT);

    ATOMIC_BLOCK()  {
        start = HAL_Timer_Get_Micro_Seconds() * 1000;
        for (uint32_t i = 0; i < NUM_ITERATIONS; i++) {
            pinSetFast(pin);
            // pinResetFast(pin);
        }
        finish = HAL_Timer_Get_Micro_Seconds() * 1000;
    }
    uint32_t duration = finish - start;
    uint32_t expected = NUM_ITERATIONS * MAX_DURATION_PINSET_NS;
    // Serial.printlnf("pinSetFast total duration: %lu vs. expected max duration: %lu", duration, expected);
    assertNotEqual(duration, 0);
    assertLessOrEqual(duration, expected);
}

test(FASTPIN_02_MaxDuration_PinReset) {
    // Attempt pinResetFast numerous times and check speed.

#if HAL_PLATFORM_GEN == 3
#if PLATFORM_ID == PLATFORM_ESOMX
    const pin_t pin = D5;
#else
    const pin_t pin = D7;
#endif // PLATFORM_ID == PLATFORM_ESOMX

#if HAL_PLATFORM_RTL872X
    const uint32_t MAX_DURATION_PINRESET_NS = 2970 * 110 / 100; // 10% higher than measured (includes for() loop overhead)
#else
    const uint32_t MAX_DURATION_PINRESET_NS = 1030 * 110 / 100; // 10% higher than measured (includes for() loop overhead)
#endif // HAL_PLATFORM_RTL872X
#else
#error "No gpio fastpin timing benchmark yet measured for this platform"
#endif // HAL_PLATFORM_GEN == 3

    SCOPE_GUARD ({
        pinMode(pin, INPUT);
    });

    const uint32_t NUM_ITERATIONS = 100;
    uint32_t start, finish;
    pinMode(pin, OUTPUT);

    ATOMIC_BLOCK()  {
        start = HAL_Timer_Get_Micro_Seconds() * 1000;
        for (uint32_t i = 0; i < NUM_ITERATIONS; i++) {
            // pinSetFast(pin);
            pinResetFast(pin);
        }
        finish = HAL_Timer_Get_Micro_Seconds() * 1000;
    }
    uint32_t duration = finish - start;
    uint32_t expected = NUM_ITERATIONS * MAX_DURATION_PINRESET_NS;
    // Serial.printlnf("pinResetFast total duration: %lu vs. expected max duration: %lu", duration, expected);
    assertNotEqual(duration, 0);
    assertLessOrEqual(duration, expected);
}
