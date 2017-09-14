#include "Arduino.h"
#include "unit-test/unit-test.h"

test(ARDUINO_cbi_sbi_portOutputRegister_digitalPinToBitMask) {
    pinMode(D7, OUTPUT);
    auto port = digitalPinToPort(D7);
    auto pinmask = digitalPinToBitMask(D7);
    cbi(portOutputRegister(port), pinmask);
    assertEqual(digitalRead(D7), (int32_t)LOW);
    sbi(portOutputRegister(port), pinmask);
    assertEqual(digitalRead(D7), (int32_t)HIGH);
}

test(ARDUINO_cbi_sbi_portOutputRegister_digitalPinToBitMask_deinit) {
    pinMode(D7, INPUT);
}
