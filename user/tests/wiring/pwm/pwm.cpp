/*
 * Copyright (c) 2022 Particle Industries, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

/*
 * pwm Test requires P2 PWM pins to all be jumpered together:
 * D1, A2, A5, S0, S1
 * or adjust `pwm_pins` below to only test specific pairs/combinations of pins if 
 * pin capacitance becomes an issue.
 */

#include "application.h"
#include "unit-test/unit-test.h"
#include <cmath>
#include "scope_guard.h"

#ifdef ABS
#undef ABS
#endif
#define ABS(x) ( ((x) < 0) ? -(x) : (x) )

static const uint32_t maxPulseSamples = 25;
static const uint32_t minimumFrequency = 100;

struct PinMapping {
    const char* name;
    hal_pin_t pin;
};

#define PIN(p) {#p, p}

const PinMapping pwm_pins[] = {
#if PLATFORM_ID == PLATFORM_P2 || PLATFORM_ID == PLATFORM_TRACKERM
    PIN(D1), PIN(A2), PIN(A5), PIN(S0), PIN(S1) /* ,PIN(RGBR), PIN(RGBG), PIN(RGBB) */
#else
#error "Unsupported platform"
#endif
};

static hal_pin_t pwm_output_pin;

static uint32_t get_average_pulse_width(hal_pin_t pin) {
    uint32_t avgPulseHigh = 0;
    for(int i=0; i<10; i++) {
        avgPulseHigh += pulseIn(pin, HIGH);
    }
    avgPulseHigh /= 10;
    return avgPulseHigh;
}

template <typename F> void for_all_pwm_pins(F callback, hal_pin_t skipPin = PIN_INVALID)
{
    RGB.control(true);        
    for (uint8_t i = 0; i<arraySize(pwm_pins); i++)
    {
        if (skipPin != PIN_INVALID && pwm_pins[i].pin == skipPin) {
            continue;
        }
        callback(pwm_pins[i].pin, pwm_pins[i].name);
    }
    RGB.control(false);
}

test(PWM_01_LowDCAnalogWriteOnPinResultsInCorrectPulseWidth) {
    for_all_pwm_pins([](hal_pin_t pin, const char* name) {
        pwm_output_pin = pin;
        pinMode(pwm_output_pin, OUTPUT);
        //out->printlnf("Output pin: %s", name);

        for_all_pwm_pins([](hal_pin_t pin, const char* name) {

            //out->printlnf("Input pin: %s", name);
            pinMode(pin, INPUT);
            uint32_t avgPulseHigh = 0;

            // 8-bit resolution
            analogWriteResolution(pwm_output_pin, 8);
            assertEqual(analogWriteResolution(pwm_output_pin), 8);
            // 9.8% Duty Cycle at 500Hz = 196us HIGH, 1804us LOW.
            analogWrite(pwm_output_pin, 25);
            avgPulseHigh = get_average_pulse_width(pin);
            // avgPulseHigh should equal 200 +/- 50
            assertMoreOrEqual(avgPulseHigh, 150);
            assertLessOrEqual(avgPulseHigh, 250);

            // 4-bit resolution
            analogWriteResolution(pwm_output_pin, 4);
            assertEqual(analogWriteResolution(pwm_output_pin), 4);
            // 13.3% Duty Cycle at 500Hz = 266us HIGH, 1733us LOW.
            analogWrite(pwm_output_pin, 2);
            avgPulseHigh = get_average_pulse_width(pin);
            // avgPulseHigh should equal 266 +/- 50
            assertMoreOrEqual(avgPulseHigh, 216);
            assertLessOrEqual(avgPulseHigh, 316);

            // 12-bit resolution
            analogWriteResolution(pwm_output_pin, 12);
            assertEqual(analogWriteResolution(pwm_output_pin), 12);
            // 10% Duty Cycle at 500Hz = 200us HIGH, 1800us LOW.
            analogWrite(pwm_output_pin, 409);
            avgPulseHigh = get_average_pulse_width(pin);
            // avgPulseHigh should equal 200 +/- 50
            assertMoreOrEqual(avgPulseHigh, 150);
            assertLessOrEqual(avgPulseHigh, 250);

            // 15-bit resolution
            analogWriteResolution(pwm_output_pin, 15);
            assertEqual(analogWriteResolution(pwm_output_pin), 15);
            // 10% Duty Cycle at 500Hz = 200us HIGH, 1800us LOW.
            analogWrite(pwm_output_pin, 3277);
            avgPulseHigh = get_average_pulse_width(pin);
            // avgPulseHigh should equal 200 +/- 50
            assertMoreOrEqual(avgPulseHigh, 150);
            assertLessOrEqual(avgPulseHigh, 250);
        }, pwm_output_pin);
        pinMode(pwm_output_pin, INPUT);
    });
}

