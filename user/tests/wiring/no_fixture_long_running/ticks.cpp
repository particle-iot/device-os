#include "spark_wiring_system.h"

#include "application.h"
#include "unit-test/unit-test.h"

#include <cstdlib>
#include "random.h"

#if HAL_PLATFORM_RTL872X
#ifdef AMEBAD_TODO
#define __ARMV8MML_REV                 0x0000U  /*!< ARMV8MML Core Revision                                                    */
#define __Vendor_SysTickConfig         0        /*!< Set to 1 if different SysTick Config is used                              */
#define __VTOR_PRESENT                 1        /*!< Set to 1 if CPU supports Vector Table Offset Register                     */
#define __FPU_DP                       0        /*!< Double Precision FPU                                                      */
#endif
#define __CM3_REV                      0x0200    /**< Core revision r0p0 */
#define __MPU_PRESENT                  1         /**< Defines if an MPU is present or not */
#define __NVIC_PRIO_BITS               3         /**< Number of priority bits implemented in the NVIC */
#define __Vendor_SysTickConfig         1         /**< Vendor specific implementation of SysTickConfig is defined *///see vPortSetupTimerInterrupt
#define __SAUREGION_PRESENT            1        /*!< SAU present or not                                                        */

#define __FPU_PRESENT             1       /*!< FPU present                                   */
#define __VFP_FP__	1
#ifndef __ARM_FEATURE_CMSE
#define __ARM_FEATURE_CMSE	3
#endif
#include <arm_cmse.h>   /* Use CMSE intrinsics */
#include "core_armv8mml.h"
#endif // HAL_PLATFORM_RTL872X

namespace {

#ifndef PARTICLE_TEST_RUNNER
const int MILLIS_MICROS_MAX_DIFF_US = 1500;
#else
const int MILLIS_MICROS_MAX_DIFF_US = 2000;
#endif // PARTICLE_TEST_RUNNER

struct TicksAtomic {
    TicksAtomic() {
        pri = __get_PRIMASK();
        __disable_irq();
    }
    ~TicksAtomic() {
        __set_PRIMASK(pri);
    }

    int pri;
};
#define TICKS_ATOMIC_BLOCK() for (bool __todo=true; __todo;) for (TicksAtomic __as; __todo; __todo=false)

}

void assert_micros_millis(int duration, bool overflow = false)
{
    system_tick_t last_millis_64 = System.millis();
    system_tick_t last_millis = millis();
    system_tick_t last_micros = micros();

    do
    {
        uint64_t now_millis_64;
        system_tick_t now_millis, now_micros;
        // FIXME: acquiring these three atomically, as there is no guarantee
        // that we are not interrupted/pre-empted between the calls, which breaks
        // the assumptions below.
        TICKS_ATOMIC_BLOCK() {
            now_millis_64 = System.millis();
            now_millis = millis();
            now_micros = micros();
        }

        if (!overflow) {
            assertMoreOrEqual(now_millis_64, last_millis_64);
            assertMoreOrEqual(now_millis, last_millis);
            assertMoreOrEqual(now_micros, last_micros);
        }

        // micros always at least (millis()*1000)
        // even with overflow
        assertMoreOrEqual(now_micros, now_millis * 1000);
        // at most 1.5ms difference between micros() and millis()
        int diff = (int32_t)now_micros - (int32_t)now_millis * 1000;
        assertLessOrEqual(diff, MILLIS_MICROS_MAX_DIFF_US);
        // at most 1ms difference between millis() and lower 32 bits of System.millis()
        diff = std::abs((int64_t)now_millis - (int64_t)(now_millis_64 & 0xffffffffull));
        assertLessOrEqual(diff, MILLIS_MICROS_MAX_DIFF_US / 1000);

        duration -= now_millis - last_millis;

        last_millis_64 = now_millis_64;
        last_millis = now_millis;
        last_micros = now_micros;
    }
    while (duration > 0);
}

