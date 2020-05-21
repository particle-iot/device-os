
#include "application.h"
#include "unit-test/unit-test.h"
#include "scope_guard.h"

#if PLATFORM_THREADING
#include "system_threading.h"

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

test(THREAD_05_try_lock)
{
	TRY_LOCK(Serial) {

	}
}

// With THREADING DISABLED, checks that behavior of Particle.process() is as expected
// when run from main loop and custom threads
test(THREAD_06_particle_process_behavior_when_threading_disabled)
{
	/* This test should only be run with threading disabled */
	if (system_thread_get_state(nullptr) == spark::feature::ENABLED) {
		skip();
		return;
	}

	// Make sure Particle is connected
	Particle.connect();
	waitFor(Particle.connected,20000);
	assertTrue(Particle.connected());

	// Call disconnect from main thread
	Particle.disconnect();
	SCOPE_GUARD({
		// Make sure we restore cloud connection after exiting this test
		Particle.connect();
		waitFor(Particle.connected,20000);
		assertTrue(Particle.connected());
	});

	HAL_Delay_Milliseconds(2000); // wait sometime to confirm delay will not disconnect
	assertTrue(Particle.connected());

	// Run Particle.process from a custom thread
	Thread* th = new Thread("name", []{
			Particle.process();
		},OS_THREAD_PRIORITY_DEFAULT);
	assertTrue(bool(th));
	th->join();
	delete th;

	// Particle.process() should not do anything from custom thread, hence particle should be still connected
	assertTrue(Particle.connected());
	// Run Particle.process() from main thread
	Particle.process();
	HAL_Delay_Milliseconds(200);
	assertFalse(Particle.connected());
}

namespace {

volatile int test_val_fn1;
volatile int test_val_fn2;
static int test_val = 1;
void increment(void)
{
	test_val_fn1++;
}

void sys_particle_process_increment(void)
{
	Particle.process();
	test_val_fn2++;
}

void sys_thr_block(void)
{
	while(test_val) {
		HAL_Delay_Milliseconds(1);
	}
}

} // anonymous

// With THREADING ENABLED, checks that behavior of Particle.process() is as expected
// when run from system, app, and custom threads
test(THREAD_07_particle_process_behavior_when_threading_enabled)
{
	/* This test should only be run with threading disabled */
	if (system_thread_get_state(nullptr) == spark::feature::DISABLED) {
		skip();
		return;
	}

	// Make sure Particle is connected
	Particle.connect();
	waitFor(Particle.connected,20000);
	assertTrue(Particle.connected());

	// Schedule function on application thread that does not run for sometime because of hard delays
	test_val_fn1 = 0;
	ActiveObjectBase* app = (ActiveObjectBase*)system_internal(0, nullptr); // Returns application thread instance
	std::function<void(void)> fn = increment;
	app->invoke_async(fn);
	HAL_Delay_Milliseconds(10);
	assertEqual((int)test_val_fn1, 0);

	// Schedule particle.process on system thread
	test_val_fn2 = 0;
	ActiveObjectBase* system = (ActiveObjectBase*)system_internal(1, nullptr); // Returns system thread instance
	system->invoke_async(std::function<void()>(sys_particle_process_increment));
	HAL_Delay_Milliseconds(10);
	assertEqual((int)test_val_fn2, 1);

	// Schedule function on system thread that blocks for a variable (test_val)
	ActiveObjectBase* system2 = (ActiveObjectBase*)system_internal(1, nullptr); // Returns system thread instance
	system2->invoke_async(std::function<void()>(sys_thr_block));
	HAL_Delay_Milliseconds(10);

	// Call disconnect from main thread
	Particle.disconnect();
	SCOPE_GUARD({
		// Make sure we unblock the system thread and restore cloud connection after exiting this test
		test_val = 0;
		Particle.connect();
		waitFor(Particle.connected,20000);
		assertTrue(Particle.connected());
	});

	// Run Particle.process from a custom thread
	Thread* th = new Thread("name", []{
			Particle.process();
	},OS_THREAD_PRIORITY_DEFAULT);
	assertTrue(bool(th));
	th->join();
	delete th;
	// Particle.process() should not do anything from custom thread, hence particle should be still connected
	assertTrue(Particle.connected());

	assertEqual((int)test_val_fn1, 0);
	Particle.process();
	assertEqual((int)test_val_fn1, 1);
	// Unblock system thread
	test_val = 0;
	HAL_Delay_Milliseconds(5000);
	assertFalse(Particle.connected());
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
		// Worst case scenario it may take two application watchdog loop iterations
		HAL_Delay_Milliseconds(t * 2);
		// LOG_DEBUG(INFO, "TIME: %d, R %d:%s", millis()-startTime, x, timeout_called?"pass":"fail");
		assertEqual(timeout_called, 1);
		waitForComplete(wd);
		uint32_t endTime = millis();
		const auto expected = t * 4; // should be t*4
		const auto margin = expected / 100; // 1%
		assertMoreOrEqual(endTime - startTime, expected - margin);
		assertLessOrEqual(endTime - startTime, expected + margin);
		// LOG_DEBUG(INFO, "E %d",endTime-startTime);
	}
}

test(APPLICATION_WATCHDOG_03_doesnt_leak_memory)
{
	// Give the system some time to get the resources back
	delay(500);

	// Before Photon/P1 Thread fixes were introduced, we would lose approximately 20k
	// of RAM due to 10 allocations of ApplicationWatchdog in APPLICATION_WATCHDOG_02.
	// Taking fragmentation and other potential allocations into consideration, 2k seems like
	// a good testing value.
	assertMoreOrEqual(System.freeMemory(), s_ram_free_before - 2048);
}

#endif
