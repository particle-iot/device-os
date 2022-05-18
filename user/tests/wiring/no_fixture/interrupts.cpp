
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
		assertTrue(HAL_IsISR());
		assertEqual((IRQn_Type)HAL_ServicedIRQn(), SysTick_IRQn);
		cont = true;
	});
	while (!cont);
	assertFalse(HAL_WillPreempt(SysTick_IRQn, SysTick_IRQn));
	assertTrue(HAL_WillPreempt(NonMaskableInt_IRQn, SysTick_IRQn));
	assertFalse(HAL_WillPreempt(SysTick_IRQn, NonMaskableInt_IRQn));
}