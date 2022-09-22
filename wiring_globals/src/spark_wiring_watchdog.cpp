
#include "spark_wiring_watchdog.h"

#if PLATFORM_THREADING

volatile system_tick_t ApplicationWatchdog::last_checkin = 0;

os_thread_return_t ApplicationWatchdog::start(void* pointer)
{
	ApplicationWatchdog& wd = *(ApplicationWatchdog*)pointer;
	wd.loop();
	os_thread_cleanup(nullptr);
}

void ApplicationWatchdog::loop()
{
	auto wakeupTimestamp = last_checkin + timeout;
	auto now = current_time();
	if (wakeupTimestamp > now) {
		HAL_Delay_Milliseconds(wakeupTimestamp - now + 1);
	}

	while (true) {
		now = current_time();
		if ((now-last_checkin)>=timeout) {
			break;
		}
		HAL_Delay_Milliseconds(timeout);
	}

	if (timeout>0 && timeout_fn) {
		timeout_fn();
	}
}


#endif
