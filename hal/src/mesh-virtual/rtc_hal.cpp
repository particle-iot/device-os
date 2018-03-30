
#include "rtc_hal.h"


#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/version.hpp>


void HAL_RTC_Configuration(void)
{
}

#if BOOST_VERSION < 105800
time_t to_time_t(boost::posix_time::ptime t)
{
    using namespace boost::posix_time;
    static ptime epoch(boost::gregorian::date(1970,1,1));
    time_duration::sec_type x = (t - epoch).total_seconds();
    return time_t(x);
}
#endif

time_t HAL_RTC_Get_UnixTime(void)
{
    auto now = boost::posix_time::microsec_clock::universal_time();
    return to_time_t(now);
}

void HAL_RTC_Set_UnixTime(time_t value)
{

}

void HAL_RTC_Set_UnixAlarm(time_t value)
{
}

void HAL_RTC_Cancel_UnixAlarm(void)
{
}

uint8_t HAL_RTC_Time_Is_Valid(void* reserved)
{
    return 1;
}