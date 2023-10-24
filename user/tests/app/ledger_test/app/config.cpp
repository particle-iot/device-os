#include <cstdint>

#include <spark_wiring_cloud.h>
#include <platform_headers.h> // For `retained`

#include "config.h"

namespace particle::test {

namespace {

retained Config g_config;
retained uint32_t g_magic;

} // namespace

void Config::setRestoreConnectionFlag() {
    if (!restoreConnection) {
        restoreConnection = Particle.connected();
    }
}

Config& Config::get() {
    if (g_magic != 0xcf9addedu) {
        g_config = {
            .autoConnect = false,
            .restoreConnection = false,
            .removeLedger = false,
            .removeAllLedgers = false,
            .debugEnabled = false
        };
        g_magic = 0xcf9addedu;
    }
    return g_config;
}

} // namespace particle::test
