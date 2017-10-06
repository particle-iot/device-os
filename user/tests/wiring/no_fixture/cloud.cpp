#include "application.h"
#include "unit-test/unit-test.h"

test(CLOUD_01_Particle_Connect_Does_Not_Block_In_SemiAutomatic_Mode) {
    Particle.disconnect();
    waitFor(Particle.disconnected, 10000);
    assertTrue(Particle.disconnected());

    // Switch to SEMI_AUTOMATIC mode
    set_system_mode(SEMI_AUTOMATIC);

    Particle.connect();
    assertFalse(Particle.connected());

    waitFor(Particle.connected, 10000);
}

test(CLOUD_03_Restore_System_Mode) {
    set_system_mode(AUTOMATIC);
}
