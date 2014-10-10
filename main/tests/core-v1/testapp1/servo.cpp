
#include "application.h"
#include "servo_hal.h"
#include "unit-test/unit-test.h"

test(SERVO_CannotAttachWhenPinSelectedIsNotTimerChannel) {
    pin_t pin = D5;//pin under test (not a Timer channel)
    Servo testServo;
    // when
    testServo.attach(pin);
    // then
    //Servo works on fixed PWM frequency of 50Hz
    assertNotEqual(HAL_Servo_Read_Frequency(pin), SERVO_TIM_PWM_FREQ);
    //To Do : Add test for remaining pins if required
}

test(SERVO_CannotAttachWhenPinSelectedIsOutOfRange) {
    pin_t pin = 21;//pin under test (not a valid user pin)
    Servo testServo;
    // when
    testServo.attach(pin);
    // then
    //Servo works on fixed PWM frequency of 50Hz
    assertNotEqual(HAL_Servo_Read_Frequency(pin), SERVO_TIM_PWM_FREQ);
    //To Do : Add test for remaining pins if required
}

test(SERVO_AttachedOnPinResultsInCorrectFrequency) {
    pin_t pin = A0;//pin under test
    Servo testServo;
    // when
    testServo.attach(pin);
    // then
    //Servo works on fixed PWM frequency of 50Hz
    assertEqual(HAL_Servo_Read_Frequency(pin), SERVO_TIM_PWM_FREQ);
    //To Do : Add test for remaining pins if required
}

test(SERVO_WritePulseWidthOnPinResultsInCorrectMicroSeconds) {
    pin_t pin = A1;//pin under test
    uint16_t pulseWidth = 1500;//value corresponding to servo's mid-point
    Servo testServo;
    // when
    testServo.attach(pin);
    testServo.writeMicroseconds(pulseWidth);
    uint16_t readPulseWidth = testServo.readMicroseconds();
    // then
    assertEqual(readPulseWidth, pulseWidth);
    //To Do : Add test for remaining pins if required
}
