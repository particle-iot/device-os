#include "application.h"
#include "unit-test/unit-test.h"

#if PLATFORM_THREADING
#include "system_threading.h"
#include <functional>

volatile bool listening_test_flag;

void network_power_cycle() {
#if Wiring_WiFi
    WiFi.off();
    WiFi.on();
#elif Wiring_Cellular
    Cellular.off();
    Cellular.on();
#endif
}

void network_set_listen_timeout(uint16_t timeout) {
#if Wiring_WiFi
    WiFi.setListenTimeout(timeout);
#elif Wiring_Cellular
    Cellular.setListenTimeout(timeout);
#endif
}

uint16_t network_get_listen_timeout() {
#if Wiring_WiFi
    return WiFi.getListenTimeout();
#elif Wiring_Cellular
    return Cellular.getListenTimeout();
#endif
    return 0;
}

void network_listen(bool start) {
#if Wiring_WiFi
    WiFi.listen(start);
#elif Wiring_Cellular
    Cellular.listen(start);
#endif
}

bool network_listening() {
#if Wiring_WiFi
    return WiFi.listening();
#elif Wiring_Cellular
    return Cellular.listening();
#endif
    return 0;
}

// used to create a sync call to system thread
void blocking_call() {
#if Wiring_WiFi
    WiFi.hasCredentials();
#elif Wiring_Cellular
    // Cellular.hasCredentials(); // no Wiring API
    delay(1000); // todo - find a better way to interact with the system thread
#endif
}

void listening_start_with_delay() {
    waitFor(Particle.connected, 30000);
    if (!Particle.connected()) {
        Particle.disconnect();
        network_power_cycle();
        Particle.connect();
        waitFor(Particle.connected, 100000);
    }
    assertTrue(Particle.connected()); // make sure we are not stuck trying to handshake before trying to enter listening mode
    listening_test_flag = false;
    network_listen(true);
    delay(1000); // Time for the system thread to enter listening mode
}

void listening_stop_with_delay() {
    network_listen(false);
    delay(1000); // Time for the system thread to exit listening mode
}

void listening_set_test_flag() {
    listening_test_flag = true;
}

// Following test ensures that handler function is invoked for SystemEvents::wifi_listen_update events
volatile int listening_update_time;

void listening_update_handler(system_event_t, int time, void*) {
    listening_update_time = time;
}

volatile int test_val;
void increment(void)
{
    test_val++;
}

test(01_THREADING_listening_timeout_default_matches_expected) {
#if Wiring_WiFi
    assertEqual(network_get_listen_timeout(),0); // default no timeout
#elif Wiring_Cellular
    assertEqual(network_get_listen_timeout(),300); // default 5 minute timeout
#endif
    network_set_listen_timeout(0); // disable for now
}

test(02_THREADING_system_thread_can_pump_events)
{
    network_listen(false);
    test_val = 0;

    ActiveObjectBase* system = (ActiveObjectBase*)system_internal(1, nullptr); // Returns system thread instance
    system->invoke_async(std::function<void()>(increment));

    uint32_t start = millis();
    while (test_val != 1 && millis() - start < 4000); // Busy wait

    assertEqual((int)test_val, 1);
}

test(03_THREADING_application_thread_can_pump_events)
{
    test_val = 0;

    ActiveObjectBase* app = (ActiveObjectBase*)system_internal(0, nullptr); // Returns application thread instance
    std::function<void(void)> fn = increment;
    app->invoke_async(fn);

    // test value not incremented
    assertEqual((int)test_val, 0);

    Particle.process();

    // validate the function was called.
    assertEqual((int)test_val, 1);
}

/**
 * Validates that the listening event is sent to the application.
 * This is a white box test - verifies that both the system event loop is
 * running and that the application can pump events by calling Particle.process()
 *
 */
