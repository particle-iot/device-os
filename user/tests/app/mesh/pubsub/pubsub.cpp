#include "Particle.h"

SYSTEM_MODE(MANUAL);

const static SerialLogHandler logHandler(115200, LOG_LEVEL_ALL);

void panic(const char* topic, const char* data) {
	PANIC(Exit, "This should not happen");
}

void toggleLED(const char* topic, const char* data) {
	RGB.control(true);

	uint8_t brightness = RGB.brightness() ? 0 : 96;
	RGB.brightness(brightness);
	RGB.color(255, 90, 0);
	LOG(INFO, "toggling RGB led %s", data);
}


bool publish = false;

void pressed(system_event_t event, int duration) {
	if (duration) {
		publish = true;
	}
}

void setup() {
	Serial.println("starting mesh test app");
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
