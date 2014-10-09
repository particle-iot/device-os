
#include "application.h"
#include "unit-test/unit-test.h"

test(PWM_NoAnalogWriteWhenPinModeIsNotSetToOutput) {
    // when
    pinMode(A0, INPUT);//pin set to INPUT mode
    analogWrite(A0, 50);
    // then
    assertNotEqual(HAL_PWM_Get_AnalogValue(A0), 50);
    //To Do : Add test for remaining pins if required
}

test(PWM_NoAnalogWriteWhenPinSelectedIsNotTimerChannel) {
    // when
    pinMode(D5, OUTPUT);//D5 is not a Timer channel
    analogWrite(D5, 100);
    // then
    //analogWrite works on fixed PWM frequency of 500Hz
    assertNotEqual(HAL_PWM_Get_Frequency(D5), TIM_PWM_FREQ);
    //To Do : Add test for remaining pins if required
}

test(PWM_NoAnalogWriteWhenPinSelectedIsOutOfRange) {
    // when
    pinMode(21, OUTPUT);//21 is not a user pin
    analogWrite(21, 100);
    // then
    assertNotEqual(HAL_PWM_Get_AnalogValue(21), 100);
    //To Do : Add test for remaining pins if required
}

test(PWM_AnalogWriteOnPinResultsInCorrectFrequency) {
    // when
    pinMode(A0, OUTPUT);
    analogWrite(A0, 150);
    // then
    //analogWrite works on fixed PWM frequency of 500Hz
    assertEqual(HAL_PWM_Get_Frequency(A0), TIM_PWM_FREQ);
    //To Do : Add test for remaining pins if required
}

test(PWM_AnalogWriteOnPinResultsInCorrectAnalogValue) {
    // when
    pinMode(A1, OUTPUT);
    analogWrite(A1, 200);
    // then
    assertEqual(HAL_PWM_Get_AnalogValue(A1), 200);
    //To Do : Add test for remaining pins if required
}
