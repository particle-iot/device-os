
#include "application.h"
#include "unit-test/unit-test.h"


#if PLATFORM_THREADING
#define scheduler(b) os_thread_scheduling(b, NULL)
#else
#define scheduler(b)
#endif

class DelayTest
{
    public:

    DelayTest(uint16_t duration)
    {
        HAL_Delay_Milliseconds(duration);
    }
};

DelayTest dt(10);

test(delay_1_is_within_5_percent)
{
    // on RTOS have to stop task scheduling for the delays or we may not
    // be executed often enough
    for (int i=0; i<100; i++) {
        scheduler(false);
        uint32_t start = micros();
        system_delay_ms(1, true);
        uint32_t end = micros();
        scheduler(true);

        assertMoreOrEqual(end-start, 1000);
        assertLessOrEqual(end-start, 1050);
    }
}