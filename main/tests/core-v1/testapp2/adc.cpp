
#include "application.h"
#include "unit-test/unit-test.h"

test(ADC_NoAnalogReadWhenPinSelectedIsOutOfRange) {
    pin_t pin = 23;//pin under test (not a valid user pin)
    // when
    int32_t ADCValue = analogRead(pin);
    // then
    assertFalse(ADCValue!=0);
    //To Do : Add test for remaining pins if required
}

test(ADC_AnalogReadOnPinWithVoltageDividerResultsInCorrectValue) {
    pin_t pin = A5;//pin under test (Voltage divider with equal resistor values)
    // when
    int32_t ADCValue = analogRead(pin);
    // then
    assertTrue((ADCValue>2000)&&(ADCValue<2100));//ADCValue should be around 2048
    //To Do : Add test for remaining pins if required
}
