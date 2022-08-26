#include "Particle.h"
SYSTEM_MODE(SEMI_AUTOMATIC);
SerialLogHandler logHandler(LOG_LEVEL_ALL);
int count = 0;
int error = 0;
int test_errors = 0;
int state = 1;
int COUNT_MULTIPLIER = 1;
system_tick_t lastState = 0;
const system_tick_t TIME_FOR_5_PULSES = (20 * 5);
InterruptMode intType = RISING;
//
// We will set most pins to INPUT_x modes, but only test the first 7
// since PM_INT takes up one of the 8, and after that RISING and CHANGE pending interrupt tests will fail.
//

// Most of the tests start with D0
const int START_PIN = D0;
#if (PLATFORM_ID == PLATFORM_P2)
    const int PIN_MAX = S6; // TEST: D0 ~ D6 (however all will work on P2)
#elif (PLATFORM_ID == PLATFORM_TRACKERM)
    START_PIN = D8;         // Overwrite start pin for trackerm
    const int PIN_MAX = D9; // TEST: D8,D9
#elif (PLATFORM_ID == PLATFORM_ARGON) || (PLATFORM_ID == PLATFORM_BORON)
    const int PIN_MAX = A0; // TEST: D0 ~ D6
#elif (PLATFORM_ID == PLATFORM_B5SOM)
    const int PIN_MAX = D23;// TEST: D0 ~ D6
#elif (PLATFORM_ID == PLATFORM_BSOM)
    const int PIN_MAX = A6; // TEST: D0 ~ D6 (SDA, SCL, RTS, CTS, PWM0, PWM1, PWM2)
#elif (PLATFORM_ID == PLATFORM_ESOMX)
    const int PIN_MAX = D9; // TEST: D0, D1, D2, D5, D8, D9
#elif (PLATFORM_ID == PLATFORM_TRACKER)
    const int PIN_MAX = D9; // TEST: D0 ~ D3 (D4,D6 will error like shared PORT event interrupts do)
#else
    #error "Platform not supported!"
#endif

// required for B SoM / B5SoM
#if defined(HAL_PLATFORM_POWER_MANAGEMENT_OPTIONAL) && HAL_PLATFORM_POWER_MANAGEMENT_OPTIONAL
// Disable runtime PMIC / FuelGauge detection so we can test D0/D1 (SDA/SCL)
STARTUP(System.disable(SYSTEM_FLAG_PM_DETECTION));
#endif // defined(HAL_PLATFORM_POWER_MANAGEMENT_OPTIONAL) && HAL_PLATFORM_POWER_MANAGEMENT_OPTIONAL

pin_t PULSE_PIN = A5; // A randomly higher up pin that's available on all platforms (and won't be in the test as an interrupt pin)

static void attach_interrupt_handler() {
    count++;
}

void gimmeFivePulses() {
    for (int x = 0; x < 5; x++) {
        digitalWrite(PULSE_PIN, HIGH);
        delay(10);
        digitalWrite(PULSE_PIN, LOW);
        delay(10);
    }
}

void teardownTest() {
    for (int i = START_PIN; i <= PIN_MAX; i++) {
        if (i == PULSE_PIN) continue;
        if (i >= D7) continue;
#if (PLATFORM_ID == PLATFORM_BSOM)
        // if (i == SCL || i == SDA) continue; // exclude SCL & SDA
#elif (PLATFORM_ID == PLATFORM_ESOMX && SYSTEM_VERSION <= SYSTEM_VERSION_BETA(4,0,0,1))
        if (i == D3 || i == D3 || i == D6 || i == D7 || i == D22 || i == D23) continue; // these don't exist
#endif
        detachInterrupt(i);
    }
}

void setupTest(InterruptMode type) {
    Log.info("TEST (%s) START", type==RISING?"RISING":type==FALLING?"FALLING":"CHANGE");
    for (int i = START_PIN; i <= PIN_MAX; i++) {
        if (i == PULSE_PIN) continue;
        if (type == RISING) {
            pinMode(i, INPUT_PULLDOWN);
        } else if (type == FALLING) {
            pinMode(i, INPUT_PULLUP);
        } else {
            pinMode(i, INPUT_PULLUP); // change
        }
    }
    for (int i = START_PIN; i <= PIN_MAX; i++) {
        if (i == PULSE_PIN) continue;
        if (i >= D7) continue;
#if (PLATFORM_ID == PLATFORM_BSOM)
        // if (i == SCL || i == SDA) continue; // exclude SCL & SDA
#elif (PLATFORM_ID == PLATFORM_ESOMX && SYSTEM_VERSION <= SYSTEM_VERSION_BETA(4,0,0,1))
        if (i == D3 || i == D3 || i == D6 || i == D7 || i == D22 || i == D23) continue; // these don't exist
#endif
        attachInterrupt(i, attach_interrupt_handler, type);
    }
    delay(10);
    error = 0;
    state = 1;
    COUNT_MULTIPLIER = 1;
    count = 0;
    gimmeFivePulses();
    if (type == CHANGE || type == FALLING) {
        if (type == CHANGE) {
            COUNT_MULTIPLIER = 2; // double counts if testing CHANGE edge interrupts
        }
    }
    lastState = millis();
}

