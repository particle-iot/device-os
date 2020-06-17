#include "application.h"
#include "unit-test/unit-test.h"
#include "pwm_hal.h"
#include <cmath>
#include "scope_guard.h"

#ifdef ABS
#undef ABS
#endif
#define ABS(x) ( ((x) < 0) ? -(x) : (x) )

static const uint32_t maxPulseSamples = 100;
static const uint32_t minimumFrequency = 100;

uint8_t pwm_pins[] = {
#if (PLATFORM_ID == 6) // Photon
        D0, D1, D2, D3, A4, A5, WKP, RX, TX
#elif (PLATFORM_ID == 8) // P1
        D0, D1, D2, D3, A4, A5, WKP, RX, TX, P1S0, P1S1, P1S6
#elif (PLATFORM_ID == 10) // Electron
        D0, D1, D2, D3, A4, A5, WKP, RX, TX, B0, B1, B2, B3, C4, C5
#elif (PLATFORM_ID == PLATFORM_ASOM) || (PLATFORM_ID == PLATFORM_BSOM) || (PLATFORM_ID == PLATFORM_B5SOM)
        D4, D5, D6, D7, A0, A1, A7 /* , RGBR, RGBG, RGBB */
# if (PLATFORM_ID != PLATFORM_BSOM && PLATFORM_ID != PLATFORM_B5SOM) || !HAL_PLATFORM_POWER_MANAGEMENT
        ,
        A6
# endif // PLATFORM_ID != PLATFORM_BSOM || !HAL_PLATFORM_POWER_MANAGEMENT
#elif (PLATFORM_ID == PLATFORM_TRACKER)
        D0, D1, D2, D3, D4, D5, D6, D7 /* , RGBR, RGBG, RGBB */
#elif (PLATFORM_ID == PLATFORM_ARGON) || (PLATFORM_ID == PLATFORM_BORON)
        D2, D3, D4, D5, D6, /* D7, */ D8, A0, A1, A2, A3, A4, A5 /* , RGBR, RGBG, RGBB */
#else
#error "Unsupported platform"
#endif
};

static pin_t pin = pwm_pins[0];

template <typename F> void for_all_pwm_pins(F callback)
{
    for (uint8_t i = 0; i<arraySize(pwm_pins); i++)
    {
        callback(pwm_pins[i]);
    }
}

#if (PLATFORM_ID == 8) // P1
test(PWM_00_P1S6SetupForP1) {
    // disable POWERSAVE_CLOCK on P1S6
    System.disableFeature(FEATURE_WIFI_POWERSAVE_CLOCK);

    pinMode(P1S6, OUTPUT);
    digitalWrite(P1S6, HIGH);

    // https://github.com/particle-iot/device-os/issues/1763
    // Make sure that WICED wifi stack deactivations/reactivations
    // keep the 32khz clock setting and the pin is not oscillating.
    SCOPE_GUARD({
        WiFi.disconnect();
        WiFi.off();
    });
    WiFi.on();
    WiFi.connect();
    waitFor(WiFi.ready, 30000);
    assertTrue(WiFi.ready());

    // Simple test that its state has not been changed
    assertEqual(digitalRead(P1S6), (int)HIGH);

    // Otherwise try to calculate pulse width
    uint32_t avgPulseLow = 0;
    const int iters = 3;
    for(int i = 0; i < iters; i++) {
        avgPulseLow += pulseIn(P1S6, LOW);
    }
    avgPulseLow /= iters;
    // avgPulseLow should not be around 32KHz (31.25us +- 10%)
    assertEqual(avgPulseLow, 0);
}
#endif

test(PWM_01_NoAnalogWriteWhenPinModeIsNotSetToOutput) {
    // when
    pinMode(pin, INPUT);//pin set to INPUT mode
    analogWrite(pin, 50);
    // then
    assertNotEqual(hal_pwm_get_analog_value_ext(pin), 50);
    //To Do : Add test for remaining pins if required
}

