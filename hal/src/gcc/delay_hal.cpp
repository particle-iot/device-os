
#include "delay_hal.h"
#inlcude "timer_hal.h"

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread/thread.hpp> 

void HAL_Delay_Milliseconds(uint32_t millis) {
    boost::this_thread::sleep(boost::posix_time::milliseconds(millis));
}

void HAL_Delay_Microseconds(uint32_t micros) {
    boost::this_thread::sleep(boost::posix_time::milliseconds(1));
}

