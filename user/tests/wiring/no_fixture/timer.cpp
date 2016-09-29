#include "application.h"
#include "unit-test/unit-test.h"

#if PLATFORM_THREADING

uint32_t timer_callback_called;
void timer_callback()
{
	timer_callback_called = millis();
}

void dispose(Timer** timers, int count)
{
	while (count-->0)
	{
		delete *timers++;
	}
}

void stop(Timer** timers, int count)
{
	while (count-->0)
	{
		(*timers++)->stop();
	}
}


test(TIMER_01_create_10_timers)
{
	const int count = 10;
	Timer* timers[count];

	for (int i=0; i<count; i++)
	{
		Timer* t = new Timer(1000+(i*1000), timer_callback);
		timers[i] = t;
		if (!t->isValid()) {
			Serial.printlnf("timer %d is not valid", i);
		}
		assertTrue(t->isValid());
	}

	dispose(timers, count);
}

class MyTimer : public Timer
{
public:
	MyTimer(unsigned period) : Timer(period, timer_callback_fn(nullptr)), time(0) {}

	uint32_t time;

	void timeout() override
	{
		time = millis();
	}
};

test(TIMER_02_subclass)
{
	MyTimer timer(10);
	assertTrue(timer.isValid());
	uint32_t start = millis();
	timer.start();
	delay(50);
	timer.dispose();
	assertFalse(timer.isValid());
	assertMore(timer.time, start);
}

class CallbackClass
{
public:
	uint32_t time;

	CallbackClass() : time(0) {}

	void onTimeout() {
		time = millis();
	}
};


test(TIMER_03_class_method)
{
	CallbackClass c;
	Timer timer(10, &CallbackClass::onTimeout, c);
	assertTrue(timer.isValid());
	uint32_t start = millis();
	timer.start();
	delay(50);
	timer.dispose();
	assertMore(c.time, start);
}

test(TIMER_04_not_started)
{
	CallbackClass c;
	Timer timer(10, &CallbackClass::onTimeout, c);
	assertTrue(timer.isValid());
	timer.dispose();
	assertEqual(c.time, 0);
}

void create_timers_with_delay(unsigned block_for, int& fails)
{
	// the exact amount of timers needed is based on the size of the
	// timer queue.
	fails = 0;
	Timer* timers[10];
	for (int i=0; i<5; i++)
	{
		Timer* t = new Timer(1, [] { HAL_Delay_Milliseconds(50); }, true);
		bool started = t->start();
		assertTrue(started);
		delay(3);
		Timer* t2 = new Timer(1, timer_callback, true);
		if (!t2->start(block_for))
				fails++;
		timers[i*2] = t;
		timers[i*2+1] = t2;
	}
	stop(timers, 10);
	dispose(timers, 10);
}

int create_timers_with_delay(unsigned block_for)
{
	// assertXXX macros expect the function to have a void return type.
	int fails = 0;
	create_timers_with_delay(block_for, fails);
	return fails;
}


test(TIMER_05_create_with_no_delay_fails_when_timer_service_is_busy)
{
	int fails = create_timers_with_delay(0);
	assertMore(fails, 0);
}

test(TIMER_06_create_with_long_delay_succeeds_when_timer_service_is_not_busy)
{
	int fails = create_timers_with_delay(1000);
	assertEqual(fails, 0);
}

/**
 * When the timer service is busy, attempts to create a timer with too short a delay
 * will fail.
 */
test(TIMER_07_can_be_disposed_disposed_when_running)
{
	Timer t(1, [] { HAL_Delay_Milliseconds(50); }, true);
	assertTrue(t.start());
}

test(TIMER_08_disposed_early)
{
	Timer t(1, [] { HAL_Delay_Milliseconds(50); }, true);
	assertTrue(t.start());
	delay(2);
	Timer t2(1, timer_callback, true);
	t2.start(0);
}

test(TIMER_09_is_active)
{
	Timer t(10, [] {}, true);
	// Serial.println("not started");
	assertFalse(t.isActive());
	t.start();
	// Serial.println("valid");
	assertTrue(t.isValid());
	// Serial.println("started");
	delay(1);
	assertTrue(t.isActive());
	delay(20);
	// Serial.println("20ms later should be stopped");
	assertFalse(t.isActive());
	t.start();
	// Serial.println("re-started");
	delay(1);
	assertTrue(t.isActive());
	t.stop();
	delay(1);
	// Serial.println("stopped");
	assertFalse(t.isActive());
}

#endif