test(PWM_02_NoAnalogWriteWhenPinSelectedIsNotTimerChannel) {
#if HAL_PLATFORM_NRF52840
#if PLATFORM_ID != PLATFORM_TRACKER
    pin_t pin = D0;  //pin under test, D0 is not a Timer channel
#else
    // There are no non-PWM pins that we can safely use
    pin_t pin = PIN_INVALID;
    skip();
#endif
#else
    pin_t pin = D5;  //pin under test, D5 is not a Timer channel
#endif
    // when
    pinMode(pin, OUTPUT);
    analogWrite(pin, 100);
    // then
    //analogWrite has a default PWM frequency of 500Hz
    assertNotEqual(hal_pwm_get_frequency_ext(pin), TIM_PWM_FREQ);
    //To Do : Add test for remaining pins if required
}

test(PWM_03_NoAnalogWriteWhenPinSelectedIsOutOfRange) {
    pin_t pin = TOTAL_PINS; // pin under test (not a valid user pin)
    // when
    pinMode(pin, OUTPUT); // will simply return
    analogWrite(pin, 100); // will simply return
    // when pinAvailable is checked with a bad pin,
    // then it returns 0
    assertEqual(pinAvailable(pin), 0);
}

test(PWM_04_AnalogWriteOnPinResultsInCorrectFrequency) {
    for_all_pwm_pins([](uint16_t pin) {
	// when
    pinMode(pin, OUTPUT);

    // 8-bit resolution
    analogWriteResolution(pin, 8);
    assertEqual(analogWriteResolution(pin), 8);
    analogWrite(pin, 150);
    // then
    //analogWrite has a default PWM frequency of 500Hz
    assertEqual(hal_pwm_get_frequency_ext(pin), TIM_PWM_FREQ);

    // 4-bit resolution
    analogWriteResolution(pin, 4);
    assertEqual(analogWriteResolution(pin), 4);
    analogWrite(pin, 9);
    // then
    assertEqual(hal_pwm_get_frequency_ext(pin), TIM_PWM_FREQ);

    // 12-bit resolution
    analogWriteResolution(pin, 12);
    assertEqual(analogWriteResolution(pin), 12);
    analogWrite(pin, 1900);
    // then
    assertEqual(hal_pwm_get_frequency_ext(pin), TIM_PWM_FREQ);

    // 15-bit resolution
    analogWriteResolution(pin, 15);
    assertEqual(analogWriteResolution(pin), 15);
    analogWrite(pin, 15900);
    // then
    assertEqual(hal_pwm_get_frequency_ext(pin), TIM_PWM_FREQ);

    pinMode(pin, INPUT);
    });
}

test(PWM_05_AnalogWriteOnPinResultsInCorrectAnalogValue) {
	for_all_pwm_pins([](uint16_t pin) {
	// when
	pinMode(pin, OUTPUT);

    // 8-bit resolution
    analogWriteResolution(pin, 8);
    assertEqual(analogWriteResolution(pin), 8);
	analogWrite(pin, 200);
	// then
	assertEqual(hal_pwm_get_analog_value_ext(pin), 200);

    // 4-bit resolution
    analogWriteResolution(pin, 4);
    assertEqual(analogWriteResolution(pin), 4);
    analogWrite(pin, 9);
    // then
    assertEqual(hal_pwm_get_analog_value_ext(pin), 9);

    // 12-bit resolution
    analogWriteResolution(pin, 12);
    assertEqual(analogWriteResolution(pin), 12);
    analogWrite(pin, 1900);
    // then
    assertEqual(hal_pwm_get_analog_value_ext(pin), 1900);

    // 15-bit resolution
    analogWriteResolution(pin, 15);
    assertEqual(analogWriteResolution(pin), 15);
    analogWrite(pin, 15900);
    // then
    assertEqual(hal_pwm_get_analog_value_ext(pin), 15900);

	pinMode(pin, INPUT);
	});
}

