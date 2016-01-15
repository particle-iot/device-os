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

test(listening_update_event_is_sent) {
    listening_update_time = 0;
    System.on(wifi_listen_update, listening_update_handler); // Register handler

    listening_start();

    uint32_t start = millis();
    while (millis() - start < 1000); // Busy wait

    listening_stop();

    System.off(listening_update_handler); // Unregister handler

    // Handler function should be invoked only in the context of application thread
    assertEqual(listening_update_time, 0);

    // Process application queue
    start = millis();
    do {
        Particle.process();
    } while (listening_update_time == 0 && millis() - start < 1000);

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
    while (!listening_test_flag && millis() - time < 1000); // Busy wait

    listening_stop();

    assertTrue((bool)listening_test_flag);
}
