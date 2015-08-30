
#include "application.h"
#include "unit-test/unit-test.h"


test(delay_1_is_within_5_percent)
{
    for (int i=0; i<100; i++) {

        uint32_t start = micros();
        system_delay_ms(1, true);
        uint32_t end = micros();

        assertMoreOrEqual(end-start, 1000);
        assertLessOrEqual(end-start, 1050);
    }
}