void assert_micros_millis_interrupts(int duration)
{
    // TODO: nRF52 platforms
    uint64_t last_millis_64 = System.millis();
    system_tick_t last_millis = millis();
    system_tick_t last_micros = micros();

    system_tick_t last_relax = millis();

    do
    {
        uint64_t now_millis_64;
        system_tick_t now_millis, now_micros;
        // FIXME: acquiring these three atomically, as there is no guarantee
        // that we are not interrupted/pre-empted between the calls, which breaks
        // the assumptions below.
        TICKS_ATOMIC_BLOCK() {
            now_millis_64 = System.millis();
            now_millis = millis();
            now_micros = micros();
        }

        assertMoreOrEqual(now_millis_64, last_millis_64);
        assertMoreOrEqual(now_millis, last_millis);
        assertMoreOrEqual(now_micros, last_micros);

        // micros always at least (millis()*1000)
        // even with overflow
        assertMoreOrEqual(now_micros, now_millis * 1000);
        // at most 1.5ms difference between micros() and millis()
        int diff = (int32_t)now_micros - (int32_t)now_millis * 1000;
        assertLessOrEqual(diff, MILLIS_MICROS_MAX_DIFF_US);
        // at most 1ms difference between millis() and lower 32 bits of System.millis()
        diff = std::abs((int64_t)now_millis - (int64_t)(now_millis_64 & 0xffffffffull));
        assertLessOrEqual(diff, MILLIS_MICROS_MAX_DIFF_US);

        duration -= now_millis - last_millis;

        last_millis_64 = now_millis_64;
        last_millis = now_millis;
        last_micros = now_micros;

#ifdef PARTICLE_TEST_RUNNER
        if (millis() - last_relax >= 10000) {
            for (int i = 0; i < 10; i++) {
                Particle.process();
                delay(10);
            }
            last_relax = millis();
        }
#endif // PARTICLE_TEST_RUNNER
    }
    while (duration > 0);
}

test(TICKS_00_prepare)
{
    // Make sure we are disconnected from the cloud/network
    Particle.disconnect();
    Network.disconnect();
    delay(5000);
}

test(TICKS_01_millis_micros_baseline_test)
{
    const unsigned DELAY = 3 * 1000;
    // delay()
    uint64_t startMillis64 = System.millis();
    system_tick_t startMillis = millis();
    system_tick_t startMicros = micros();
    delay(DELAY);
    assertMoreOrEqual(System.millis() - startMillis64, DELAY);
    assertMoreOrEqual(millis() - startMillis, DELAY);
    assertMoreOrEqual(micros() - startMicros, DELAY * 1000);
    // delayMicroseconds()
    startMillis64 = System.millis();
    startMillis = millis();
    startMicros = micros();
    delayMicroseconds(DELAY);
    assertMoreOrEqual(System.millis() - startMillis64, DELAY / 1000);
    assertMoreOrEqual(millis() - startMillis, DELAY / 1000);
    assertMoreOrEqual(micros() - startMicros, DELAY);
}

// TODO millis_and_micros_rollover test for nRF52 platforms

// // the __advance_system1MsTick isn't dynamically linked so we build this as a monolithic app
// #include "hw_ticks.h"
// test(TICKS_02_millis_and_micros_rollover)
// {
// }
// #endif

test(TICKS_02_millis_and_micros_along_with_high_priority_interrupts)
{
    const system_tick_t TWO_MINUTES = 2 * 60 * 1000;
    system_tick_t start = millis();
    assert_micros_millis_interrupts(TWO_MINUTES);
    assertMoreOrEqual(millis()-start, TWO_MINUTES);
}

test(TICKS_03_millis_and_micros_monotonically_increases)
{
    const system_tick_t TWO_MINUTES = 2 * 60 * 1000;
    system_tick_t start = millis();
    assert_micros_millis(TWO_MINUTES);
    assertMoreOrEqual(millis()-start, TWO_MINUTES);
}
