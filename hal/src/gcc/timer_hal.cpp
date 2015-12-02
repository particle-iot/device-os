
#include "timer_hal.h"

#include <boost/date_time/posix_time/posix_time.hpp>

auto start = boost::posix_time::microsec_clock::universal_time();

system_tick_t HAL_Timer_Get_Micro_Seconds(void)
{
    auto now = boost::posix_time::microsec_clock::universal_time();
    auto diff = now - start;
    return diff.total_microseconds();
}

system_tick_t HAL_Timer_Get_Milli_Seconds(void)
{
    auto now = boost::posix_time::microsec_clock::universal_time();
    auto diff = now - start;
    return diff.total_milliseconds();
}



