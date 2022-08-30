
#include "application.h"
#include "unit-test/unit-test.h"
#include "scope_guard.h"

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

int count = 0;

static void attach_interrupt_handler() {
    count++;
}

} // namespace

test(INTERRUPTS_01_isisr_willpreempt_servicedirqn)
{
	static volatile bool cont = false;
	attachInterruptDirect(SysTick_IRQn, []() {
		detachInterruptDirect(SysTick_IRQn);
		assertTrue(hal_interrupt_is_isr());
		assertEqual((int)hal_interrupt_serviced_irqn(), (int)SysTick_IRQn);
		cont = true;
	});
	while (!cont);
	assertFalse(hal_interrupt_will_preempt(SysTick_IRQn, SysTick_IRQn));
	assertTrue(hal_interrupt_will_preempt(NonMaskableInt_IRQn, SysTick_IRQn));
	assertFalse(hal_interrupt_will_preempt(SysTick_IRQn, NonMaskableInt_IRQn));

#if HAL_PLATFORM_RTL872X
	// Using arbitrary interrupt
	assertTrue(attachInterruptDirect(I2C0_IRQ, []() {return;}));
	assertFalse(attachInterruptDirect(I2C0_IRQ, []() {return;}));
	assertTrue(detachInterruptDirect(I2C0_IRQ));
#endif
}

test(INTERRUPTS_02_attachintterupt_does_not_affect_pullup_pulldown_nopull)
{
#if PLATFORM_ID == PLATFORM_ESOMX
	const pin_t START_PIN = D0;
    const pin_t END_PIN = D2;
#elif PLATFORM_ID == PLATFORM_TRACKERM
    const pin_t START_PIN = D8;
    const pin_t END_PIN = D9;
#else
    // Every other platform has D2 ~ D3 available.
    const pin_t START_PIN = D2;
    const pin_t END_PIN = D3;
#endif // PLATFORM_ID == PLATFORM_ESOMX
    SCOPE_GUARD ({
            for (int i = START_PIN; i <= END_PIN; i++) {
                detachInterrupt(i);
            }
        });
    for (int i = START_PIN; i <= END_PIN; i++) {
        pinMode(i, INPUT_PULLUP);
        delay(100);
        assertTrue(digitalRead(i) == HIGH); // INPUT_PULLUP should be set
    }
    for (int i = START_PIN; i <= END_PIN; i++) {
        attachInterrupt(i, attach_interrupt_handler, RISING); // used to set PULLDOWN before sc-107554
        delay(100);
        assertTrue(digitalRead(i) == HIGH); // INPUT_PULLUP should still be set
    }
    for (int i = START_PIN; i <= END_PIN; i++) {
        detachInterrupt(i);
    }

    for (int i = START_PIN; i <= END_PIN; i++) {
        pinMode(i, INPUT_PULLDOWN);
        delay(100);
        assertTrue(digitalRead(i) == LOW); // INPUT_PULLUP should be set
    }
    for (int i = START_PIN; i <= END_PIN; i++) {
        attachInterrupt(i, attach_interrupt_handler, FALLING); // used to set PULLUP before sc-107554
        delay(100);
        assertTrue(digitalRead(i) == LOW); // INPUT_PULLDOWN should still be set
    }
    for (int i = START_PIN; i <= END_PIN; i++) {
        detachInterrupt(i);
        delay(100);
        assertTrue(digitalRead(i) == LOW); // INPUT_PULLDOWN should be set
    }
    for (int i = START_PIN; i <= END_PIN; i++) {
        attachInterrupt(i, attach_interrupt_handler, CHANGE); // used to set PULLUP before sc-107554
        delay(100);
        assertTrue(digitalRead(i) == LOW); // INPUT_PULLDOWN should still be set
    }
    // SCOPE_GUARD will detachInterrupt's
}
