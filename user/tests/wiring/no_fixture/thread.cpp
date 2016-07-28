
#include "application.h"
#include "unit-test/unit-test.h"

#if PLATFORM_THREADING
test(THREAD_01_creation)
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

test(THREAD_02_SingleThreadedBlock)
{
	SINGLE_THREADED_BLOCK() {

	}
	SINGLE_THREADED_BLOCK() {

	}
}

test(THREAD_03_with_lock)
{
	WITH_LOCK(Serial) {

	}

	WITH_LOCK(Serial) {

	}

}

test(THREAD_04_try_lock)
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


test(APPLICATION_WATCHDOG_01_fires_timeout)
{
	timeout_called = 0;
	ApplicationWatchdog wd(5, timeout);
	HAL_Delay_Milliseconds(10);

	assertEqual(timeout_called, 1);
	waitForComplete(wd);
}

test(APPLICATION_WATCHDOG_02_doesnt_fire_when_app_checks_in)
{
	for (int x=0; x<10; x++) {
		timeout_called = 0;
		unsigned t = 100;
		uint32_t startTime;
		// LOG_DEBUG(INFO, "S %d", millis());
		ApplicationWatchdog wd(t, timeout, 2048);
		startTime = millis();
		// this for() loop should consume more than t(seconds), about 2x as much
		for (int i=0; i<10; i++) {
			HAL_Delay_Milliseconds(t/5);
			assertEqual(timeout_called, 0);
			application_checkin();
			os_thread_yield();
		}
		// now force a timeout
		HAL_Delay_Milliseconds(t+10);
		assertEqual(timeout_called, 1);
		// LOG_DEBUG(INFO, "TIME: %d, R %d:%s", millis()-startTime, x, timeout_called?"pass":"fail");
		waitForComplete(wd);
		uint32_t endTime = millis();
		assertMoreOrEqual(endTime-startTime, 307); // should be 310 (give it 1% margin)
		assertLessOrEqual(endTime-startTime, 313); //   |
		// LOG_DEBUG(INFO, "E %d",endTime-startTime);
	}
}

#endif