// 10hz
test(PWM_02_LowFrequencyAnalogWriteOnPinResultsInCorrectPulseWidth) {
    static const uint32_t TEST_FREQUENCY_HZ = 10;

    for_all_pwm_pins([](hal_pin_t pin, const char* name) {
        pwm_output_pin = pin;
        pinMode(pwm_output_pin, OUTPUT);
        // out->printlnf("Output pin: %s", name);

        for_all_pwm_pins([](hal_pin_t pin, const char* name) {
            // out->printlnf("Input pin: %s", name);
            pinMode(pin, INPUT);
            uint32_t avgPulseHigh = 0;

            // 8-bit resolution
            analogWriteResolution(pwm_output_pin, 8);
            assertEqual(analogWriteResolution(pwm_output_pin), 8);
            // 9.8% Duty Cycle at 10Hz = 9800us HIGH, 90200us LOW.
            analogWrite(pwm_output_pin, 25, TEST_FREQUENCY_HZ);
            avgPulseHigh = get_average_pulse_width(pin);
            // avgPulseHigh should equal 9800 +/- 100
            assertMoreOrEqual(avgPulseHigh, 9700);
            assertLessOrEqual(avgPulseHigh, 9900);

            // 4-bit resolution
            analogWriteResolution(pwm_output_pin, 4);
            assertEqual(analogWriteResolution(pwm_output_pin), 4);
            // 13.3% Duty Cycle at 10Hz = 13333us HIGH, 90000us LOW.
            analogWrite(pwm_output_pin, 2, TEST_FREQUENCY_HZ);
            avgPulseHigh = get_average_pulse_width(pin);
            // avgPulseHigh should equal 13333 +/- 1000
            assertMoreOrEqual(avgPulseHigh, (13333 - 1000));
            assertLessOrEqual(avgPulseHigh, (13333 + 1000));

            // 12-bit resolution
            analogWriteResolution(pwm_output_pin, 12);
            assertEqual(analogWriteResolution(pwm_output_pin), 12);
            // 10% Duty Cycle at 10Hz = 10000us HIGH, 90000us LOW.
            analogWrite(pwm_output_pin, 409, TEST_FREQUENCY_HZ);
            avgPulseHigh = get_average_pulse_width(pin);
            // avgPulseHigh should equal 10000 +/- 100
            assertMoreOrEqual(avgPulseHigh, 9900);
            assertLessOrEqual(avgPulseHigh, 10100);

            // 15-bit resolution
            analogWriteResolution(pwm_output_pin, 15);
            assertEqual(analogWriteResolution(pwm_output_pin), 15);
            // 10% Duty Cycle at 10Hz = 10000us HIGH, 90000us LOW.
            analogWrite(pwm_output_pin, 3277, TEST_FREQUENCY_HZ);
            avgPulseHigh = get_average_pulse_width(pin);
            // avgPulseHigh should equal 200 +/- 50
            assertMoreOrEqual(avgPulseHigh, 9900);
            assertLessOrEqual(avgPulseHigh, 10100);
        }, pwm_output_pin);
        pinMode(pwm_output_pin, INPUT);
    });
}

