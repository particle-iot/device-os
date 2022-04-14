#include "Particle.h"

// To verify product creator macros work correctly.
// These values get sent to the cloud on connection and help dashboard.particle.io do the right thing
//
PRODUCT_ID(42);
PRODUCT_VERSION(3);

void setup() {
	pinMode(D7, OUTPUT);
}

void loop() {
	digitalWrite(D7, HIGH);
	delay(100);
	digitalWrite(D7, LOW);
	delay(100);
}