test(PWM_06_AnalogWriteWithFrequencyOnPinResultsInCorrectFrequency) {
	for_all_pwm_pins([](uint16_t pin) {
	// when
    pinMode(pin, OUTPUT);

    // 8-bit resolution
    analogWriteResolution(pin, 8);
    assertEqual(analogWriteResolution(pin), 8);
    analogWrite(pin, 150, analogWriteMaxFrequency(pin) / 2);
    // then
    // 1 digit error is acceptible due to rounding at higher frequencies
    assertLessOrEqual((int32_t)hal_pwm_get_frequency_ext(pin) - analogWriteMaxFrequency(pin) / 2, 1);

    // 4-bit resolution
    analogWriteResolution(pin, 4);
    assertEqual(analogWriteResolution(pin), 4);
    analogWrite(pin, 9, analogWriteMaxFrequency(pin) / 2);
    // then
    // 1 digit error is acceptible due to rounding at higher frequencies
    assertLessOrEqual((int32_t)hal_pwm_get_frequency_ext(pin) - analogWriteMaxFrequency(pin) / 2, 1);

    // 12-bit resolution
    analogWriteResolution(pin, 12);
    assertEqual(analogWriteResolution(pin), 12);
    analogWrite(pin, 1900, analogWriteMaxFrequency(pin) / 2);
    // then
    // 1 digit error is acceptible due to rounding at higher frequencies
    assertLessOrEqual((int32_t)hal_pwm_get_frequency_ext(pin) - analogWriteMaxFrequency(pin) / 2, 1);

    // 15-bit resolution
    analogWriteResolution(pin, 15);
    assertEqual(analogWriteResolution(pin), 15);
    analogWrite(pin, 15900, analogWriteMaxFrequency(pin) / 2);
    // then
    // 1 digit error is acceptible due to rounding at higher frequencies
    assertLessOrEqual((int32_t)hal_pwm_get_frequency_ext(pin) - analogWriteMaxFrequency(pin) / 2, 1);

	pinMode(pin, INPUT);
	});
}

test(PWM_07_AnalogWriteWithFrequencyOnPinResultsInCorrectAnalogValue) {
	for_all_pwm_pins([](uint16_t pin) {
	// when
    pinMode(pin, OUTPUT);

    // 8-bit resolution
    analogWriteResolution(pin, 8);
    assertEqual(analogWriteResolution(pin), 8);
    analogWrite(pin, 200, analogWriteMaxFrequency(pin) / 2);
    // then
    assertEqual(hal_pwm_get_analog_value_ext(pin), 200);

    // 4-bit resolution
    analogWriteResolution(pin, 4);
    assertEqual(analogWriteResolution(pin), 4);
    analogWrite(pin, 9, analogWriteMaxFrequency(pin) / 2);
    // then
    assertEqual(hal_pwm_get_analog_value_ext(pin), 9);

    // 12-bit resolution
    analogWriteResolution(pin, 12);
    assertEqual(analogWriteResolution(pin), 12);
    analogWrite(pin, 1900, analogWriteMaxFrequency(pin) / 2);
    // then
    assertEqual(hal_pwm_get_analog_value_ext(pin), 1900);

    // 15-bit resolution
    analogWriteResolution(pin, 15);
    assertEqual(analogWriteResolution(pin), 15);
    analogWrite(pin, 15900, analogWriteMaxFrequency(pin) / 2);
    // then
    assertEqual(hal_pwm_get_analog_value_ext(pin), 15900);

    pinMode(pin, INPUT);
	});
}

