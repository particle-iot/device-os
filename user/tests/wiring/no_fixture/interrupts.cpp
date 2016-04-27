
#include "application.h"
#include "unit-test/unit-test.h"

test(interrupts_atomic_section)
{
	// using SysTick for convenience - we could use a stm32 timer interrupt for a more generic test
	// millis() will not increment while interrupts are disabled.
	// micros() does increment

	uint32_t start_micros = micros();
	uint32_t start_millis, end_millis;
	{
		ATOMIC_BLOCK() {
			start_millis = millis();
			while (micros()-start_micros<2000)
			{
				// validates that atomic sections can be nested and interrupts restored when the outermost one exits
				ATOMIC_BLOCK();
			}
			end_millis = millis();
		}
	}
	assertEqual(start_millis, end_millis);

	// now do the same without an atomic block
	{
		start_micros = micros();
		start_millis = millis();
		while (micros()-start_micros<2000)
		{
			// busy wait
		}
		end_millis = millis();
	}

	assertMore(end_millis, start_millis);
}

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

test(interrupts_detached_handler_is_destroyed)
{
	assertEqual(TestHandler::count, 0);
	attachSystemInterrupt(SysInterrupt_SysTick, TestHandler());
	assertEqual(TestHandler::count, 1);
	attachSystemInterrupt(SysInterrupt_SysTick, TestHandler()); // Override current handler
	assertEqual(TestHandler::count, 1); // Previous handler has been destroyed
	detachSystemInterrupt(SysInterrupt_SysTick);
	assertEqual(TestHandler::count, 0);
}

test(interrupts_isisr_willpreempt_servicedirqn)
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
