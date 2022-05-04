
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

class DetachingHandler
{
	public:
		DetachingHandler(pin_t p): pin(p) {}

		void handler()
		{
			cnt++;
			detachInterrupt(pin);
		}

		int count() {
			return cnt;
		}

	private:
		pin_t pin;
		volatile int cnt = 0;
};

} // namespace

#if !HAL_PLATFORM_NRF52840 // TODO

int TestHandler::count = 0;

test(INTERRUPTS_02_detached_handler_is_destroyed)
{
	assertEqual(TestHandler::count, 0);
	attachSystemInterrupt(SysInterrupt_SysTick, TestHandler());
	assertEqual(TestHandler::count, 1);
	attachSystemInterrupt(SysInterrupt_SysTick, TestHandler()); // Override current handler
	assertEqual(TestHandler::count, 1); // Previous handler has been destroyed
	detachSystemInterrupt(SysInterrupt_SysTick);
	assertEqual(TestHandler::count, 1); // Detaching does not destroy the handler since this would prevent use in an ISR
}

test(INTERRUPTS_03_isisr_willpreempt_servicedirqn)
{
#if /* defined(STM32F10X_MD) || defined(STM32F10X_HD) || */ defined(STM32F2XX)
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

#endif // !HAL_PLATFORM_NRF52840

#if PLATFORM_ID == PLATFORM_PHOTON_PRODUCTION || PLATFORM_ID == PLATFORM_P1 || PLATFORM_ID == PLATFORM_ELECTRON_PRODUCTION
void trigger_falling_interrupt(pin_t pin) {
	auto pinmap = HAL_Pin_Map();
	static const uint16_t exti_line = pinmap[pin].gpio_pin;

	pinMode(pin, INPUT_PULLDOWN);
	EXTI_GenerateSWInterrupt(exti_line);
	pinMode(pin, INPUT_PULLUP);
}


test(INTERRUPTS_04_attachInterruptDirect) {
	const pin_t pin = D1;
	const IRQn_Type irqn = EXTI9_5_IRQn;
	auto pinmap = HAL_Pin_Map();
	static const uint16_t exti_line = pinmap[pin].gpio_pin;
	static volatile bool attachInterruptHandler = false;
	static volatile bool attachInterruptDirectHandler = false;

	pinMode(pin, INPUT_PULLUP);
	attachInterrupt(pin, (wiring_interrupt_handler_t)[](void) -> void {
		attachInterruptHandler = true;
	}, FALLING);

	// Trigger
	trigger_falling_interrupt(pin);

	// attachInterrupt handler should have been called
	assertTrue(attachInterruptHandler == true);
	attachInterruptHandler = false;

	// attach a direct handler
	bool res = attachInterruptDirect(irqn, []() {
		attachInterruptDirectHandler = true;
		EXTI_ClearFlag(exti_line);
	});
	assertTrue(res);

	// Trigger
	trigger_falling_interrupt(pin);

	// Only a direct handler should have been called
	assertTrue(attachInterruptDirectHandler == true);
	assertFalse(attachInterruptHandler == true);
	attachInterruptDirectHandler = false;
	attachInterruptHandler = false;

	// Detach, restore previous handler, do not disable
	res = detachInterruptDirect(irqn, false);
	assertTrue(res);

	// Trigger
	trigger_falling_interrupt(pin);

	// attachInterrupt handler should have been called
	assertTrue(attachInterruptHandler == true);
	assertFalse(attachInterruptDirectHandler == true);
	attachInterruptDirectHandler = false;
	attachInterruptHandler = false;

	// attach a direct handler
	res = attachInterruptDirect(irqn, []() {
		attachInterruptDirectHandler = true;
		EXTI_ClearFlag(exti_line);
	});
	assertTrue(res);

	// Trigger
	trigger_falling_interrupt(pin);

	// Only a direct handler should have been called
	assertTrue(attachInterruptDirectHandler == true);
	assertFalse(attachInterruptHandler == true);
	attachInterruptDirectHandler = false;
	attachInterruptHandler = false;

	// Detach, restore previous handler, _disable_
	res = detachInterruptDirect(irqn);
	assertTrue(res);

	// Trigger
	trigger_falling_interrupt(pin);

	// Not handler should have been called as IRQ should be disabled
	assertTrue(attachInterruptHandler == false);
	assertTrue(attachInterruptDirectHandler == false);
}

test(INTERRUPTS_04_attachInterruptDirect_1) {
	const pin_t pin = D1;

	detachInterrupt(pin);
}

test(INTERRUPTS_04_detachFromISR) {
	// This test validates that calling detachInterrupt in an ISR is safe when attachInterrupt was called with
	// an instance and member. This uses the same code path as attachInterrupt with a function pointer,
	// validating that as well. It also validates that we can replace the handler with a new one.

	const pin_t pin = D1;

	DetachingHandler first_handler(pin);
	DetachingHandler second_handler(pin);

	pinMode(pin, INPUT_PULLUP);
	attachInterrupt(pin, &DetachingHandler::handler, &first_handler, FALLING);

	// Trigger
	trigger_falling_interrupt(pin);

	assertEqual(1, first_handler.count());
	assertEqual(0, second_handler.count());

	attachInterrupt(pin, &DetachingHandler::handler, &second_handler, FALLING);

	// Trigger
	trigger_falling_interrupt(pin);

	assertEqual(1, first_handler.count());
	assertEqual(1, second_handler.count());
}
#endif // PLATFORM_ID == PLATFORM_PHOTON_PRODUCTION || PLATFORM_ID == PLATFORM_P1 || PLATFORM_ID == PLATFORM_ELECTRON_PRODUCTION

#if PLATFORM_ID == PLATFORM_ELECTRON_PRODUCTION
test(INTERRUPTS_05_attachInterruptD7) {
	const pin_t pin = D7;
	bool res = attachInterrupt(pin, nullptr, FALLING);
	bool tem = detachInterrupt(pin);
	assertFalse(res);
	assertFalse(tem);
}
#endif