test(PWM_08_LowDCAnalogWriteOnPinResultsInCorrectPulseWidth) {
	for_all_pwm_pins([](uint16_t pin) {

    // when
    pinMode(pin, OUTPUT);

    // 8-bit resolution
    analogWriteResolution(pin, 8);
    assertEqual(analogWriteResolution(pin), 8);
    // analogWrite(pin, 25); // 9.8% Duty Cycle at 500Hz = 196us HIGH, 1804us LOW.
    // if (pin == D0) delay(5000);
#if HAL_PLATFORM_NRF52840
    // Dummy read to wait until the change of PWM takes effect
    pulseIn(pin, HIGH);
    pulseIn(pin, LOW);
#endif
    uint32_t avgPulseHigh = 0;
    for(int i=0; i<10; i++) {
        analogWrite(pin, 25); // 9.8% Duty Cycle at 500Hz = 196us HIGH, 1804us LOW.
        avgPulseHigh += pulseIn(pin, HIGH);
    }
    avgPulseHigh /= 10;
    analogWrite(pin, 0);

    // then
    // avgPulseHigh should equal 200 +/- 50
    assertMoreOrEqual(avgPulseHigh, 150);
    assertLessOrEqual(avgPulseHigh, 250);

    // 4-bit resolution
    analogWriteResolution(pin, 4);
    assertEqual(analogWriteResolution(pin), 4);
    // analogWrite(pin, 2); // 13.3% Duty Cycle at 500Hz = 266us HIGH, 1733us LOW.
    // if (pin == D0) delay(5000);
#if HAL_PLATFORM_NRF52840
    // Dummy read to wait until the change of PWM takes effect
    pulseIn(pin, HIGH);
    pulseIn(pin, LOW);
#endif
    avgPulseHigh = 0;
    for(int i=0; i<10; i++) {
        analogWrite(pin, 2); // 13.3% Duty Cycle at 500Hz = 266us HIGH, 1733us LOW.
        avgPulseHigh += pulseIn(pin, HIGH);
    }
    avgPulseHigh /= 10;
    analogWrite(pin, 0);

    // then
    // avgPulseHigh should equal 266 +/- 50
    assertMoreOrEqual(avgPulseHigh, 216);
    assertLessOrEqual(avgPulseHigh, 316);

    // 12-bit resolution
    analogWriteResolution(pin, 12);
    assertEqual(analogWriteResolution(pin), 12);
    // analogWrite(pin, 409); // 10% Duty Cycle at 500Hz = 200us HIGH, 1800us LOW.
    // if (pin == D0) delay(5000);
#if HAL_PLATFORM_NRF52840
    // Dummy read to wait until the change of PWM takes effect
    pulseIn(pin, HIGH);
    pulseIn(pin, LOW);
#endif
    avgPulseHigh = 0;
    for(int i=0; i<10; i++) {
        analogWrite(pin, 409); // 10% Duty Cycle at 500Hz = 200us HIGH, 1800us LOW.
        avgPulseHigh += pulseIn(pin, HIGH);
    }
    avgPulseHigh /= 10;
    analogWrite(pin, 0);

    // then
    // avgPulseHigh should equal 200 +/- 50
    assertMoreOrEqual(avgPulseHigh, 150);
    assertLessOrEqual(avgPulseHigh, 250);

    // 15-bit resolution
    analogWriteResolution(pin, 15);
    assertEqual(analogWriteResolution(pin), 15);
    // analogWrite(pin, 3277); // 10% Duty Cycle at 500Hz = 200us HIGH, 1800us LOW.
    // if (pin == D0) delay(5000);
#if HAL_PLATFORM_NRF52840
    // Dummy read to wait until the change of PWM takes effect
    pulseIn(pin, HIGH);
    pulseIn(pin, LOW);
#endif
    avgPulseHigh = 0;
    for(int i=0; i<10; i++) {
        analogWrite(pin, 3277); // 10% Duty Cycle at 500Hz = 200us HIGH, 1800us LOW.
        avgPulseHigh += pulseIn(pin, HIGH);
    }
    avgPulseHigh /= 10;
    analogWrite(pin, 0);

    // then
    // avgPulseHigh should equal 200 +/- 50
    assertMoreOrEqual(avgPulseHigh, 150);
    assertLessOrEqual(avgPulseHigh, 250);

    pinMode(pin, INPUT);
	});
}

