
#include "application.h"
#include "unit-test/unit-test.h"

void assert_micros_millis(system_tick_t duration)
{
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


test(ticks_millis_and_micros_monotonically_increases)
{
    assert_micros_millis(2*60*1000);    // 2 minutes
}
