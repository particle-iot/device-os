#include "timer_hal.h"

#include <boost/date_time/posix_time/posix_time.hpp>

namespace {

const auto startMicros = boost::posix_time::microsec_clock::universal_time();
const auto startSeconds = boost::posix_time::second_clock::universal_time();

} // namespace

system_tick_t HAL_Timer_Get_Micro_Seconds(void)
{
    auto now = boost::posix_time::microsec_clock::universal_time();
    auto diff = now - startMicros;
    return diff.total_microseconds();
}

system_tick_t HAL_Timer_Get_Milli_Seconds(void)
{
    auto now = boost::posix_time::microsec_clock::universal_time();
    auto diff = now - startMicros;
    return diff.total_milliseconds();
}

system_tick_t HAL_Timer_Get_Seconds(void)
{
    const auto now = boost::posix_time::second_clock::universal_time();
    return (now - startSeconds).total_seconds();
}
