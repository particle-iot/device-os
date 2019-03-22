/*
 * Copyright (c) 2019 Particle Industries, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "application.h"
#include "literals.h"
#include <functional>

SYSTEM_MODE(MANUAL);

using namespace particle::literals::chrono_literals;
using namespace std::literals::chrono_literals;

// No inlining, no optimizations. Simulating some dynalib function doing actual work based on passed argument
unsigned int __attribute__((noinline, optimize("O0"))) someFunctionDoingActualWork(system_tick_t n) {
    unsigned int v = 0;
    n /= 1000;
    while (n--) {
        v += n;
    }
    return v;
}

// Just as the name says: takes duration, casts to particle::chrono::milliseconds (which is system_tick_t-based),
// passes that as system_tick_t to someFunctionDoingActualWork()
template<typename Rep, typename Period>
auto someFunctionTakingDurationAndCastingImmediatelyToSystemTickTBasedMilliseconds(std::chrono::duration<Rep, Period> arg) {
    return someFunctionDoingActualWork(std::chrono::duration_cast<particle::chrono::milliseconds>(arg).count());
}

// Just as the name says: takes duration, casts to std::chrono::milliseconds (which is uint64_t-based),
// passes that to someFunctionDoingActualWork() that takes system_tick_t
template<typename Rep, typename Period>
auto someFunctionTakingDurationAndCastingToStdChronoMillisecondsFirstAndThenToSystemTickT(std::chrono::duration<Rep, Period> arg) {
    return someFunctionDoingActualWork(std::chrono::duration_cast<std::chrono::milliseconds>(arg).count());
}

// Just as the name says: takes a millisecond system_tick_t-based duration
// NOTE: calling this function with less than ms durations (e.g. 1000us) will not work, as opposed to function templates above
auto someFunctionTakingSystemTickTBasedMillisecondDuration(particle::chrono::milliseconds arg) {
    return someFunctionDoingActualWork(arg.count());
}

// Just as the name says: takes an std::chrono::milliseconds duration (which is uint64_t-based)
// NOTE: calling this function with less than ms durations (e.g. 1000us) will not work, as opposed to function templates above
auto someFunctionTakingStdMillisecondDuration(std::chrono::milliseconds arg) {
    return someFunctionDoingActualWork(arg.count());
}

// Returns microseconds per operation
template <typename F>
double profile(unsigned int iterations, double& result, F&& func) {
    double counter = 0;
    for (unsigned int i = 0; i < iterations; ++i) {
        // Disable interrupts and thread scheduling so that they don't interfere
        SINGLE_THREADED_BLOCK() {
            // NOTE: we are using CPSID/CPSIE here because HAL_disable_irq() will not disable SoftDevice interrupts
            // on Gen 3 devices
            __disable_irq();
            // Also disable SysTick
            SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
            auto tStart = SYSTEM_TICK_COUNTER;
            auto v = func();
            (void)v;
            auto tEnd = SYSTEM_TICK_COUNTER;
            // Re-enable SysTick
            SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
            // Re-enable interrupts
            __enable_irq();
            counter += tEnd - tStart;
        }        
    }

    // Convert to microseconds
    result = counter / (double)SYSTEM_US_TICKS / (double)iterations;
    return result;
}

template <typename F>
void runTest(F&& func, const char* type, double& result, double& referenceValue, unsigned int iterations = 10000) {
    profile(iterations, result, func);
    Serial.printlnf("%s:\t%lf\t%3.6lf%%", type, result, result / referenceValue * 100.0);
}

/* executes once at startup */
void setup() {
#ifndef SIZE_TEST
    waitUntil(Serial.isConnected);
    delay(1000);

    double refUs;
    double refMs;
    double refS;
    double refMin;
    double refH;
    double tmp;

    // system_tick_t-based literals
    Serial.printlnf("system_tick_t-based literals:");
    Serial.printlnf("someFunctionTakingDurationAndCastingImmediatelyToSystemTickTBasedMilliseconds:");
    runTest([]() { return someFunctionTakingDurationAndCastingImmediatelyToSystemTickTBasedMilliseconds(3600000000_us); }, "us", refUs, refUs);
    runTest([]() { return someFunctionTakingDurationAndCastingImmediatelyToSystemTickTBasedMilliseconds(3600000_ms); }, "ms", refMs, refMs);
    runTest([]() { return someFunctionTakingDurationAndCastingImmediatelyToSystemTickTBasedMilliseconds(3600_s); }, "s", refS, refS);
    runTest([]() { return someFunctionTakingDurationAndCastingImmediatelyToSystemTickTBasedMilliseconds(60_min); }, "min", refMin, refMin);
    runTest([]() { return someFunctionTakingDurationAndCastingImmediatelyToSystemTickTBasedMilliseconds(1_h); }, "h", refH, refH);

    Serial.printlnf("someFunctionTakingDurationAndCastingToStdChronoMillisecondsFirstAndThenToSystemTickT:");
    runTest([]() { return someFunctionTakingDurationAndCastingToStdChronoMillisecondsFirstAndThenToSystemTickT(3600000000_us); }, "us", tmp, refUs);
    runTest([]() { return someFunctionTakingDurationAndCastingToStdChronoMillisecondsFirstAndThenToSystemTickT(3600000_ms); }, "ms", tmp, refMs);
    runTest([]() { return someFunctionTakingDurationAndCastingToStdChronoMillisecondsFirstAndThenToSystemTickT(3600_s); }, "s", tmp, refS);
    runTest([]() { return someFunctionTakingDurationAndCastingToStdChronoMillisecondsFirstAndThenToSystemTickT(60_min); }, "min", tmp, refMin);
    runTest([]() { return someFunctionTakingDurationAndCastingToStdChronoMillisecondsFirstAndThenToSystemTickT(1_h); }, "h", tmp, refH);

    Serial.printlnf("someFunctionTakingSystemTickTBasedMillisecondDuration:");
    Serial.printlnf("us:\twon't work");
    // runTest([]() { return someFunctionTakingSystemTickTBasedMillisecondDuration(3600000000_us); }, "us", tmp, refUs);
    runTest([]() { return someFunctionTakingSystemTickTBasedMillisecondDuration(3600000_ms); }, "ms", tmp, refMs);
    runTest([]() { return someFunctionTakingSystemTickTBasedMillisecondDuration(3600_s); }, "s", tmp, refS);
    runTest([]() { return someFunctionTakingSystemTickTBasedMillisecondDuration(60_min); }, "min", tmp, refMin);
    runTest([]() { return someFunctionTakingSystemTickTBasedMillisecondDuration(1_h); }, "h", tmp, refH);

    Serial.printlnf("someFunctionTakingStdMillisecondDuration:");
    Serial.printlnf("us:\twon't work");
    // runTest([]() { return someFunctionTakingStdMillisecondDuration(3600000000_us); }, "us", tmp, refUs);
    runTest([]() { return someFunctionTakingStdMillisecondDuration(3600000_ms); }, "ms", tmp, refMs);
    runTest([]() { return someFunctionTakingStdMillisecondDuration(3600_s); }, "s", tmp, refS);
    runTest([]() { return someFunctionTakingStdMillisecondDuration(60_min); }, "min", tmp, refMin);
    runTest([]() { return someFunctionTakingStdMillisecondDuration(1_h); }, "h", tmp, refH);
    Serial.println();

    // std literals
    Serial.printlnf("std literals:");
    Serial.printlnf("someFunctionTakingDurationAndCastingImmediatelyToSystemTickTBasedMilliseconds:");
    runTest([]() { return someFunctionTakingDurationAndCastingImmediatelyToSystemTickTBasedMilliseconds(3600000000us); }, "us", tmp, refUs);
    runTest([]() { return someFunctionTakingDurationAndCastingImmediatelyToSystemTickTBasedMilliseconds(3600000ms); }, "ms", tmp, refMs);
    runTest([]() { return someFunctionTakingDurationAndCastingImmediatelyToSystemTickTBasedMilliseconds(3600s); }, "s", tmp, refS);
    runTest([]() { return someFunctionTakingDurationAndCastingImmediatelyToSystemTickTBasedMilliseconds(60min); }, "min", tmp, refMin);
    runTest([]() { return someFunctionTakingDurationAndCastingImmediatelyToSystemTickTBasedMilliseconds(1h); }, "h", tmp, refH);

    Serial.printlnf("someFunctionTakingDurationAndCastingToStdChronoMillisecondsFirstAndThenToSystemTickT:");
    runTest([]() { return someFunctionTakingDurationAndCastingToStdChronoMillisecondsFirstAndThenToSystemTickT(3600000000us); }, "us", tmp, refUs);
    runTest([]() { return someFunctionTakingDurationAndCastingToStdChronoMillisecondsFirstAndThenToSystemTickT(3600000ms); }, "ms", tmp, refMs);
    runTest([]() { return someFunctionTakingDurationAndCastingToStdChronoMillisecondsFirstAndThenToSystemTickT(3600s); }, "s", tmp, refS);
    runTest([]() { return someFunctionTakingDurationAndCastingToStdChronoMillisecondsFirstAndThenToSystemTickT(60min); }, "min", tmp, refMin);
    runTest([]() { return someFunctionTakingDurationAndCastingToStdChronoMillisecondsFirstAndThenToSystemTickT(1h); }, "h", tmp, refH);

    Serial.printlnf("someFunctionTakingSystemTickTBasedMillisecondDuration:");
    Serial.printlnf("us:\twon't work");
    // runTest([]() { return someFunctionTakingSystemTickTBasedMillisecondDuration(3600000000us); }, "us", tmp, refUs);
    runTest([]() { return someFunctionTakingSystemTickTBasedMillisecondDuration(3600000ms); }, "ms", tmp, refMs);
    runTest([]() { return someFunctionTakingSystemTickTBasedMillisecondDuration(3600s); }, "s", tmp, refS);
    runTest([]() { return someFunctionTakingSystemTickTBasedMillisecondDuration(60min); }, "min", tmp, refMin);
    runTest([]() { return someFunctionTakingSystemTickTBasedMillisecondDuration(1h); }, "h", tmp, refH);

    Serial.printlnf("someFunctionTakingStdMillisecondDuration:");
    Serial.printlnf("us:\twon't work");
    // runTest([]() { return someFunctionTakingStdMillisecondDuration(3600000000us); }, "us", tmp, refUs);
    runTest([]() { return someFunctionTakingStdMillisecondDuration(3600000ms); }, "ms", tmp, refMs);
    runTest([]() { return someFunctionTakingStdMillisecondDuration(3600s); }, "s", tmp, refS);
    runTest([]() { return someFunctionTakingStdMillisecondDuration(60min); }, "min", tmp, refMin);
    runTest([]() { return someFunctionTakingStdMillisecondDuration(1h); }, "h", tmp, refH);
    Serial.println();
#endif // SIZE_TEST
}