// 10khz
test(PWM_03_HighFrequencyAnalogWriteOnPinResultsInCorrectPulseWidth) {
    static const uint32_t TEST_FREQUENCY_HZ = 10000;

    for_all_pwm_pins([](hal_pin_t pin, const char* name) {
        pwm_output_pin = pin;
        pinMode(pwm_output_pin, OUTPUT);
        // out->printlnf("Output pin: %s", name);

        for_all_pwm_pins([](hal_pin_t pin, const char* name) {
            // out->printlnf("Input pin: %s", name);
            pinMode(pin, INPUT);
            uint32_t avgPulseHigh = 0;

            // 8-bit resolution
            analogWriteResolution(pwm_output_pin, 8);
            assertEqual(analogWriteResolution(pwm_output_pin), 8);
            // 9.8% Duty Cycle at 10kHz = 9.8us HIGH, 90.2us LOW.
            analogWrite(pwm_output_pin, 25, TEST_FREQUENCY_HZ);
            avgPulseHigh = get_average_pulse_width(pin);
            // avgPulseHigh should equal 10 +/- 5
            assertMoreOrEqual(avgPulseHigh, 5);
            assertLessOrEqual(avgPulseHigh, 15);

            // 4-bit resolution
            analogWriteResolution(pwm_output_pin, 4);
            assertEqual(analogWriteResolution(pwm_output_pin), 4);
            // 13.3% Duty Cycle at 10kHz = 13.3us HIGH, 86.6us LOW.
            analogWrite(pwm_output_pin, 2, TEST_FREQUENCY_HZ);
            avgPulseHigh = get_average_pulse_width(pin);
            // avgPulseHigh should equal 13 +/- 5
            assertMoreOrEqual(avgPulseHigh, 8);
            assertLessOrEqual(avgPulseHigh, 18);

            // 12-bit resolution
            analogWriteResolution(pwm_output_pin, 12);
            assertEqual(analogWriteResolution(pwm_output_pin), 12);
            // 10% Duty Cycle at 10kHz = 10us HIGH, 90us LOW.
            analogWrite(pwm_output_pin, 409, TEST_FREQUENCY_HZ);
            avgPulseHigh = get_average_pulse_width(pin);
            // avgPulseHigh should equal 10 +/- 2
            assertMoreOrEqual(avgPulseHigh, 8);
            assertLessOrEqual(avgPulseHigh, 12);

            // 15-bit resolution
            analogWriteResolution(pwm_output_pin, 15);
            assertEqual(analogWriteResolution(pwm_output_pin), 15);
            // 10% Duty Cycle at 10kHz = 10us HIGH, 90us LOW.
            analogWrite(pwm_output_pin, 3277, TEST_FREQUENCY_HZ);
            avgPulseHigh = get_average_pulse_width(pin);
            // avgPulseHigh should equal 10 +/- 2
            assertMoreOrEqual(avgPulseHigh, 8);
            assertLessOrEqual(avgPulseHigh, 12);
        }, pwm_output_pin);
        pinMode(pwm_output_pin, INPUT);
    });
}

test(PWM_04_PwmSleep) {
    for_all_pwm_pins([](hal_pin_t pin, const char* name) {
        pwm_output_pin = pin;
        pinMode(pwm_output_pin, OUTPUT);
        // out->printlnf("Output pin: %s", name);

        // 9.8% Duty Cycle at 10kHz = 9.8us HIGH, 90.2us LOW.
        analogWriteResolution(pwm_output_pin, 8);
        assertEqual(analogWriteResolution(pwm_output_pin), 8);
        analogWrite(pwm_output_pin, 25, 10000); 

        for_all_pwm_pins([](hal_pin_t pin, const char* name) {
            // out->printlnf("Input Pin: %s", name);
            pinMode(pin, INPUT);

            uint32_t avgPulseHigh = 0;
            for(int i=0; i<10; i++) {
                // Disable SysTick to avoid potential interrupt safety issues with RGB LED pins
                SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
                assertEqual(0, hal_pwm_sleep(true, nullptr));

                int pinState = digitalRead(pin);
                assertEqual(pinState, 0);
                assertEqual(0, hal_pwm_sleep(false, nullptr));
                
                SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
                avgPulseHigh += pulseIn(pin, HIGH);
            }
            avgPulseHigh /= 10;
            // avgPulseHigh should equal 10 +/- 5
            assertMoreOrEqual(avgPulseHigh, 5);
            assertLessOrEqual(avgPulseHigh, 15);
        }, pwm_output_pin);
        pinMode(pwm_output_pin, INPUT);
    });
}

