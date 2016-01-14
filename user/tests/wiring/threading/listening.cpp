#include "application.h"
#include "unit-test/unit-test.h"
#include "system_threading.h"

volatile bool listening_flag = false;

void listening_start() {
    listening_flag = false;
    WiFi.listen();
    delay(100); // Time for the system thread to enter listening loop
}

void listening_stop() {
    WiFi.listen(false);
    delay(100); // Time for the system thread to exit listening loop
}

void listening_set_flag() {
    listening_flag = true;
}

// Checks whether listening loop performs processing of the system thread's queued events
test(listening_loop_is_processing_system_events) {
    listening_start();

    ActiveObjectBase *system = (ActiveObjectBase*)system_internal(1 /* System thread */, nullptr);
    system->invoke_async(std::function<void()>(listening_set_flag));

    uint32_t time = millis();
    while (!listening_flag && millis() - time < 500); // Busy wait

    listening_stop();

    assertTrue((bool)listening_flag);
}
