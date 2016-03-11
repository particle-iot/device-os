
#include "application.h"
#include "unit-test/unit-test.h"

#if PLATFORM_THREADING
test(Thread_creation)
{
    volatile bool threadRan = false;
    Thread testThread = Thread("test", [&]() {
        threadRan = true;
        for(;;) {}
    });

    for(int tries = 5; !threadRan && tries >= 0; tries--) {
        delay(1);
    }

    testThread.dispose();

    assertTrue((bool)threadRan);
}

test(thread_SingleThreadedBlock)
{
	SINGLE_THREADED_BLOCK() {

	}
	SINGLE_THREADED_BLOCK() {

	}
}

test(thread_with_lock)
{
	WITH_LOCK(Serial) {

	}

	WITH_LOCK(Serial) {

	}

}

test(thread_try_lock)
{
	TRY_LOCK(Serial) {

	}
}

// todo - test for SingleThreadedSection



volatile bool timeout_called = 0;
void timeout()
{
	timeout_called++;
}

void waitForComplete(ApplicationWatchdog& wd)
{
	while (!wd.isComplete()) {
		HAL_Delay_Milliseconds(10);
	}
}


test(application_watchdog_fires_timeout)
{
	timeout_called = 0;
	ApplicationWatchdog wd(5, timeout);
	HAL_Delay_Milliseconds(10);

	assertEqual(timeout_called, 1);
	waitForComplete(wd);
}

test(application_watchdog_doesnt_fire_when_app_checks_in)
{
	timeout_called = 0;
	unsigned t = 100;
	ApplicationWatchdog wd(t, timeout);

	for (int i=0; i<10; i++) {
		assertEqual(timeout_called, 0);
		application_checkin();
		os_thread_yield();
	}
	HAL_Delay_Milliseconds(t);
	assertEqual(timeout_called, 1);
	waitForComplete(wd);
}

#endif