test(PWM_09_LowFrequencyAnalogWriteOnPinResultsInCorrectPulseWidth) {
    for_all_pwm_pins([](uint16_t pin) {
    // when
    pinMode(pin, OUTPUT);

    // 8-bit resolution
    analogWriteResolution(pin, 8);
    assertEqual(analogWriteResolution(pin), 8);
    // analogWrite(pin, 25, 10); // 9.8% Duty Cycle at 10Hz = 9800us HIGH, 90200us LOW.
    // if (pin == D0) delay(5000);
#if HAL_PLATFORM_NRF52840
    // Dummy read to wait until the change of PWM takes effect
    pulseIn(pin, HIGH);
    pulseIn(pin, LOW);
#endif
    uint32_t avgPulseHigh = 0;
    for(int i=0; i<2; i++) {
        analogWrite(pin, 25, 10); // 9.8% Duty Cycle at 10Hz = 9800us HIGH, 90200us LOW.
        avgPulseHigh += pulseIn(pin, HIGH);
    }
    avgPulseHigh /= 2;
    // then
    // avgPulseHigh should equal 9800 +/- 100
    assertMoreOrEqual(avgPulseHigh, 9700);
    assertLessOrEqual(avgPulseHigh, 9900);

    // 4-bit resolution
    analogWriteResolution(pin, 4);
    assertEqual(analogWriteResolution(pin), 4);
    // analogWrite(pin, 2, 10); // 13.3% Duty Cycle at 10Hz = 13333us HIGH, 86000us LOW.
    // if (pin == D0) delay(5000);
#if HAL_PLATFORM_NRF52840
    // Dummy read to wait until the change of PWM takes effect
    pulseIn(pin, HIGH);
    pulseIn(pin, LOW);
#endif
    avgPulseHigh = 0;
    for(int i=0; i<2; i++) {
        analogWrite(pin, 2, 10); // 13.3% Duty Cycle at 10Hz = 13333us HIGH, 90000us LOW.
        avgPulseHigh += pulseIn(pin, HIGH);
    }
    avgPulseHigh /= 2;
    // then
    // avgPulseHigh should equal 13333 +/- 1000
    assertMoreOrEqual(avgPulseHigh, (13333 - 1000) );
    assertLessOrEqual(avgPulseHigh, (13333 + 1000) );

    // 12-bit resolution
    analogWriteResolution(pin, 12);
    assertEqual(analogWriteResolution(pin), 12);
    // analogWrite(pin, 409, 10); // 10% Duty Cycle at 10Hz = 10000us HIGH, 90000us LOW.
    // if (pin == D0) delay(5000);
#if HAL_PLATFORM_NRF52840
    // Dummy read to wait until the change of PWM takes effect
    pulseIn(pin, HIGH);
    pulseIn(pin, LOW);
#endif
    avgPulseHigh = 0;
    for(int i=0; i<2; i++) {
        analogWrite(pin, 409, 10); // 10% Duty Cycle at 10Hz = 10000us HIGH, 90000us LOW.
        avgPulseHigh += pulseIn(pin, HIGH);
    }
    avgPulseHigh /= 2;
    // then
    // avgPulseHigh should equal 10000 +/- 100
    assertMoreOrEqual(avgPulseHigh,  9900);
    assertLessOrEqual(avgPulseHigh, 10100);

    // 15-bit resolution
    analogWriteResolution(pin, 15);
    assertEqual(analogWriteResolution(pin), 15);
    // analogWrite(pin, 3277, 10); // 10% Duty Cycle at 10Hz = 10000us HIGH, 90000us LOW.
    // if (pin == D0) delay(5000);
#if HAL_PLATFORM_NRF52840
    // Dummy read to wait until the change of PWM takes effect
    pulseIn(pin, HIGH);
    pulseIn(pin, LOW);
#endif
    avgPulseHigh = 0;
    for(int i=0; i<2; i++) {
        analogWrite(pin, 3277, 10); // 10% Duty Cycle at 10Hz = 10000us HIGH, 90000us LOW.
        avgPulseHigh += pulseIn(pin, HIGH);
    }
    avgPulseHigh /= 2;
    // then
    // avgPulseHigh should equal 10000 +/- 100
    assertMoreOrEqual(avgPulseHigh,  9900);
    assertLessOrEqual(avgPulseHigh, 10100);

    analogWrite(pin, 0, 10);
    pinMode(pin, INPUT);
    });
}

