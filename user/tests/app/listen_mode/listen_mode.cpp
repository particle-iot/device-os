#include "Particle.h"

SYSTEM_MODE(MANUAL);

STARTUP(System.enable(SYSTEM_FLAG_WIFITESTER_OVER_SERIAL1));

// SerialLogHandler logHandler(LOG_LEVEL_ALL); // >= 0.6.0
// SerialDebugOutput debugOutput(9600, ALL_LEVEL); // < 0.6.0

void setup() {
   //Network.listen();
   Cellular.listen();
}
