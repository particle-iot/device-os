
#include "application.h"
#include "unit-test/unit-test.h"

test(TIME_NowReturnsCorrectUnixTime) {
    // when
    time_t last_time = Time.now();
    delay(1000); //systick delay for 1 second
    // then
    time_t current_time = Time.now();
    assertEqual(current_time, last_time + 1);//RTC interrupt fires successfully
}

test(TIME_SetTimeResultsInCorrectUnixTimeUpdate) {
    // when
    time_t current_time = Time.now();
    Time.setTime(86400);//set to epoch time + 1 day
    // then
    time_t temp_time = Time.now();
    assertEqual(temp_time, 86400);
    // restore original time
    Time.setTime(current_time);
}
