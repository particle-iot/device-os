#include "application.h"
#include "unit-test/unit-test.h"
#include "system_threading.h"

static retained uint32_t magick = 0;
static void startup();

bool threading_state = false;

STARTUP(startup());


static bool will_process_events()
{
    if ((!PLATFORM_THREADING || application_thread_current(nullptr)) && !HAL_IsISR())
        return true;
    return false;
}

void startup()
{
    System.enableFeature(FEATURE_RETAINED_MEMORY);
    if (magick == 0xdeadbeef) {
        // Switch to SYSTEM_THREAD(ENABLED);
        magick = 0;
        system_thread_set_state(spark::feature::ENABLED, NULL);
        threading_state = true;
    } else {
        // Switch to SYSTEM_THREAD(DISABLED);
        magick = 0;
        system_thread_set_state(spark::feature::DISABLED, NULL);
        threading_state = false;
    }
}

test(THREADING_00_no_threading_app_and_system_are_both_current_and_will_process_events_when_delay_is_called)
{
    if (threading_state) {
        pass();
        return;
    }
    assertTrue((bool)application_thread_current(nullptr));
    assertTrue((bool)system_thread_current(nullptr));
    assertTrue(will_process_events());
}

test(THREADING_01_no_threading_create_thread_and_check_it_is_not_application_or_system_current_and_will_not_process_events_when_delay_is_called)
{
    if (threading_state) {
        pass();
        return;
    }
    uint8_t app_current = 0xAA;
    uint8_t system_current = 0xAA;
    uint8_t will_process = 0xAA;
    Thread* thread = new Thread("test", [&](void) {
        app_current = application_thread_current(nullptr);
        system_current = system_thread_current(nullptr);
        will_process = will_process_events();
    });
    uint32_t m = millis();
    while (app_current == 0xAA) {
        assertLessOrEqual((millis() - m), 5000);
    }
    thread->join();
    assertNotEqual(app_current, 0xAA);
    assertNotEqual(system_current, 0xAA);
    assertNotEqual(will_process, 0xAA);

    assertFalse((bool)app_current);
    assertFalse((bool)system_current);
    assertFalse((bool)will_process);

    delete thread;
}

test(THREADING_02_no_threading_create_timer_and_check_it_is_not_application_or_system_current_and_will_not_process_events_when_delay_is_called)
{
    if (threading_state) {
        pass();
        return;
    }
    uint8_t app_current = 0xAA;
    uint8_t system_current = 0xAA;
    uint8_t will_process = 0xAA;
    Timer* timer = new Timer(10, [&]() {
        app_current = application_thread_current(nullptr);
        system_current = system_thread_current(nullptr);
        will_process = will_process_events();
    }, true);

    timer->start();

    uint32_t m = millis();
    while (app_current == 0xAA) {
        assertLessOrEqual((millis() - m), 5000);
    }

    assertNotEqual(app_current, 0xAA);
    assertNotEqual(system_current, 0xAA);
    assertNotEqual(will_process, 0xAA);

    assertFalse((bool)app_current);
    assertFalse((bool)system_current);
    assertFalse((bool)will_process);

    delete timer;
}

test(THREADING_03_no_threading_restart) {
    if (threading_state) {
        pass();
        return;
    }

    magick = 0xdeadbeef;
    Serial.println("The device will now reset, please rerun the tests once it boots");
    int d = 5;
    while (d >= 0) {
        Serial.printf("%d... ", d);
        delay(1000);
        d--;
    }
    Serial.println();
    Serial.println("BLASTOFF! Erm, uh, RESET!");
    delay(1000);
    System.reset();
}

test(THREADING_04_threading_app_is_current_system_is_not_will_not_process_events_when_delay_is_called)
{
    assertTrue((bool)application_thread_current(nullptr));
    assertFalse((bool)system_thread_current(nullptr));
    assertTrue(will_process_events());
}

test(THREADING_05_threading_create_thread_and_check_it_is_not_application_or_system_current_and_will_not_process_events_when_delay_is_called)
{
    if (!threading_state) {
        fail();
        return;
    }
    uint8_t app_current = 0xAA;
    uint8_t system_current = 0xAA;
    uint8_t will_process = 0xAA;
    Thread* thread = new Thread("test", [&](void) {
        app_current = application_thread_current(nullptr);
        system_current = system_thread_current(nullptr);
        will_process = will_process_events();
    });

    uint32_t m = millis();
    while (app_current == 0xAA) {
        assertLessOrEqual((millis() - m), 5000);
    }

    assertNotEqual(app_current, 0xAA);
    assertNotEqual(system_current, 0xAA);
    assertNotEqual(will_process, 0xAA);

    assertFalse((bool)app_current);
    assertFalse((bool)system_current);
    assertFalse((bool)will_process);

    delete thread;
}

test(THREADING_06_threading_create_timer_and_check_it_is_not_application_or_system_current_and_will_not_process_events_when_delay_is_called)
{
    if (!threading_state) {
        fail();
        return;
    }
    uint8_t app_current = 0xAA;
    uint8_t system_current = 0xAA;
    uint8_t will_process = 0xAA;
    Timer* timer = new Timer(10, [&]() {
        app_current = application_thread_current(nullptr);
        system_current = system_thread_current(nullptr);
        will_process = will_process_events();
    }, true);

    timer->start();

    uint32_t m = millis();
    while (app_current == 0xAA) {
        assertLessOrEqual((millis() - m), 5000);
    }

    assertNotEqual(app_current, 0xAA);
    assertNotEqual(system_current, 0xAA);
    assertNotEqual(will_process, 0xAA);

    assertFalse((bool)app_current);
    assertFalse((bool)system_current);
    assertFalse((bool)will_process);

    delete timer;
}

test(THREADING_07_threading_execute_async_on_system_thread_check_it_is_current_and_will_not_process_events_when_delay_is_called)
{
    if (!threading_state) {
        fail();
        return;
    }
    uint8_t app_current = 0xAA;
    uint8_t system_current = 0xAA;
    uint8_t will_process = 0xAA;
    
    ActiveObjectBase* system = (ActiveObjectBase*)system_internal(1, nullptr); // Returns system thread instance
    std::function<void(void)> f = [&](){
        app_current = application_thread_current(nullptr);
        system_current = system_thread_current(nullptr);
        will_process = will_process_events();
    };
    system->invoke_async(f);

    uint32_t m = millis();
    while (app_current == 0xAA) {
        assertLessOrEqual((millis() - m), 5000);
    }

    assertNotEqual(app_current, 0xAA);
    assertNotEqual(system_current, 0xAA);
    assertNotEqual(will_process, 0xAA);

    assertFalse((bool)app_current);
    assertTrue((bool)system_current);
    assertFalse((bool)will_process);
}
