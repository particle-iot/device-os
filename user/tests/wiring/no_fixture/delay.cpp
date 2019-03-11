
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

test(DELAY_01_delay_1ms_is_within_tolerance)
{
    // These errors are mainly due to processing overhead, which is a greater factor on NRF with a slower clock speed
#if HAL_PLATFORM_NRF52840
    const uint32_t delay_us_error = 200; // 20%
#else
    const uint32_t delay_us_error = 50; // 5%
#endif

    DelayTest dt(10);
    // on RTOS have to stop task scheduling for the delays or we may not
    // be executed often enough
    for (int i=0; i<100; i++) {
        scheduler(false);
        uint32_t start = micros();
        system_delay_ms(1, true);
        uint32_t end = micros();
        scheduler(true);

        // Serial.printlnf("total time:%lu", end-start);
        assertMoreOrEqual(end-start, 1000);
        assertLessOrEqual(end-start, 1000 + delay_us_error);
    }
}