/*
 ******************************************************************************
 *  Copyright (c) 2015 Particle Industries, Inc.  All rights reserved.
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
 ******************************************************************************
 */

#include "application.h"

#include "Serial2/Serial2.h"

SYSTEM_MODE(AUTOMATIC);

uint32_t now = millis();
uint32_t lastFlash = now;

/* executes once at startup */
void setup()
{
	pinMode(RGBR, OUTPUT);
	Serial.begin(9600);
	Serial1.begin(9600);
	Serial2.begin(9600);
	Serial.println("Hi, I'm Serial USB!\r\nPress 1 to talk to Serial1 or 2 to talk to Serial2.");
}

/* executes continuously after setup() runs */
void loop()
{
	now = millis();
	if (now - lastFlash > 200UL) {
		lastFlash = now;
		// This blinks the RED segment of the RGB led
		// NOTE: You cannot use the BLUE or GREEN segment
		// of the RGB anymore while using Serial2.
		digitalWrite(RGBR, !digitalRead(RGBR));
	}

	if(Serial.available()) {
		char c = Serial.read();
		if (c == '1') Serial1.println("Hi, I'm Serial 1!\r\nType in here to talk to Serial USB.");
		else if (c == '2') Serial2.println("Hi, I'm Serial 2!\r\nType in here to talk to Serial USB.");
	}

	// If you type on your Serial1 terminal it will appear on Serial USB
	if (Serial1.available()) {
		char c = Serial1.read();
		Serial.write(c);
	}

	// If you type on your Serial2 terminal it will appear on Serial USB
	if (Serial2.available()) {
		char c = Serial2.read();
		Serial.write(c);
	}
}
