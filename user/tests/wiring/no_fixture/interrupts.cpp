
#include "application.h"
#include "unit-test/unit-test.h"

#if defined(STM32F2XX)
volatile uint32_t time_ms = 10000; // start at some arbitrary value

void inc_time() {
	time_ms++;
}

uint32_t get_time() {
	return time_ms;
}

test(INTERRUPTS_01_atomic_section)
{
	// software timer and time_ms will not increment while interrupts are disabled.
	// micros() does increment

	uint32_t start_micros = micros();
	uint32_t start_millis, end_millis;
	Timer timer(1, inc_time); // increment time_ms every 1ms with a software timer
	timer.start();
	{
		ATOMIC_BLOCK() {
			start_millis = get_time();
			while (micros()-start_micros<10000)
			{
				// validates that atomic sections can be nested and interrupts restored when the outermost one exits
				ATOMIC_BLOCK();
			}
			end_millis = get_time();
		}
	}
	// Serial.printlnf("%d == %d", start_millis, end_millis);
	assertEqual(start_millis, end_millis);

	// now do the same without an atomic block
	{
		start_micros = micros();
		start_millis = get_time();
		while (micros()-start_micros<10000)
		{
			// busy wait
		}
		end_millis = get_time();
	}
	// Serial.printlnf("%d > %d", end_millis, start_millis);
	assertMore(end_millis, start_millis);
	timer.stop();

}
#endif // defined(STM32F2XX)

namespace
{

class TestHandler
{
public:
	TestHandler()
	{
		++count;
	}

	TestHandler(const TestHandler&)
	{
		++count;
	}

	~TestHandler()
	{
		--count;
	}

	void operator()()
	{
	}

	static int count;
};

} // namespace

int TestHandler::count = 0;

test(INTERRUPTS_02_detached_handler_is_destroyed)
{
	assertEqual(TestHandler::count, 0);
	attachSystemInterrupt(SysInterrupt_SysTick, TestHandler());
	assertEqual(TestHandler::count, 1);
	attachSystemInterrupt(SysInterrupt_SysTick, TestHandler()); // Override current handler
	assertEqual(TestHandler::count, 1); // Previous handler has been destroyed
	detachSystemInterrupt(SysInterrupt_SysTick);
	assertEqual(TestHandler::count, 0);
}

test(INTERRUPTS_03_isisr_willpreempt_servicedirqn)
{
#if defined(STM32F10X_MD) || defined(STM32F10X_HD) || defined(STM32F2XX)
	volatile bool cont = false;
	attachSystemInterrupt(SysInterrupt_SysTick, [&] {
		assertTrue(HAL_IsISR());
		assertEqual((IRQn)HAL_ServicedIRQn(), SysTick_IRQn);
		cont = true;
	});
	while (!cont);
	detachSystemInterrupt(SysInterrupt_SysTick);
	assertFalse(HAL_WillPreempt(SysTick_IRQn, SysTick_IRQn));
	assertTrue(HAL_WillPreempt(NonMaskableInt_IRQn, SysTick_IRQn));
	assertFalse(HAL_WillPreempt(SysTick_IRQn, NonMaskableInt_IRQn));
#endif
}
