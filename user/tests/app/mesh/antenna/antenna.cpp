/*
 ******************************************************************************
  Copyright (c) 2015 Particle Industries, Inc.  All rights reserved.

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this program; if not, see <http://www.gnu.org/licenses/>.
  ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "application.h"
#include "ssd1306.h"
#include "openthread/platform/radio.h"
#include "ot_api.h"

#define SETUP_BTN                   A0
#define PUB_BTN                     A1

#define ANT_LED                     D7
#define RGB_RED                     D6
#define RGB_GREEN                   D5
#define RGB_BLUE                    D4

SYSTEM_MODE(AUTOMATIC);

Serial1LogHandler logHandler(115200, LOG_LEVEL_ALL);

#if (PLATFORM_ID == PLATFORM_ARGON)
TCPServer server = TCPServer(23);
TCPClient client;
#endif

static bool on = false;

static void toggle_antenna(void) {
    static bool extAnt = false;

    pinMode(ANTSW1, OUTPUT);
#if (PLATFORM_ID == PLATFORM_XENON) || (PLATFORM_ID == PLATFORM_ARGON)
    pinMode(ANTSW2, OUTPUT);
#endif

    if (!extAnt) {
#if (PLATFORM_ID == PLATFORM_ARGON)
        digitalWrite(ANTSW1, HIGH);
        digitalWrite(ANTSW2, LOW);
#elif (PLATFORM_ID == PLATFORM_BORON)
        digitalWrite(ANTSW1, LOW);
#else
        digitalWrite(ANTSW1, LOW);
        digitalWrite(ANTSW2, HIGH);
#endif
        digitalWrite(ANT_LED, HIGH);
        extAnt = true;
    }
    else {
#if (PLATFORM_ID == PLATFORM_ARGON)
        digitalWrite(ANTSW1, LOW);
        digitalWrite(ANTSW2, HIGH);
#elif (PLATFORM_ID == PLATFORM_BORON)
        digitalWrite(ANTSW1, HIGH);
#else
        digitalWrite(ANTSW1, HIGH);
        digitalWrite(ANTSW2, LOW);
#endif
        digitalWrite(ANT_LED, LOW);
        extAnt = false;
    }
}


void meshHandler(const char *event, const char *data) {
#if (PLATFORM_ID == PLATFORM_XENON)
    if (!strcmp("mesh-xenon", event)) {
#elif (PLATFORM_ID == PLATFORM_ARGON)
    if (!strcmp("mesh-argon", event)) {
#endif
        if (!on) {
            on = true;
            digitalWrite(RGB_GREEN, LOW);
        }
        else {
            on = false;
            digitalWrite(RGB_GREEN, HIGH);
        }
    }
}


/* This function is called once at start up ----------------------------------*/
void setup() {
    //Setup the Tinker application here
    pinMode(ANT_LED, OUTPUT);
    digitalWrite(ANT_LED,LOW);

    pinMode(RGB_RED, OUTPUT);
    pinMode(RGB_GREEN, OUTPUT);
    pinMode(RGB_BLUE, OUTPUT);
    digitalWrite(RGB_RED, HIGH);
    digitalWrite(RGB_GREEN, HIGH);
    digitalWrite(RGB_BLUE, HIGH);

    pinMode(SETUP_BTN, INPUT_PULLUP);
    pinMode(PUB_BTN, INPUT_PULLUP);

    oled.init();
    oled.clear();

#if (PLATFORM_ID == PLATFORM_ARGON)
    if (digitalRead(SETUP_BTN) == LOW) {
        delay(100);
        if (digitalRead(SETUP_BTN) == LOW) {
            while (digitalRead(SETUP_BTN) == LOW);
            if (!WiFi.ready()) {
				WiFi.on();
				WiFi.connect();
				while (!WiFi.ready());
            }
        }
    }

    if (WiFi.ready()) {
        oled.println("Wi-Fi connected.");
        oled.println(WiFi.localIP());
    }
#endif

    if (!Mesh.ready()) {
		Mesh.on();
		Mesh.connect();
		while (!Mesh.ready());
    }
    oled.println("Mesh connected.");

#if (PLATFORM_ID == PLATFORM_XENON)
    Mesh.subscribe("mesh-xenon", meshHandler);
#elif (PLATFORM_ID == PLATFORM_ARGON)
    Mesh.subscribe("mesh-argon", meshHandler);
#endif

#if (PLATFORM_ID == PLATFORM_ARGON)
    server.begin();
#endif
}

/* This function loops forever --------------------------------------------*/
void loop() {
    //This will run in a loop
    if (digitalRead(SETUP_BTN) == LOW) {
        delay(100);
        if (digitalRead(SETUP_BTN) == LOW) {
            while (digitalRead(SETUP_BTN) == LOW);
            toggle_antenna();
        }
    }

    if (digitalRead(PUB_BTN) == LOW) {
        delay(100);
        if (digitalRead(PUB_BTN) == LOW) {
            while (digitalRead(PUB_BTN) == LOW);
#if (PLATFORM_ID == PLATFORM_XENON)
            Mesh.publish("mesh-argon", "hello");
#elif (PLATFORM_ID == PLATFORM_ARGON)
            Mesh.publish("mesh-xenon", "hello");
#endif
            if (!on) {
                on = true;
                digitalWrite(RGB_GREEN, LOW);
            }
            else {
                on = false;
                digitalWrite(RGB_GREEN, HIGH);
            }
        }
    }

#if (PLATFORM_ID == PLATFORM_ARGON)
    if (!client.connected()) {
        // if no client is yet connected, check for a new connection
        client = server.available();
    }
    else {
        server.println("Hello from Particle!");
        delay(200);
    }
#endif

    // Indicate that whether the system is not blocked.
    static uint32_t pre_millis;
    uint32_t curr_millis = millis();
    if (curr_millis < pre_millis) {
        pre_millis = curr_millis; // Ticks overflow
    }
    else if ((curr_millis - pre_millis) >= 800) {
        pre_millis = curr_millis;

        if (Mesh.ready()) {
        	int8_t meshRssi = otPlatRadioGetRssi(ot_get_instance());
            if (meshRssi < 0) {
                oled.printf("Mesh: -%d dBm\r\n", -meshRssi);
            }
            else {
                oled.printf("ERROR: %d\r\n", meshRssi);
            }
        }

#if (PLATFORM_ID == PLATFORM_ARGON)
        if (WiFi.ready()) {
            int rssi = WiFi.RSSI();
            if (rssi < 0) {
                oled.printf("WiFi: -%d dBm\r\n", -rssi);
            }
            else {
                oled.printf("ERROR: %d\r\n", rssi);
            }
        }
#endif
    }
}