test(PWM_10_HighFrequencyAnalogWriteOnPinResultsInCorrectPulseWidth) {
	for_all_pwm_pins([](uint16_t pin) {

	// when
    pinMode(pin, OUTPUT);

    // 8-bit resolution
    analogWriteResolution(pin, 8);
    assertEqual(analogWriteResolution(pin), 8);
    // analogWrite(pin, 25, 10000); // 9.8% Duty Cycle at 10kHz = 9.8us HIGH, 90.2us LOW.
    // if (pin == D0) delay(5000);
#if HAL_PLATFORM_NRF52840
    // Dummy read to wait until the change of PWM takes effect
    pulseIn(pin, HIGH);
    pulseIn(pin, LOW);
#endif
    uint32_t avgPulseHigh = 0;
    for(int i=0; i<10; i++) {
        analogWrite(pin, 25, 10000); // 9.8% Duty Cycle at 10kHz = 9.8us HIGH, 90.2us LOW.
        avgPulseHigh += pulseIn(pin, HIGH);
    }
    avgPulseHigh /= 10;
    // then
    // avgPulseHigh should equal 10 +/- 5
    assertMoreOrEqual(avgPulseHigh, 5);
    assertLessOrEqual(avgPulseHigh, 15);

    // 4-bit resolution
    analogWriteResolution(pin, 4);
    assertEqual(analogWriteResolution(pin), 4);
    // analogWrite(pin, 2, 10000); // 13.3% Duty Cycle at 10kHz = 13.3us HIGH, 86.6us LOW.
    // if (pin == D0) delay(5000);
#if HAL_PLATFORM_NRF52840
    // Dummy read to wait until the change of PWM takes effect
    pulseIn(pin, HIGH);
    pulseIn(pin, LOW);
#endif
    avgPulseHigh = 0;
    for(int i=0; i<10; i++) {
        analogWrite(pin, 2, 10000); // 13.3% Duty Cycle at 10kHz = 13.3us HIGH, 86.6us LOW.
        avgPulseHigh += pulseIn(pin, HIGH);
    }
    avgPulseHigh /= 10;
    // then
    // avgPulseHigh should equal 13 +/- 5
    assertMoreOrEqual(avgPulseHigh, 8);
    assertLessOrEqual(avgPulseHigh, 18);

    // 12-bit resolution
    analogWriteResolution(pin, 12);
    assertEqual(analogWriteResolution(pin), 12);
    // analogWrite(pin, 409, 10000); // 10% Duty Cycle at 10kHz = 10us HIGH, 90us LOW.
    // if (pin == D0) delay(5000);
#if HAL_PLATFORM_NRF52840
    // Dummy read to wait until the change of PWM takes effect
    pulseIn(pin, HIGH);
    pulseIn(pin, LOW);
#endif
    avgPulseHigh = 0;
    for(int i=0; i<10; i++) {
        analogWrite(pin, 409, 10000); // 10% Duty Cycle at 10kHz = 10us HIGH, 90us LOW.
        avgPulseHigh += pulseIn(pin, HIGH);
    }
    avgPulseHigh /= 10;
    // then
    // avgPulseHigh should equal 10 +/- 2
    assertMoreOrEqual(avgPulseHigh, 8);
    assertLessOrEqual(avgPulseHigh, 12);

    // 15-bit resolution
    analogWriteResolution(pin, 15);
    assertEqual(analogWriteResolution(pin), 15);
    // analogWrite(pin, 3277, 10000); // 10% Duty Cycle at 10kHz = 10us HIGH, 90us LOW.
    // if (pin == D0) delay(5000);
#if HAL_PLATFORM_NRF52840
    // Dummy read to wait until the change of PWM takes effect
    pulseIn(pin, HIGH);
    pulseIn(pin, LOW);
#endif
    avgPulseHigh = 0;
    for(int i=0; i<10; i++) {
        analogWrite(pin, 3277, 10000); // 10% Duty Cycle at 10kHz = 10us HIGH, 90us LOW.
        avgPulseHigh += pulseIn(pin, HIGH);
    }
    avgPulseHigh /= 10;
    // then
    // avgPulseHigh should equal 10 +/- 2
    assertMoreOrEqual(avgPulseHigh, 8);
    assertLessOrEqual(avgPulseHigh, 12);

    analogWrite(pin, 0, 500);
    pinMode(pin, INPUT);
	});
}