test(04_THREADING_listening_update_event_is_sent) {
    system_unsubscribe_event(all_events, nullptr, nullptr);

    listening_update_time = 0;
    System.on(wifi_listen_update, listening_update_handler); // Register handler
    listening_start_with_delay();
    assertTrue(network_listening());

    uint32_t start = millis();
    while (!listening_update_time && ((millis() - start) < 4000)) {
        Particle.process();   // pump application events
    }
    listening_stop_with_delay();
    System.off(listening_update_handler); // Unregister handler

    assertMore((int)listening_update_time, 0);
}

// This test ensures the RTOS time slices between threads when the system is in listening mode
test(05_THREADING_listening_application_thread_is_running) {
    listening_start_with_delay();
    assertTrue(network_listening());

    uint32_t start = millis();
    while (millis() - start < 1000); // Busy wait
    uint32_t end = millis();

    listening_stop_with_delay();

    assertLess(end - start, 1200); // Small margin of error
}

// Checks whether listening loop performs processing of the system thread's queued events
test(06_THREADING_listening_loop_is_processing_system_events) {
    listening_start_with_delay();
    assertTrue(network_listening());

    ActiveObjectBase *system = (ActiveObjectBase*)system_internal(1, nullptr); // Returns system thread instance
    system->invoke_async(std::function<void()>(listening_set_test_flag));

    uint32_t time = millis();
    while (!listening_test_flag && ((millis() - time) < 4000)); // Busy wait

    listening_stop_with_delay();

    assertTrue((bool)listening_test_flag);
}

test(07_THREADING_app_can_exit_listen_mode_761)
{
    // this isn't a complete test - we can't tell if the system thread
    // is still stuck in the listening loop
    listening_start_with_delay();
    assertTrue(network_listening());
    network_listen(false);
    Particle.process();
    delay(10);	// todo - find a better way to interact with the system thread
    assertFalse(network_listening());
}

test(08_THREADING_app_can_exit_listen_mode_via_timeout)
{
    network_set_listen_timeout(3);
    listening_start_with_delay();
    assertTrue(network_listening());
    assertEqual(network_get_listen_timeout(),3);
    Particle.process();
    delay(5000);  // todo - find a better way to interact with the system thread
    assertFalse(network_listening());
}

test(09_THREADING_app_can_cancel_listen_mode_timeout)
{
    network_set_listen_timeout(3);
    listening_start_with_delay();
    assertTrue(network_listening());
    assertEqual(network_get_listen_timeout(),3);
    network_set_listen_timeout(0);
    assertEqual(network_get_listen_timeout(),0);
    Particle.process();
    delay(5000);  // todo - find a better way to interact with the system thread
    assertTrue(network_listening()); // still in listening mode

    listening_stop_with_delay(); // force exit listening mode
    assertFalse(network_listening());
}

test(10_THREADING_app_can_extend_listen_mode_timeout)
{
    network_set_listen_timeout(3);
    listening_start_with_delay();
    assertTrue(network_listening());
    assertEqual(network_get_listen_timeout(),3);
    network_set_listen_timeout(6);
    assertEqual(network_get_listen_timeout(),6);
    Particle.process();
    delay(5000);  // todo - find a better way to interact with the system thread
    assertTrue(network_listening()); // still in listening mode
    delay(2000);
    assertFalse(network_listening()); // exited listening mode
    network_set_listen_timeout(0); // disable for rest of tests
}

test(11_THREADING_app_can_invoke_synchronous_fn_in_listening_mode)
{
    network_listen(true);
    delay(10);  // time for the system thread to enter listening mode

    // this is a blocking call that requires the system thread to
    blocking_call();

    network_listen(false);
}

test(12_THREADING_publish_during_listening_mode_issue_761)
{
    listening_start_with_delay();

    bool result = Particle.publish("mdma", "codez");
    network_listen(false);
    assertFalse(result);
}

#else // NO PLATFORM_THREADING
test(00_THREADING_no_threading_on_this_device)
{
    assertTrue(true);
}
#endif // PLATFORM_THREADING

