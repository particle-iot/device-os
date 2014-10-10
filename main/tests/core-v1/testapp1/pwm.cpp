
#include "application.h"
#include "pwm_hal.h"
#include "unit-test/unit-test.h"

test(PWM_NoAnalogWriteWhenPinModeIsNotSetToOutput) {
    pin_t pin = A0;//pin under test
    // when
    pinMode(pin, INPUT);//pin set to INPUT mode
    analogWrite(pin, 50);
    // then
    assertNotEqual(HAL_PWM_Get_AnalogValue(pin), 50);
    //To Do : Add test for remaining pins if required
}

test(PWM_NoAnalogWriteWhenPinSelectedIsNotTimerChannel) {
    pin_t pin = D5;//pin under test
    // when
    pinMode(pin, OUTPUT);//D5 is not a Timer channel
    analogWrite(pin, 100);
    // then
    //analogWrite works on fixed PWM frequency of 500Hz
    assertNotEqual(HAL_PWM_Get_Frequency(pin), TIM_PWM_FREQ);
    //To Do : Add test for remaining pins if required
}

test(PWM_NoAnalogWriteWhenPinSelectedIsOutOfRange) {
    pin_t pin = 21;//pin under test (not a valid user pin)
    // when
    pinMode(pin, OUTPUT);//21 is not a user pin
    analogWrite(pin, 100);
    // then
    assertNotEqual(HAL_PWM_Get_AnalogValue(pin), 100);
    //To Do : Add test for remaining pins if required
}

test(PWM_AnalogWriteOnPinResultsInCorrectFrequency) {
    pin_t pin = A0;//pin under test
    // when
    pinMode(pin, OUTPUT);
    analogWrite(pin, 150);
    // then
    //analogWrite works on fixed PWM frequency of 500Hz
    assertEqual(HAL_PWM_Get_Frequency(pin), TIM_PWM_FREQ);
    //To Do : Add test for remaining pins if required
}

test(PWM_AnalogWriteOnPinResultsInCorrectAnalogValue) {
    pin_t pin = A1;//pin under test
    // when
    pinMode(pin, OUTPUT);
    analogWrite(pin, 200);
    // then
    assertEqual(HAL_PWM_Get_AnalogValue(pin), 200);
    //To Do : Add test for remaining pins if required
}
