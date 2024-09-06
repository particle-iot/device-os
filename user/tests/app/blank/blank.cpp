/*
 * Copyright (c) 2019 Particle Industries, Inc.  All rights reserved.
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

#include "application.h"

// !!! 
// NOTE: Only supports Argon. Photon2 is not compatible.

#define KEY_PRESSED_DURATION_POWER_ON 2000
#define KEY_PRESSED_DURATION_SLEEP_WAKEUP 100

SYSTEM_MODE(MANUAL);

SerialLogHandler l(LOG_LEVEL_INFO);

// RGB
uint8_t RGB_R = A0;
uint8_t RGB_G = A2;
uint8_t RGB_B = A5;

// MCU Keys
uint8_t PWR_KEY = SCK;
uint8_t VOL_U = MISO;
uint8_t VOL_D = MOSI;

// Power
uint8_t VBAT = A1;
uint8_t V5_3V3_1V8_PWR_EN = D5;

// Module
uint8_t MODEM_RESET_N = A3;
uint8_t MODEM_PWR_KEY = A4;
uint8_t MODEM_READY = D6;
uint8_t MODEM_VOL_U = D4;
uint8_t MODEM_VOL_D = D7;
USARTSerial& MODEM_SERIAL = Serial1;

// USB PD
TwoWire& USB_PD_I2C = Wire;

void keysInit() {
    Log.info("Initializing keys ...");
    pinMode(PWR_KEY, INPUT_PULLUP);
    pinMode(VOL_U, INPUT_PULLUP);
    pinMode(VOL_D, INPUT_PULLUP);
}

system_tick_t keyReadDebounced(uint8_t key) {
    if (digitalRead(key) == LOW) {
        system_tick_t start = millis();
        delay(50);
        if (digitalRead(key) == LOW) {
            while (digitalRead(key) == LOW) {
                delay(5);
            }
            return millis() - start;
        }
    }
    return 0;
}

void rgbInit() {
    Log.info("Initializing RGB ...");
    pinMode(RGB_R, OUTPUT);
    digitalWrite(RGB_R, HIGH);

    pinMode(RGB_G, OUTPUT);
    digitalWrite(RGB_G, HIGH);

    pinMode(RGB_B, OUTPUT);
    digitalWrite(RGB_B, HIGH);
}

void rgbColor(uint8_t r, uint8_t g, uint8_t b) {
    analogWrite(RGB_R, r);
    analogWrite(RGB_G, g);
    analogWrite(RGB_B, b);
}

void moduleInit() {
    Log.info("Initializing core module ...");
    pinMode(MODEM_RESET_N, OUTPUT);
    digitalWrite(MODEM_RESET_N, HIGH); // Put the module in reset state

    pinMode(MODEM_PWR_KEY, OUTPUT);
    digitalWrite(MODEM_PWR_KEY, LOW);

    pinMode(MODEM_VOL_U, OUTPUT);
    digitalWrite(MODEM_VOL_U, LOW);

    pinMode(MODEM_VOL_D, OUTPUT);
    digitalWrite(MODEM_VOL_D, LOW);

    pinMode(MODEM_READY, INPUT);
}

void moduleReset() {
    digitalWrite(MODEM_PWR_KEY, LOW);
    digitalWrite(MODEM_RESET_N, HIGH);
    delay(1000);
    digitalWrite(MODEM_RESET_N, LOW);
}

void modulePowerOn() {
    digitalWrite(MODEM_RESET_N, LOW);
    digitalWrite(MODEM_PWR_KEY, HIGH);
    delay(3000);
    digitalWrite(MODEM_PWR_KEY, LOW);
}

void moduleSleepWake() {
    digitalWrite(MODEM_PWR_KEY, HIGH);
    delay(500);
    digitalWrite(MODEM_PWR_KEY, LOW);
}

void moduleVolUp() {
    digitalWrite(MODEM_VOL_U, HIGH);
    delay(100);
    digitalWrite(MODEM_VOL_U, LOW);
}

void moduleVolDown() {
    digitalWrite(MODEM_VOL_D, HIGH);
    delay(100);
    digitalWrite(MODEM_VOL_D, LOW);
}

bool moduleIsReady() {
    return digitalRead(MODEM_READY) == LOW;
}

void fuelgaugeInit() {
    pinMode(VBAT, INPUT);
}

uint16_t fuelgaugeRead() {
    return analogRead(VBAT);
}

void powerInit() {
    pinMode(V5_3V3_1V8_PWR_EN, OUTPUT);
    digitalWrite(V5_3V3_1V8_PWR_EN, HIGH); // Turn off 5V, 3.3V and 1.8V power
}

void powerEnable(bool enable) {
    if (enable) {
        digitalWrite(V5_3V3_1V8_PWR_EN, LOW);
    } else {
        digitalWrite(V5_3V3_1V8_PWR_EN, HIGH);
    }
}


void setup() {
    waitFor(Serial.isConnected, 5000);
    Log.info("Co-processor started");

    keysInit();
    rgbInit();
    fuelgaugeInit();
    moduleInit();
    powerInit();
    
    MODEM_SERIAL.begin(115200);
    USB_PD_I2C.begin();
}

void loop() {
    static system_tick_t lastRgb = millis();
    if (millis() - lastRgb >= 200) {
        lastRgb = millis();
        rgbColor(random(255), random(255), random(255));
    }

    static system_tick_t lastVbat = millis();
    if (millis() - lastVbat >= 3000) {
        lastVbat = millis();
        Log.info("Vbat: %d", fuelgaugeRead());
    }

    static bool ready = false;
    if (!ready && moduleIsReady()) {
        Log.info("Module is ready");
        ready = true;
    }
    if (ready && !moduleIsReady()) {
        Log.info("module is not ready");
        ready = false;
    }

    system_tick_t pwrKeyPressed = keyReadDebounced(PWR_KEY);
    if (pwrKeyPressed > KEY_PRESSED_DURATION_POWER_ON) {
        Log.info("Power on module");
        modulePowerOn();
    } else if (pwrKeyPressed > KEY_PRESSED_DURATION_SLEEP_WAKEUP) {
        Log.info("module sleep/wakeup");
        moduleSleepWake();
    } else if (pwrKeyPressed > 0) {
        Log.info("PWR key pressed: %d ms", pwrKeyPressed);
    }

    system_tick_t volUKeyPressed = keyReadDebounced(VOL_U);
    if (volUKeyPressed > 2000) {
        Log.info("Enable 5v, 3.3v and 1.8v power");
        powerEnable(true);
    } else if (volUKeyPressed > 100) {
        Log.info("Volume -");
        moduleVolUp();
    }

    system_tick_t volDKeyPressed = keyReadDebounced(VOL_D);
    if (volDKeyPressed > 2000) {
        Log.info("Disable 5v, 3.3v and 1.8v power");
        powerEnable(false);
    } else if (volDKeyPressed > 100) {
        Log.info("Volume +");
        moduleVolDown();
    }
}
