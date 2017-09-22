
#include "application.h"
#include "unit-test/unit-test.h"

#ifndef ABS
#define ABS(x) (x >= 0 ? x : -x)
#endif

namespace {

inline system_tick_t seconds() {
    return HAL_Timer_Get_Seconds();
}

} // namespace

void assert_ticks(system_tick_t duration)
{
#if PLATFORM_ID==0 || (PLATFORM_ID>=6 && PLATFORM_ID<=10)
    // Just in case
    NVIC_DisableIRQ(TIM4_IRQn);
#endif
    system_tick_t last = millis();
    system_tick_t end = last + duration;
    system_tick_t elapsed = 0;

    system_tick_t last_micros = micros();
    system_tick_t last_seconds = seconds();

    do
    {
        system_tick_t now = millis();
        system_tick_t now_micros = micros();
        system_tick_t now_seconds = seconds();

        assertMoreOrEqual(now, last);
        assertMoreOrEqual(now_micros, last_micros);
        assertMoreOrEqual(now_seconds, last_seconds);

        // micros always at least (millis()*1000)
        // even with overflow
        assertMoreOrEqual(now_micros, now*1000);

        // seconds should not wrap along with millis
        elapsed += now - last;
        assertMoreOrEqual(now_seconds, elapsed / 1000);

        last = now;
        last_micros = now_micros;
        last_seconds = now_seconds;
    }
    while (last<end);
}

void assert_ticks_interrupts(system_tick_t duration)
{
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

    system_tick_t last = millis();
    system_tick_t end = last + duration;
    system_tick_t elapsed = 0;

    system_tick_t last_micros = micros();
    system_tick_t last_seconds = seconds();

    do
    {
        system_tick_t now = millis();
        system_tick_t now_micros = micros();
        system_tick_t now_seconds = seconds();

        assertMoreOrEqual(now, last);
        assertMoreOrEqual(now_micros, last_micros);
        assertMoreOrEqual(now_seconds, last_seconds);
        // micros always at least (millis()*1000)
        // even with overflow
        assertMoreOrEqual(now_micros, now*1000);
        // 1ms millis() advancement
        assertTrue(((int32_t)now - (int32_t)last) <= 1);
        // 1ms micros() advancement
        assertTrue(((int32_t)now_micros - (int32_t)last_micros) <= 1000);
        // 1s seconds() advancement
        assertTrue(((int32_t)now_seconds - (int32_t)last_seconds) <= 1);
        // at most 1.5ms difference between micros() and millis()
        assertTrue(ABS((int32_t)now_micros - (int32_t)now*1000) <= 1500);

        elapsed += now - last;
        assertMoreOrEqual(now_seconds, elapsed / 1000);

        last = now;
        last_micros = now_micros;
        last_seconds = now_seconds;
    }
    while (last<end);

#if PLATFORM_ID==0 || (PLATFORM_ID>=6 && PLATFORM_ID<=10)
    NVIC_DisableIRQ(TIM4_IRQn);
    detachSystemInterrupt(SysInterrupt_TIM4_IRQ);
    TIM_ITConfig(TIM4, TIM_IT_CC2, DISABLE);
    digitalWrite(D0, HIGH);
#endif
}

test(TICKS_00_millis_micros_baseline_test)
{
    const system_tick_t DELAY = 3 * 1000;
    system_tick_t startMillis = millis();
    system_tick_t startMicros = micros();
    system_tick_t startSeconds = seconds();
    delay(DELAY);
    assertMoreOrEqual(millis() - startMillis, DELAY);
    assertMoreOrEqual(micros() - startMicros, DELAY * 1000);
    assertMoreOrEqual(seconds() - startSeconds, DELAY / 1000);
    startMillis = millis();
    startMicros = micros();
    delayMicroseconds(DELAY);
    assertMoreOrEqual(millis() - startMillis, DELAY / 1000);
    assertMoreOrEqual(micros() - startMicros, DELAY);
}

#if !MODULAR_FIRMWARE
// the __advance_system1MsTick isn't dynamically linked so we build this as a monolithic app
#include "hw_ticks.h"
test(TICKS_01_millis_and_micros_rollover)
{
    // this places the micros counter 2 seconds from rollover and the systemm ticks 3 seconds
    __advance_system1MsTick(system_tick_t(-5000), 3000);
    #define TEN_SECONDS 10*1000
    system_tick_t start = millis();
    assert_ticks(TEN_SECONDS);
    assertMoreOrEqual(millis()-start,TEN_SECONDS);
}
#endif

test(TICKS_02_millis_and_micros_along_with_high_priority_interrupts)
{
    #define TWO_MINUTES 2*60*1000
    system_tick_t start = millis();
    assert_ticks_interrupts(TWO_MINUTES);
    assertMoreOrEqual(millis()-start,TWO_MINUTES);
}

test(TICKS_03_millis_and_micros_monotonically_increases)
{
    #define TWO_MINUTES 2*60*1000
    system_tick_t start = millis();
    assert_ticks(TWO_MINUTES);
    assertMoreOrEqual(millis()-start,TWO_MINUTES);
}
