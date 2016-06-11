
#include "application.h"
#include "unit-test/unit-test.h"

#ifndef ABS
#define ABS(x) (x >= 0 ? x : -x)
#endif

void assert_micros_millis(system_tick_t duration)
{
#if PLATFORM_ID==0 || (PLATFORM_ID>=6 && PLATFORM_ID<=10)
    // Just in case
    NVIC_DisableIRQ(TIM4_IRQn);
#endif
    system_tick_t last = millis();
    system_tick_t end = last + duration;

    system_tick_t last_micros = micros();

    do
    {
        system_tick_t now = millis();
        system_tick_t now_micros = micros();

        assertMoreOrEqual(now, last);
        assertMoreOrEqual(now_micros, last_micros);
        // micros always at least (millis()*1000)
        // even with overflow
        assertMoreOrEqual(now_micros, now*1000);

        last = now;
        last_micros = now_micros;
    }
    while (last<end);
}

void assert_micros_millis_interrupts(system_tick_t duration)
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
    system_tick_t last_micros = micros();
    system_tick_t end = last + duration;
    do
    {
        system_tick_t now = millis();
        system_tick_t now_micros = micros();

        assertMoreOrEqual(now, last);
        assertMoreOrEqual(now_micros, last_micros);
        // micros always at least (millis()*1000)
        // even with overflow
        assertMoreOrEqual(now_micros, now*1000);
        // 1ms millis() advancement
        assertTrue(((int32_t)now - (int32_t)last) <= 1);
        // 1ms micros() advancement
        assertTrue(((int32_t)now_micros - (int32_t)last_micros) <= 1000);
        // at most 1.5ms difference between micros() and millis()
        assertTrue(ABS((int32_t)now_micros - (int32_t)now*1000) <= 1500);

        last = now;
        last_micros = now_micros;
    }
    while (last<end);

#if PLATFORM_ID==0 || (PLATFORM_ID>=6 && PLATFORM_ID<=10)
    NVIC_DisableIRQ(TIM4_IRQn);
    detachSystemInterrupt(SysInterrupt_TIM4_IRQ);
    TIM_ITConfig(TIM4, TIM_IT_CC2, DISABLE);
    digitalWrite(D0, HIGH);
#endif
}


#if !MODULAR_FIRMWARE
// the __advance_system1MsTick isn't dynamically linked so we build this as a monolithic app
#include "hw_ticks.h"
test(ticks_millis_and_micros_rollover)
{
    // this places the micros counter 2 seconds from rollover and the systemm ticks 3 seconds
    __advance_system1MsTick(system_tick_t(-5000), 3000);
    assert_micros_millis(10*1000); // 10 seconds
}
#endif

test(ticks_millis_and_micros_along_with_high_priority_interrupts)
{
    assert_micros_millis_interrupts(2*60*1000); // 2 minutes
}

test(ticks_millis_and_micros_monotonically_increases)
{
    assert_micros_millis(2*60*1000);    // 2 minutes
}
