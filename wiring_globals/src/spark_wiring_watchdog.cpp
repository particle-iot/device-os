
#include "spark_wiring_watchdog.h"

#if PLATFORM_THREADING

volatile system_tick_t ApplicationWatchdog::last_checkin;

os_thread_return_t ApplicationWatchdog::start(void* pointer)
{
	ApplicationWatchdog& wd = *(ApplicationWatchdog*)pointer;
	wd.loop();
	os_thread_cleanup(nullptr);
}

void ApplicationWatchdog::loop()
{
	bool done = false;
	system_tick_t now;
	while (!done) {
		HAL_Delay_Milliseconds(timeout);
		now = current_time();
		done = (now-last_checkin)>=timeout;
	}

	if (timeout>0 && timeout_fn) {
		timeout_fn();
		timeout_fn = std::function<void(void)>();
	}
}


#endif
