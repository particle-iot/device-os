#include "Particle.h"

#define VERSION 6
PRODUCT_ID(2448);
PRODUCT_VERSION(VERSION);
SYSTEM_MODE(SEMI_AUTOMATIC);

int version = VERSION;
int updatesEnabled;
int updatesForced;
int updatesPending;
int doReset;

int handler(const char*  name, const char*  data) {
        Serial.println(name);
        digitalWrite(D7, !digitalRead(D7));
        return 0;
}

int publish(const char* name) {
   return Particle.publish(name, "data", 60, PRIVATE);
}

int systemReset(const char*) {
   doReset = true;
   return 0;
}


int enableUpdates(const char* name) {
	bool enable = strcmp(name, "0");
	if (enable) {
		System.enableUpdates();
	}
	else {
		System.disableUpdates();
	}
	return enable;
}

void setup() {
   Serial.begin(9600);
#if !USE_SWD_JTAG
   pinMode(D7, OUTPUT);
#endif
   Particle.subscribe("test", handler, MY_DEVICES);
   Particle.function("publish", publish);
   Particle.function("reset", systemReset);
   Particle.function("enableUpdates", enableUpdates);
   Particle.variable("version", version);
   Particle.variable("updatesEnabled", updatesEnabled);
   Particle.variable("updatesForced", updatesForced);
   Particle.variable("updatesPending", updatesPending);
   System.disableUpdates();
   Particle.connect();
}

void updateVariables() {
	updatesEnabled = System.updatesEnabled();
	updatesForced = System.updatesForced();
	updatesPending = System.updatesPending();
}

void loop() {
	updateVariables();
	if (doReset) {
		uint32_t now = millis();
		while (millis()-now < 500) {
			Particle.process();
		}
		System.reset();
	}
}