test(PWM_05_CompherensiveResolutionFrequency) {
    // Walk from 4 to 15 bits resolution, increasing frequency logarithmically from lowest to highest
    // Verify input pulse widths

    for_all_pwm_pins([](hal_pin_t pin, const char* name) {
        pwm_output_pin = pin;
        pinMode(pwm_output_pin, OUTPUT);
        // out->printlnf("Output pin: %s", name);
        system_tick_t lastProcess = millis();

        for_all_pwm_pins([](hal_pin_t pin, const char* name) {
            // out->printlnf("Input pin: %s", name);
            pinMode(pin, INPUT);
            uint8_t resolution = 4;

            for (resolution = 4; ; resolution++) {
                // Set resolution
                analogWriteResolution(pwm_output_pin, resolution);
                if (resolution <= 15) {
                    // All PWM pins should support resolution of up to 15 bits
                    assertEqual(resolution, analogWriteResolution(pwm_output_pin));
                } else {
                    // Some PWM pins (which utilize 32-bit timers) may support higher resolution
                    if (resolution != analogWriteResolution(pwm_output_pin)) {
                        break;
                    } else {
                        assertEqual(resolution, analogWriteResolution(pwm_output_pin));
                    }
                }
                // Serial.printlnf("pin=%d res=%d res_act=%d", pwm_output_pin, resolution, analogWriteResolution(pwm_output_pin));

                uint32_t maxVal = (1 << resolution) - 1;

                uint32_t freq = 1;
                uint32_t freqStep = 9;

                // Test all frequencies up to analogWriteMaxFrequency() with logarithmic step
                for (freq = minimumFrequency; freq <= analogWriteMaxFrequency(pwm_output_pin);) {
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
                        analogWrite(pwm_output_pin, value, freq);
                        // Check if the write resulted in correct analog value
                        assertEqual(hal_pwm_get_analog_value_ext(pwm_output_pin), value);

                        double refPulseWidthUs = ((double)duty/100.0) * (1000000.0 / (double)freq);
                        // pulseIn cannot measure pulses shorter than 10us reliably, limit to 20us
                        if (refPulseWidthUs < 30.0) {
                            // Skip
                        } else if ((1.0 - ((double)duty/100.0)) * refPulseWidthUs < 30.0) {
                            // pulseIn will not be able to accurately measure high pulse that is followed by <20us low pulse
                        } else {
                            uint32_t pulseAcc = 0;
                            uint32_t pulseSamples = freq < 1000 ? maxPulseSamples / 10 : maxPulseSamples;
                            for (uint32_t i = 0; i < pulseSamples; i++) {
    #if HAL_PLATFORM_NRF52840
                                // Dummy read to wait until the change of PWM takes effect
                                pulseIn(pin, HIGH);
                                pulseIn(pin, LOW);
    #endif
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
    #ifdef PARTICLE_TEST_RUNNER
                                // Relax a bit just in case
                                if (millis() - lastProcess >= 5000) {
                                    for (int i = 0; i < 10; i++) {
                                        Particle.process();
                                        delay(10);
                                    }
                                    lastProcess = millis();
                                }
    #endif // PARTICLE_TEST_RUNNER
                            }
                            double avgPulse = (double)pulseAcc / pulseSamples;
                            double err = ABS(avgPulse - refPulseWidthUs);

                            // Pulse width should be within 15% error margin resolution > 6, else 40%
                            float errPulseWidth = (resolution > 6) ? 0.15 * refPulseWidthUs : 0.40 * refPulseWidthUs;
                            // Serial.printlnf("pin=%d freq=%d duty=%d res=%d res_act=%d val=%d ref=%f avg=%f err=%f err_max=%f pcnt=%0.2f", pwm_output_pin, freq, duty, resolution, analogWriteResolution(pwm_output_pin), value, refPulseWidthUs, avgPulse, err, errPulseWidth, 100*(ABS(avgPulse-refPulseWidthUs)/refPulseWidthUs) );
                            assertLessOrEqual(err, errPulseWidth);
                        }
                    }

                    // Clamp to analogWriteMaxFrequency(pin)
                    if (freq < analogWriteMaxFrequency(pwm_output_pin) && (freq + freqStep) > analogWriteMaxFrequency(pwm_output_pin)) {
                        freq = analogWriteMaxFrequency(pwm_output_pin);
                    } else if (freq < analogWriteMaxFrequency(pwm_output_pin)) {
                        freq += freqStep;
                    } else {
                        break;
                    }
                }
            }
            // At least 15-bit maximum resolution
            assertMoreOrEqual(resolution, 15);
        }, pwm_output_pin);
        pinMode(pwm_output_pin, INPUT);
    });
}
