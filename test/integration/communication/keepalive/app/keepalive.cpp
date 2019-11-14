#include "application.h"
#include "unit-test/unit-test.h"

test(default_interval_gets_published_during_the_handshake) {
    Particle.connect();
    waitUntil(Particle.connected);
}

test(particle_keepalive_overrides_the_default_interval) {
    Particle.keepAlive(10); // 10 seconds
    Particle.connect();
    waitUntil(Particle.connected);
}