void runTest(InterruptMode type) {
    switch (state) {
    case 0:
    default: {
        break;
    }
    case 1: {
        if (count == 5 * COUNT_MULTIPLIER) {
            Log.info("Interrupts are functioning correctly, count: %d", count);
            state++;
            noInterrupts(); // Single disable
            lastState = millis();
        } else if (millis() - lastState >= TIME_FOR_5_PULSES) {
            Log.error("Interrupts are not functioning correctly, count: %d", count);
            noInterrupts(); // Single disable
            error++;
            count = 5 * COUNT_MULTIPLIER;
            gimmeFivePulses();
            lastState = millis();
            state++;
        }
        break;
    }
    case 2: {
        if (millis() - lastState >= TIME_FOR_5_PULSES) {
            if (count == 5 * COUNT_MULTIPLIER) {
                Log.info("Interrupts were disabled, count still: %d", count);
            } else {
                Log.error("Interrupts were not disabled, count now: %d", count);
                error++;
            }
            count = 5 * COUNT_MULTIPLIER;
            gimmeFivePulses();
            lastState = millis();
            interrupts(); // Single enable
            state++;
        }
        break;
    }
    case 3: {
        if (millis() - lastState >= TIME_FOR_5_PULSES) {
            if (count > 5 * COUNT_MULTIPLIER) {
                Log.info("Pending interrupts not cleared before enabling interrupts, count now: %d", count);
            } else {
                Log.error("Pending interrupts were cleared before enabling interrupts, count still: %d", count);
                error++;
            }
            lastState = millis();
            count = 5 * COUNT_MULTIPLIER;
            gimmeFivePulses();
            state++;
        }
        break;
    }
    case 4: {
        if (count == 10 * COUNT_MULTIPLIER) {
            Log.info("Interrupts are functioning correctly, count: %d", count);
            lastState = millis();
            noInterrupts(); // Double disable
            noInterrupts();
            state++;
            gimmeFivePulses();
        } else if (millis() - lastState >= TIME_FOR_5_PULSES) {
            Log.error("Interrupts are not functioning correctly, count: %d", count);
            error++;
            lastState = millis();
            noInterrupts(); // Double disable
            noInterrupts();
            state++;
            count = 10 * COUNT_MULTIPLIER;
            gimmeFivePulses();
        }
        break;
    }
    case 5: {
        if (millis() - lastState >= TIME_FOR_5_PULSES) {
            if (count == 10 * COUNT_MULTIPLIER) {
                Log.info("Interrupts were disabled, count still: %d", count);
            } else {
                Log.error("Interrupts were not disabled, count now: %d", count);
                error++;
            }
            interrupts(); // Double enable
            interrupts();
            count = 10 * COUNT_MULTIPLIER;
            gimmeFivePulses();
            lastState = millis();
            state++;
        }
        break;
    }
    case 6: {
        if (millis() - lastState >= TIME_FOR_5_PULSES) {
            if (count > 10 * COUNT_MULTIPLIER) {
                Log.info("Pending interrupts not cleared before enabling interrupts, count now: %d", count);
            } else {
                Log.error("Pending interrupts were cleared before enabling interrupts, count still: %d", count);
                error++;
            }
            lastState = millis();
            count = 10 * COUNT_MULTIPLIER;
            gimmeFivePulses();
            state++;
        }
        break;
    }
    case 7: {
        if (count == 15 * COUNT_MULTIPLIER) {
            // disablePulses();
            Log.info("Interrupts are functioning correctly, count: %d", count);
            state++;
        } else if (millis() - lastState >= TIME_FOR_5_PULSES) {
            // disablePulses();
            Log.error("Interrupts are not functioning correctly, count: %d", count);
            error++;
            state++;
        }
        break;
    }
    case 8: {
        interrupts();
        if (error > 0) {
            Log.info("TEST (%s) FAILED, Error Count: %d", type==RISING?"RISING":type==FALLING?"FALLING":"CHANGE", error);
            RGB.color(255,0,0); // RED
            test_errors++;
        } else {
            Log.info("TEST (%s) PASSED", type==RISING?"RISING":type==FALLING?"FALLING":"CHANGE");
            RGB.color(0,255,0); // GREEN
        }
        delay(500);
        RGB.color(0,0,0); // NONE
        state = 0;
    }
    } // switch
}

void setup() {
    waitFor(Serial.isConnected, 10000);
    delay(200);
    Log.info("TESTS START");
    Log.info("Connect PULSE_PIN(A5) to any other pin (e.g. D0 ~ D6) and reset the device to start the test");
    Log.info("TRIES EACH INTERRUPT TYPE FOR ALL PINS AUTOMATICALLY (RISING, FALLING, CHANGE)");
    RGB.control(true);
    pinMode(PULSE_PIN, OUTPUT);
    digitalWrite(PULSE_PIN, LOW);
    setupTest(intType);
}

void loop() {
    if (state == 0) {
        if (intType == RISING) {
            intType = FALLING;
        } else if (intType == FALLING) {
            intType = CHANGE;
        } else {
            if (test_errors > 0) {
                Log.info("TESTS FAILED: %d/3", test_errors);
                RGB.color(255,0,0); // RED
            } else {
                Log.info("TESTS PASSED: 3/3");
                RGB.color(0,255,0); // GREEN
            }
            Log.info("ALL TESTS COMPLETE!!!");
            while(1) Particle.process();
        }
        teardownTest();
        setupTest(intType);
    }
    runTest(intType);
}