test(PWM_11_CompherensiveResolutionFrequency) {
    for_all_pwm_pins([&](uint16_t pin) {
        // when
        pinMode(pin, OUTPUT);

        // 2 or 3 bit resolution PWM is crude at best and hard to be accurate, we won't test it here.
        uint8_t resolution = 4;

        for (resolution = 4; ; resolution++) {
            // Set resolution
            analogWriteResolution(pin, resolution);
            // Serial.printlnf("pin=%d res=%d res_act=%d", pin, resolution, analogWriteResolution(pin));
            if (resolution <= 15) {
                // All PWM pins should support resolution of up to 15 bits
                assertEqual(resolution, analogWriteResolution(pin));
            } else {
                // Some PWM pins (which utilize 32-bit timers) may support higher resolution
                if (resolution != analogWriteResolution(pin)) {
                    break;
                } else {
                    assertEqual(resolution, analogWriteResolution(pin));
                }
            }

            uint32_t maxVal = (1 << resolution) - 1;

            uint32_t freq = 1;
            uint32_t freqStep = 9;

            // Test all frequencies up to analogWriteMaxFrequency() with logarithmic step
            for (freq = minimumFrequency; freq <= analogWriteMaxFrequency(pin);) {
                float fp = floor(log10((float)freq));
                if (fp < 1) {
                    fp = 1;
                }
                freqStep = ((uint32_t)pow(10.0, fp + 1.0)) / 3;
                if (freqStep < 33) {
                    freqStep = 33;
                }

                for (uint32_t duty = 20; duty <= 90; duty += 5) {
                    uint32_t value = (uint32_t)(((double)duty / 100.0) * (double)maxVal);
                    analogWrite(pin, value, freq);
                    // Check if the write resulted in correct analog value
                    assertEqual(hal_pwm_get_analog_value_ext(pin), value);

                    double refPulseWidthUs = ((double)duty/100.0) * (1000000.0 / (double)freq);
                    // pulseIn cannot measure pulses shorter than 10us reliably, limit to 20us
                    if (refPulseWidthUs < 30.0) {
                        // Skip
                    } else if ((1.0 - ((double)duty/100.0)) * refPulseWidthUs < 30.0) {
                        // pulseIn will not be able to accurately measure high pulse that is followed by <20us low pulse
                    } else {
                        uint32_t pulseAcc = 0;
                        uint32_t pulseSamples = freq < 1000 ? maxPulseSamples / 10 : maxPulseSamples;
#if HAL_PLATFORM_NRF52840
                        // Dummy read to wait until the change of PWM takes effect
                        pulseIn(pin, HIGH);
                        pulseIn(pin, LOW);
#endif
                        for (uint32_t i = 0; i < pulseSamples; i++) {
                            ATOMIC_BLOCK() {
                                pulseAcc += pulseIn(pin, HIGH);
                            }
                            // 0 and maxVal should result in a timeout
                            if (value == 0 || value == maxVal) {
                                Serial.printlnf("timeout: %lu", i);
                                assertEqual(pulseAcc, 0);
                                assertEqual(digitalRead(pin), 0);
                                break;
                            }
                        }
                        double avgPulse = (double)pulseAcc / pulseSamples;
                        double err = ABS(avgPulse - refPulseWidthUs);

                        // Pulse width should be within 15% error margin resolution > 6, else 40%
                        float errPulseWidth = (resolution > 6) ? 0.15 * refPulseWidthUs : 0.40 * refPulseWidthUs;
                        // Serial.printlnf("pin=%d freq=%d duty=%d res=%d res_act=%d val=%d ref=%f avg=%f err=%f err_max=%f pcnt=%0.2f", pin, freq, duty, resolution, analogWriteResolution(pin), value, refPulseWidthUs, avgPulse, err, errPulseWidth, 100*(ABS(avgPulse-refPulseWidthUs)/refPulseWidthUs) );
                        assertLessOrEqual(err, errPulseWidth);
                    }
                }

                // Clamp to analogWriteMaxFrequency(pin)
                if (freq < analogWriteMaxFrequency(pin) && (freq + freqStep) > analogWriteMaxFrequency(pin)) {
                    freq = analogWriteMaxFrequency(pin);
                } else if (freq < analogWriteMaxFrequency(pin)) {
                    freq += freqStep;
                } else {
                    break;
                }
            }
        }

        // At least 15-bit maximum resolution
        assertMoreOrEqual(resolution, 15);
    });
}

#if (PLATFORM_ID == 8) // P1
test(PWM_99_P1S6CleanupForP1) {
    // enable POWERSAVE_CLOCK on P1S6
    System.enableFeature(FEATURE_WIFI_POWERSAVE_CLOCK);
}
#endif
