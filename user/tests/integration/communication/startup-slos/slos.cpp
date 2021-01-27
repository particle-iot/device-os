#include "application.h"
#include "test.h"

test(particle_publish_publishes_an_event) {
    Particle.connect();
	static bool published_once = false;

	if (!published_once) {
		if (Particle.connected()) {
			unsigned long delta_millis = millis() - initial_time;
			uint32_t seconds = (uint32_t)(delta_millis / 1000);
			uint32_t free_mem = System.freeMemory();
			String stats = String::format("{\"seconds\": %u, \"ram\": %u }", seconds, free_mem);
			Particle.publish("startup_stats", stats);
			published_once = true;
		}
		else {
			// Joe comment: I don't think this is needed
			// Particle.process();
		}
	}
}