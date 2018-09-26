#include "Particle.h"

SYSTEM_MODE(MANUAL);

void panic(const char* topic, const char* data) {
	PANIC(Exit, "This should not happen");
}

void toggleLED(const char* topic, const char* data) {
	RGB.control(true);

	uint8_t brightness = RGB.brightness() ? 0 : 96;
	RGB.brightness(brightness);
}


bool publish = false;

void pressed(system_event_t event, int duration) {
	if (duration) {
		publish = true;
	}
}

void setup() {
	Mesh.on();
	Mesh.connect();
	Mesh.subscribe("panic",  panic);
	Mesh.subscribe("led", toggleLED);

	System.on(button_status, pressed);
}

void loop() {
	if (publish) {
		publish = false;
		// this publish should be ignored
		Mesh.publish("dummy", "data");

		// this publish toggles the LED
		Mesh.publish("led/toggle", "data");
	}
}
