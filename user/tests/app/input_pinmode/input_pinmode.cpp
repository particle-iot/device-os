#include "Particle.h"
SYSTEM_MODE(SEMI_AUTOMATIC);
SerialLogHandler logHandler(LOG_LEVEL_ALL);
bool switch_test = false;
int count = 0;
int error = 0;
int test_errors = 0;
int state = 1;
int COUNT_MULTIPLIER = 1;
system_tick_t lastState = 0;
const system_tick_t TIME_FOR_5_PULSES = (20 * 5);
InterruptMode intType = CHANGE;
//
// We will set most pins to INPUT_x modes, but only test the first 7
// since PM_INT takes up one of the 8, and after that RISING and CHANGE pending interrupt tests will fail.
//
#if (PLATFORM_ID == PLATFORM_P2) || (PLATFORM_ID == PLATFORM_TRACKERM)
    const int PIN_MAX = D6; // TEST: D0 ~ D6 (however all will work on P2)
#elif (PLATFORM_ID == PLATFORM_ARGON) || (PLATFORM_ID == PLATFORM_BORON)
    const int PIN_MAX = D6; // TEST: D0 ~ D6
#elif (PLATFORM_ID == PLATFORM_B5SOM)
    const int PIN_MAX = D6;// TEST: D0 ~ D6
#elif (PLATFORM_ID == PLATFORM_BSOM)
    const int PIN_MAX = D6; // TEST: D0 ~ D6 (SDA, SCL, RTS, CTS, PWM0, PWM1, PWM2)
#elif (PLATFORM_ID == PLATFORM_ESOMX)
    const int PIN_MAX = D9; // TEST: D0, D1, D2, D5, D8, D9
#elif (PLATFORM_ID == PLATFORM_TRACKER)
    const int PIN_MAX = D6; // TEST: D0 ~ D3 (D4,D6 will error like shared PORT event interrupts do)
#else
    #error "Platform not supported!"
#endif

// required for B SoM / B5SoM
#if defined(HAL_PLATFORM_POWER_MANAGEMENT_OPTIONAL) && HAL_PLATFORM_POWER_MANAGEMENT_OPTIONAL
// Disable runtime PMIC / FuelGauge detection so we can test D0/D1 (SDA/SCL)
STARTUP(System.disable(SYSTEM_FLAG_PM_DETECTION));
#endif // defined(HAL_PLATFORM_POWER_MANAGEMENT_OPTIONAL) && HAL_PLATFORM_POWER_MANAGEMENT_OPTIONAL

pin_t PULSE_PIN = A5; // A randomly higher up pin that's available on all platforms (and won't be in the test as an interrupt pin)

void button_handler(system_event_t event, int clicks) {
    if (event == button_final_click) {
        if (clicks >= 1) {
            switch_test = true;
        }
    }
}

static void attach_interrupt_handler() {
    count++;
}

void teardownTest() {
    for (int i = 0; i <= PIN_MAX; i++) {
        if (i >= D7) continue;
#if (PLATFORM_ID == PLATFORM_ESOMX && SYSTEM_VERSION <= SYSTEM_VERSION_BETA(4,0,0,1))
        if (i == D3 || i == D3 || i == D6 || i == D7 || i == D22 || i == D23) continue; // these don't exist
#endif
        detachInterrupt(i);
    }
}

void setupTest(InterruptMode type) {
    static int pinModeType = 0;
    Log.info("CHECK (%s) WITH (%s)", type == RISING ? "RISING" : type == FALLING ? "FALLING" : "CHANGE",
            pinModeType == 0 ? "INPUT" : pinModeType == 1 ? "INPUT_PULLUP" : "INPUT_PULLDOWN");
    digitalWriteFast(A5, HIGH);
    delayMicroseconds(10);
    digitalWriteFast(A5, LOW);
    for (int i = 0; i <= PIN_MAX; i++) {
        if (pinModeType == 0) {
            pinMode(i, INPUT);
        } else if (pinModeType == 1) {
            pinMode(i, INPUT_PULLUP);
        } else {
            pinMode(i, INPUT_PULLDOWN);
        }
    }
    if (type == CHANGE) {
        pinModeType++;
        pinModeType = pinModeType % 2;
    }
    digitalWriteFast(A5, HIGH);
    delayMicroseconds(10);
    digitalWriteFast(A5, LOW);
    for (int i = 0; i <= PIN_MAX; i++) {
        if (i >= D7) continue;
#if (PLATFORM_ID == PLATFORM_ESOMX && SYSTEM_VERSION <= SYSTEM_VERSION_BETA(4,0,0,1))
        if (i == D3 || i == D3 || i == D6 || i == D7 || i == D22 || i == D23) continue; // these don't exist
#endif
        attachInterrupt(i, attach_interrupt_handler, type);
    }
    digitalWriteFast(A5, HIGH);
    delayMicroseconds(10);
    digitalWriteFast(A5, LOW);
}

void setup() {
    waitFor(Serial.isConnected, 10000);
    delay(200);
    Log.info("Attach a scope to A5, and test input D0 ~ D6 one at a time.");
    Log.info("Check that test input mode only changes between the first two pulses on A5,");
    Log.info("and not the second and third. Press the MODE button to cycle through all 9");
    Log.info("configurations (INPUT/INPUT_PULLUP/INPUT_PULLDOWN) with (RISING/FALLING/CHANGE).");
    pinMode(A5, OUTPUT);
    System.on(button_final_click, button_handler);
}

void loop() {
    if (switch_test) {
        switch_test = false;
        if (intType == CHANGE) {
            intType = RISING;
        } else if (intType == RISING) {
            intType = FALLING;
        } else if (intType == FALLING) {
            intType = CHANGE;
        }
        teardownTest();
        setupTest(intType);
    }
}