#include "application.h"
#include "test.h"

#include "str_util.h"

namespace {

retained char origAppHash[65] = {};

bool getAppHash(char* buf, size_t size) {
    hal_system_info_t info = {};
    info.size = sizeof(info);
    const int r = system_info_get(&info, 0 /* flags */, nullptr /* reserved */);
    if (r != 0) {
        return false;
    }
    SCOPE_GUARD({
        system_info_free(&info, 0 /* flags */, nullptr /* reserved */);
    });
    for (size_t i = 0; i < info.module_count; ++i) {
        const auto& module = info.modules[i];
        if (module.info.module_function == MODULE_FUNCTION_USER_PART) {
            toHex(module.suffix.sha, sizeof(module.suffix.sha), buf, size);
            return true;
        }
    }
    return false;
}

} // namespace

test(01_disable_resets_and_connect) {
    assertTrue(getAppHash(origAppHash, sizeof(origAppHash)));
    System.disableReset();
    Particle.connect();
    waitUntil(Particle.connected);
}

test(02_flash_binaries_and_reset) {
    // See the spec file
}

test(03_validate_module_info) {
    char appHash[65] = {};
    assertTrue(getAppHash(appHash, sizeof(appHash)));
    // The app hash should not have changed
    assertEqual(strcmp(appHash, origAppHash), 0);
}
