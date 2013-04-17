/*
 * application.c
 *
 *  Created on: Apr 15, 2013
 *      Author: zsupalla
 */

#include "application.h"

#define LED 9

int i = 0;

void setup() {
	pinMode(LED, OUTPUT);
	digitalWrite(LED, LOW);
}

void loop() {
	delay(1000);
	digitalWrite(LED, HIGH);
	delay(1000);
	digitalWrite(LED, LOW);
	i++;
}
