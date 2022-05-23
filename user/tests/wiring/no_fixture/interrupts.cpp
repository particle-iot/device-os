
#include "application.h"
#include "unit-test/unit-test.h"

// TODO for HAL_PLATFORM_NRF52840
// TestHandler is currently unused but leaving in for refernece
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

test(INTERRUPTS_01_isisr_willpreempt_servicedirqn)
{
	static volatile bool cont = false;
	attachInterruptDirect(SysTick_IRQn, []() {
		detachInterruptDirect(SysTick_IRQn);
		assertTrue(hal_interrupt_is_isr());
		assertEqual((IRQn_Type)hal_interrupt_serviced_irqn(), SysTick_IRQn);
		cont = true;
	});
	while (!cont);
	assertFalse(hal_interrupt_will_preempt(SysTick_IRQn, SysTick_IRQn));
	assertTrue(hal_interrupt_will_preempt(NonMaskableInt_IRQn, SysTick_IRQn));
	assertFalse(hal_interrupt_will_preempt(SysTick_IRQn, NonMaskableInt_IRQn));
}
