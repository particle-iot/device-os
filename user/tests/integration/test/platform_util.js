'use strict';

const { platforms } = require('@particle/device-constants');

// Returns the platform properties from @particle/device-constants, e.g. the
// USB vendor/product IDs and the base MCU.
//
// Wiring test specs cannot require node modules directly: their real path is
// outside of user/tests/integration, so Node's module resolution never reaches
// its node_modules. This helper's real path is under user/tests/integration,
// so requiring it relatively (through the user/tests/test symlink) works.
function platformForName(name) {
    const info = Object.values(platforms).find((p) => p.name === name);
    if (!info) {
        throw new Error(`Unknown platform: ${name}`);
    }
    return info;
}

module.exports = {
    platformForName
};
