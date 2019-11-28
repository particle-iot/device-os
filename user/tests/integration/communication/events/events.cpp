#include "application.h"
#include "test.h"

test(particle_publish_publishes_an_event) {
    Particle.connect();
    waitUntil(Particle.connected);
    Particle.publish("my_event", "event data", PRIVATE);
}
