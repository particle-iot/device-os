
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
