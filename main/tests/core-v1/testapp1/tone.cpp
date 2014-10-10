
#include "application.h"
#include "tone_hal.h"
#include "unit-test/unit-test.h"

test(TONE_NoGenerateWhenPinSelectedIsNotTimerChannel) {
    uint8_t pin = D5;//pin under test
    uint32_t frequency = 500;
    uint32_t duration = 100;
    // when
    tone(pin, frequency, duration);
    // then
    assertNotEqual(HAL_Tone_Is_Stopped(pin), false);
    //To Do : Add test for remaining pins if required
}

test(TONE_NoGenerateWhenPinSelectedIsOutOfRange) {
    pin_t pin = 25;//pin under test (not a valid user pin)
    uint32_t frequency = 500;
    uint32_t duration = 100;
    // when
    tone(pin, frequency, duration);
    // then
    assertNotEqual(HAL_Tone_Is_Stopped(pin), false);
    //To Do : Add test for remaining pins if required
}

test(TONE_GeneratedOnPinResultsInCorrectFrequency) {
    pin_t pin = A0;//pin under test
    uint32_t frequency = 500;
    uint32_t duration = 100;
    // when
    tone(pin, frequency, duration);
    // then
    assertEqual(HAL_Tone_Get_Frequency(pin), frequency);
    //To Do : Add test for remaining pins if required
}

test(TONE_GeneratedOnPinResultsInCorrectDuration) {
    pin_t pin = A1;//pin under test
    uint32_t frequency = 500;
    uint32_t duration = 100;
    // when
    tone(pin, frequency, duration);
    // then
    assertEqual(HAL_Tone_Is_Stopped(pin), false);
    // when
    delay(110);//delay for > 100 ms
    // then
    assertEqual(HAL_Tone_Is_Stopped(pin), true);
    //To Do : Add test for remaining pins if required
}

test(TONE_GeneratedOnPinStopsWhenStopped) {
    pin_t pin = A1;//pin under test
    uint32_t frequency = 500;
    uint32_t duration = 100;
    // when
    tone(pin, frequency, duration);
    // then
    assertEqual(HAL_Tone_Is_Stopped(pin), false);
    // when
    noTone(pin);
    // then
    assertEqual(HAL_Tone_Is_Stopped(pin), true);
    //To Do : Add test for remaining pins if required
}

