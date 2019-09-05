#include "Particle.h"

SYSTEM_MODE(MANUAL);

void setup() {
    iBeacon beacon(1, 2, "9c1b8bdc-5548-4e32-8a78-b9f524131206", -55);
    BLE.advertise(beacon);
}

void loop() {

}
