#include "spark_wiring_system.h"

#include "application.h"
#include "unit-test/unit-test.h"

#include <cstdlib>

void assert_micros_millis(int duration, bool overflow = false)
{
#if PLATFORM_ID==0 || (PLATFORM_ID>=6 && PLATFORM_ID<=10)
    // Just in case
    NVIC_DisableIRQ(TIM4_IRQn);
#endif

    system_tick_t last_millis_64 = System.millis();
    system_tick_t last_millis = millis();
    system_tick_t last_micros = micros();

    do
    {
        system_tick_t now_millis_64 = System.millis();
        system_tick_t now_millis = millis();
        system_tick_t now_micros = micros();

        if (!overflow) {
            assertMoreOrEqual(now_millis_64, last_millis_64);
            assertMoreOrEqual(now_millis, last_millis);
            assertMoreOrEqual(now_micros, last_micros);
        }

        // micros always at least (millis()*1000)
        // even with overflow
        assertMoreOrEqual(now_micros, now_millis * 1000);
        // at most 1ms difference between millis() and lower 32 bits of System.millis()
        assertTrue(std::abs((int64_t)now_millis - (int64_t)(now_millis_64 & 0xffffffffull)) <= 1);

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
#if PLATFORM_ID==0 || (PLATFORM_ID>=6 && PLATFORM_ID<=10)
    // Enable some high priority interrupt to run interference
    pinMode(D0, OUTPUT);
    // D0 uses TIM2 and channel 2 equally on Core and Photon/Electron
    // Run at 4khz (T=250us)
    analogWrite(D0, 1, 4000);
    randomSeed(micros());
    // Set higher than SysTick_IRQn priority for TIM4_IRQn
    NVIC_SetPriority(TIM4_IRQn, 3);
    TIM_ITConfig(TIM4, TIM_IT_CC2, ENABLE);
    NVIC_EnableIRQ(TIM4_IRQn);
    attachSystemInterrupt(SysInterrupt_TIM4_IRQ, [&] {
        // Do some work up to 200 microseconds
        delayMicroseconds(random(200));
        TIM_ClearITPendingBit(TIM4, TIM_IT_CC2);
    });
#endif

    system_tick_t last_millis_64 = System.millis();
    system_tick_t last_millis = millis();
    system_tick_t last_micros = micros();

    do
    {
        system_tick_t now_millis_64 = System.millis();
        system_tick_t now_millis = millis();
        system_tick_t now_micros = micros();

        assertMoreOrEqual(now_millis_64, last_millis_64);
        assertMoreOrEqual(now_millis, last_millis);
        assertMoreOrEqual(now_micros, last_micros);

        // micros always at least (millis()*1000)
        // even with overflow
        assertMoreOrEqual(now_micros, now_millis * 1000);
        // 1ms millis() advancement
        assertTrue(((int64_t)now_millis_64 - (int64_t)last_millis_64) <= 1);
        assertTrue(((int32_t)now_millis - (int32_t)last_millis) <= 1);
        // 1ms micros() advancement
        assertTrue(((int32_t)now_micros - (int32_t)last_micros) <= 1000);
        // at most 1.5ms difference between micros() and millis()
        assertTrue(std::abs((int32_t)now_micros - (int32_t)now_millis * 1000) <= 1500);
        // at most 1ms difference between millis() and lower 32 bits of System.millis()
        assertTrue(std::abs((int64_t)now_millis - (int64_t)(now_millis_64 & 0xffffffffull)) <= 1);

        duration -= now_millis - last_millis;

        last_millis_64 = now_millis_64;
        last_millis = now_millis;
        last_micros = now_micros;
    }
    while (duration > 0);

#if PLATFORM_ID==0 || (PLATFORM_ID>=6 && PLATFORM_ID<=10)
    NVIC_DisableIRQ(TIM4_IRQn);
    detachSystemInterrupt(SysInterrupt_TIM4_IRQ);
    TIM_ITConfig(TIM4, TIM_IT_CC2, DISABLE);
    digitalWrite(D0, HIGH);
#endif
}

test(TICKS_00_millis_micros_baseline_test)
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

// TODO: nRF52 platforms
#if (!defined(MODULAR_FIRMWARE) || !MODULAR_FIRMWARE) && !defined(HAL_PLATFORM_NRF52840)
// the __advance_system1MsTick isn't dynamically linked so we build this as a monolithic app
#include "hw_ticks.h"
test(TICKS_01_millis_and_micros_rollover)
{
    // this places the micros counter 3 seconds from rollover and the system ticks 5 seconds
    __advance_system1MsTick((uint64_t)-5000, 3000);
    #define TEN_SECONDS 10*1000
    const uint64_t start = System.millis();
    assert_micros_millis(TEN_SECONDS, true /* overflow */);
    assertMoreOrEqual(System.millis() - start, TEN_SECONDS);
}
#endif

test(TICKS_02_millis_and_micros_along_with_high_priority_interrupts)
{
    static const system_tick_t TWO_MINUTES = 2 * 60 * 1000;
    system_tick_t start = millis();
    assert_micros_millis_interrupts(TWO_MINUTES);
    assertMoreOrEqual(millis()-start,TWO_MINUTES);
}

test(TICKS_03_millis_and_micros_monotonically_increases)
{
    static const system_tick_t TWO_MINUTES = 2 * 60 * 1000;
    system_tick_t start = millis();
    assert_micros_millis(TWO_MINUTES);
    assertMoreOrEqual(millis()-start,TWO_MINUTES);
}
