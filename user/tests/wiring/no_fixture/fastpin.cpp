
#include "application.h"
#include "unit-test/unit-test.h"



test(FASTPIN_01_MaxDuration_PinSet) {
    // Attempt pinSetFast and pinResetFast numerous times and check speed.
    // Duration went from about 62 -> 24 ticks per set on Gen2 with this change list

#if HAL_PLATFORM_GEN == 3
    // expected max ticks of pinSetFast / pinResetFast on Gen3
    const uint32_t MAX_DURATION_PINSET_TICKS = 70;
#elif HAL_PLATFORM_GEN == 2
    // expected max ticks of pinSetFast / pinResetFast on Gen2
    const uint32_t MAX_DURATION_PINSET_TICKS = 30;
#else
#error "No gpio fastpin timing benchmark yet measured for this platform"
#endif

    const uint32_t NUM_ITERATIONS = 100;
    uint32_t start, finish;

    ATOMIC_BLOCK()  {
        start = System.ticks();
        for (uint32_t i = 0; i < NUM_ITERATIONS; i++) {
            pinSetFast(D7);
            //pinResetFast(D7);
        }
        finish = System.ticks();
    }
    uint32_t duration = finish - start;
//    Serial.print("Set duration:");
//    Serial.println(duration);
    assertLessOrEqual(duration, NUM_ITERATIONS*MAX_DURATION_PINSET_TICKS);
}

test(FASTPIN_02_MaxDuration_PinReset) {
    // Attempt pinResetFast numerous times and check speed.

#if HAL_PLATFORM_GEN == 3
    // expected max ticks of pinResetFast on Gen3
    const uint32_t MAX_DURATION_PINRESET_TICKS = 70;
#elif HAL_PLATFORM_GEN == 2
    // expected max ticks of pinResetFast on Gen2
    const uint32_t MAX_DURATION_PINRESET_TICKS = 30;
#else
#error "No gpio fastpin timing benchmark yet measured for this platform"
#endif

    const uint32_t NUM_ITERATIONS = 100;
    uint32_t start, finish;

    ATOMIC_BLOCK()  {
        start = System.ticks();
        for (uint32_t i = 0; i < NUM_ITERATIONS; i++) {
            pinResetFast(D7);
        }
        finish = System.ticks();
    }
    uint32_t duration = finish - start;
//    Serial.print("Reset duration:");
//    Serial.println(duration);
    assertLessOrEqual(duration, NUM_ITERATIONS*MAX_DURATION_PINRESET_TICKS);
}



