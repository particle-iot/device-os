#include "application.h"
#include "test.h"

test(slo_startup_stats)
{
    Particle.connect();
    waitUntil(Particle.connected);
    uint32_t free_mem = System.freeMemory();
    String stats = String::format("{\"free_mem\": %u }", free_mem);
    Particle.publish("startup_stats", stats);
}