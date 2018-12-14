
#include "application.h"
#include "unit-test/unit-test.h"

#if PLATFORM_THREADING

static uint32_t s_ram_free_before = 0;

Thread testThread;
test(THREAD_01_creation)
{
    s_ram_free_before = System.freeMemory();
    volatile bool threadRan = false;
    testThread = Thread("test", [&]() {
        threadRan = true;
    }, OS_THREAD_PRIORITY_DEFAULT, 4096);

    for(int tries = 5; !threadRan && tries >= 0; tries--) {
        delay(1);
    }

    testThread.dispose();

    assertTrue((bool)threadRan);
}

test(THREAD_02_thread_doesnt_leak_memory)
{
	// 1024 less to account for fragmentation and other allocations
	delay(1000);
	assertMoreOrEqual(System.freeMemory(), s_ram_free_before - 1024);
}

test(THREAD_03_SingleThreadedBlock)
{
	SINGLE_THREADED_BLOCK() {

	}
	SINGLE_THREADED_BLOCK() {

	}
}

test(THREAD_04_with_lock)
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
	s_ram_free_before = System.freeMemory();
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
		// LOG_DEBUG(INFO, "TIME: %d, R %d:%s", millis()-startTime, x, timeout_called?"pass":"fail");
		assertEqual(timeout_called, 1);
		waitForComplete(wd);
		uint32_t endTime = millis();
		assertMoreOrEqual(endTime-startTime, 307); // should be 310 (give it 1% margin)
		assertLessOrEqual(endTime-startTime, 313); //   |
		// LOG_DEBUG(INFO, "E %d",endTime-startTime);
	}
}

test(APPLICATION_WATCHDOG_03_doesnt_leak_memory)
{
	// Before Photon/P1 Thread fixes were introduced, we would lose approximately 20k
	// of RAM due to 10 allocations of ApplicationWatchdog in APPLICATION_WATCHDOG_02.
	// Taking fragmentation and other potential allocations into consideration, 2k seems like
	// a good testing value.
	assertMoreOrEqual(System.freeMemory(), s_ram_free_before - 2048);
}

#endif
