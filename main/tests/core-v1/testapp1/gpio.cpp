
#include "application.h"
#include "unit-test/unit-test.h"

test(GPIO_PinModeSetResultsInCorrectMode) {
    PinMode mode[] = {
            OUTPUT,
            INPUT,
            INPUT_PULLUP,
            INPUT_PULLDOWN,
            AF_OUTPUT_PUSHPULL,
            AF_OUTPUT_DRAIN,
            AN_INPUT
    };
    int n = sizeof(mode) / sizeof(mode[0]);
    pin_t pin = A0;//pin under test
    for(int i=0;i<n;i++)
    {
        // when
        pinMode(pin, mode[i]);
        // then
        assertEqual(HAL_Get_Pin_Mode(pin), mode[i]);
    }
    //To Do : Add test for remaining pins if required
}

test(GPIO_NoDigitalWriteWhenPinModeIsNotSetToOutput) {
    pin_t pin = D0;//pin under test
    // when
    pinMode(pin, INPUT);//pin set to INPUT mode
    digitalWrite(pin, HIGH);
    // then
    assertNotEqual((PinState)HAL_GPIO_Read(pin), HIGH);
    //To Do : Add test for remaining pins if required
}

test(GPIO_NoDigitalWriteWhenPinSelectedIsOutOfRange) {
    pin_t pin = 21;//pin under test (not a valid user pin)
    // when
    pinMode(pin, OUTPUT);
    digitalWrite(pin, HIGH);
    // then
    assertNotEqual((PinState)HAL_GPIO_Read(pin), HIGH);
    //To Do : Add test for remaining pins if required
}

test(GPIO_DigitalWriteOnPinResultsInCorrectDigitalRead) {
    pin_t pin = D0;//pin under test
    // when
    pinMode(pin, OUTPUT);
    digitalWrite(pin, HIGH);
    PinState pinState = (PinState)digitalRead(pin);
    // then
    assertEqual(pinState, HIGH);
    delay(100);
    // when
    digitalWrite(pin, LOW);
    pinState = (PinState)digitalRead(pin);
    // then
    assertEqual(pinState, LOW);
    //To Do : Add test for remaining pins if required
}
