#include "Particle.h"

// To verify product creator macros work correctly.
// These values get sent to the cloud on connection and help dashboard.particle.io do the right thing
//
PRODUCT_VERSION(3);

void setup() {
	pinMode(LED_PIN_USER, OUTPUT);
}

void loop() {
	digitalWrite(LED_PIN_USER, HIGH);
	delay(100);
	digitalWrite(LED_PIN_USER, LOW);
	delay(100);
}