/* executes continuously after setup() runs */
void loop() {
#ifdef SIZE_TEST
#warning Binary size test only
    double dummy;

#ifdef SIZE_TEST_SYSTEM_TICK_T_LITERALS
#warning Using system_tick_t-based literals

    runTest([]() { return someFunctionTakingDurationAndCastingImmediatelyToSystemTickTBasedMilliseconds(3600000000_us); }, "us", dummy, dummy);
    runTest([]() { return someFunctionTakingDurationAndCastingImmediatelyToSystemTickTBasedMilliseconds(3600000_ms); }, "ms", dummy, dummy);
    runTest([]() { return someFunctionTakingDurationAndCastingImmediatelyToSystemTickTBasedMilliseconds(3600_s); }, "s", dummy, dummy);
    runTest([]() { return someFunctionTakingDurationAndCastingImmediatelyToSystemTickTBasedMilliseconds(60_min); }, "min", dummy, dummy);
    runTest([]() { return someFunctionTakingDurationAndCastingImmediatelyToSystemTickTBasedMilliseconds(1_h); }, "h", dummy, dummy);

    // runTest([]() { return someFunctionTakingSystemTickTBasedMillisecondDuration(3600000000_us); }, "us", dummy, dummy);
    runTest([]() { return someFunctionTakingSystemTickTBasedMillisecondDuration(3600000_ms); }, "ms", dummy, dummy);
    runTest([]() { return someFunctionTakingSystemTickTBasedMillisecondDuration(3600_s); }, "s", dummy, dummy);
    runTest([]() { return someFunctionTakingSystemTickTBasedMillisecondDuration(60_min); }, "min", dummy, dummy);
    runTest([]() { return someFunctionTakingSystemTickTBasedMillisecondDuration(1_h); }, "h", dummy, dummy);

#else
#warning Using std literals

    runTest([]() { return someFunctionTakingDurationAndCastingToStdChronoMillisecondsFirstAndThenToSystemTickT(3600000000us); }, "us", dummy, dummy);
    runTest([]() { return someFunctionTakingDurationAndCastingToStdChronoMillisecondsFirstAndThenToSystemTickT(3600000ms); }, "ms", dummy, dummy);
    runTest([]() { return someFunctionTakingDurationAndCastingToStdChronoMillisecondsFirstAndThenToSystemTickT(3600s); }, "s", dummy, dummy);
    runTest([]() { return someFunctionTakingDurationAndCastingToStdChronoMillisecondsFirstAndThenToSystemTickT(60min); }, "min", dummy, dummy);
    runTest([]() { return someFunctionTakingDurationAndCastingToStdChronoMillisecondsFirstAndThenToSystemTickT(1h); }, "h", dummy, dummy);

    // runTest([]() { return someFunctionTakingSystemTickTBasedMillisecondDuration(3600000000us); }, "us", dummy, dummy);
    runTest([]() { return someFunctionTakingSystemTickTBasedMillisecondDuration(3600000ms); }, "ms", dummy, dummy);
    runTest([]() { return someFunctionTakingSystemTickTBasedMillisecondDuration(3600s); }, "s", dummy, dummy);
    runTest([]() { return someFunctionTakingSystemTickTBasedMillisecondDuration(60min); }, "min", dummy, dummy);
    runTest([]() { return someFunctionTakingSystemTickTBasedMillisecondDuration(1h); }, "h", dummy, dummy);

#endif // SIZE_TEST_SYSTEM_TICK_T_LITERALS

#endif // SIZE_TEST
}
