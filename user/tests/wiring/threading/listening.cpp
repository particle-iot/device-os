#include "application.h"
#include "unit-test/unit-test.h"
#include "system_threading.h"

volatile bool listening_test_flag;

void listening_start() {
    listening_test_flag = false;
    WiFi.listen(true);
    delay(1000); // Time for the system thread to enter listening mode
}

void listening_stop() {
    WiFi.listen(false);
    delay(1000); // Time for the system thread to exit listening mode
}

void listening_set_test_flag() {
    listening_test_flag = true;
}

// Following test ensures that handler function is invoked for SystemEvents::wifi_listen_update events
uint32_t listening_update_time;

void listening_update_handler(system_event_t, uint32_t time, void*) {
    listening_update_time = time;
}

/**
 * Validates that the listening event is sent to the application.
 * This is a white box test - verifies that both the system event loop is
 * running and that the application can pump events by calling Particle.process()
 *
 */
test(listening_update_event_is_sent) {
    system_unsubscribe_event(all_events, nullptr, nullptr);

    listening_update_time = 0;
	System.on(wifi_listen_update, listening_update_handler); // Register handler
    listening_start();

    uint32_t start = millis();
    while (!listening_update_time && ((millis() - start) < 4000)) {
    		Particle.process();		// pump application events
    }
    listening_stop();
    System.off(listening_update_handler); // Unregister handler

    assertMore(listening_update_time, 0);
}

// This test ensures the RTOS time slices between threads when the system is in listening mode
test(listening_application_thread_is_running) {
    listening_start();

    uint32_t start = millis();
    while (millis() - start < 1000); // Busy wait
    uint32_t end = millis();

    listening_stop();

    assertLess(end - start, 1200); // Small margin of error
}

// Checks whether listening loop performs processing of the system thread's queued events
test(listening_loop_is_processing_system_events) {
    listening_start();

    ActiveObjectBase *system = (ActiveObjectBase*)system_internal(1, nullptr); // Returns system thread instance
    system->invoke_async(std::function<void()>(listening_set_test_flag));

    uint32_t time = millis();
    while (!listening_test_flag && ((millis() - time) < 4000)); // Busy wait

    listening_stop();

    assertTrue((bool)listening_test_flag);
}


test(app_can_exit_listen_mode_761)
{
	// this isn't a complete test - we can't tell if the system thread
	// is still stuck in the listening loop
	uint32_t start = millis();
	listening_start();
	WiFi.listen(false);
	Particle.process();
	delay(10);	// todo - find a better way to interact with the system thread
	assertFalse(WiFi.listening());
}

test(app_can_invoke_synchronous_fn_in_listening_mode)
{
	WiFi.listen();
	delay(10);		// time for the system thread to enter listening mode

	// this is a blocking call that requires the system thread to
	WiFi.hasCredentials();

	WiFi.listen(false);
}

test(publish_during_listening_mode_issue_761)
{
	listening_start();

	bool result = Particle.publish("mdma", "codez");
	WiFi.listen(false);
	assertFalse(result);
}


