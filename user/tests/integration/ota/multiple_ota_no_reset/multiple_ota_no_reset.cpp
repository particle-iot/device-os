#include "application.h"
#include "test.h"

namespace {

} // namespace

test(01_disable_resets_and_connect) {
    System.disableReset();
    Particle.connect();
    waitUntil(Particle.connected);
}

test(02_flash_binaries) {
    // See the spec file
}
