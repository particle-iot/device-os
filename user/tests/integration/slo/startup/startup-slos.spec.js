// The assertions here help to validate the following:
//
// Device OS releases should guarantee a minimum RAM for user applications
//   Given canonical example firmware
//   100% of Device OS releases will provide at least 60 kB of available RAM for user applications
//
// Device OS releases should guarantee sufficient flash space for user applications
//   The size of a relevant canonical app, which includes library overhead required by Device OS, does not increase more than 15% each release.

suite('Device startup service level objectives (SLOs)');

platform('gen3');
// Enabling system thread, in order to account for its overhead in the measurements
systemThread('enabled');

// Parameters validated by this test
const THRESHOLDS = {
    p2: {
        targetAppFlashSize: 21 * 1024, // 21KB
        targetFreeRam: 3 * 1024 * 1024 // 3MB
    },
    trackerm: {
        targetAppFlashSize: 21 * 1024, // 21KB
        targetFreeRam: 3 * 1024 * 1024 // 3MB
    },
    msom: {
        targetAppFlashSize: 21 * 1024, // 21KB
        targetFreeRam: 3 * 1024 * 1024 // 3MB
    },
    // See rational on this magic number: https://app.clubhouse.io/particle/story/72460/build-device-os-test-runner-integration-test-that-validates-the-minimum-flash-space-and-connects-quickly-slo#activity-72937
    default: {
        targetAppFlashSize: 18105,
        targetFreeRam: 60000
    }
};


test('slo startup stats', async function () {
    const unparsedJson = await this.particle.receiveEvent('startup_stats');
    const startupStats = JSON.parse(unparsedJson);
    console.log("startupStats JSON", startupStats);
    
    // set the device under test to set conditional targets based on platform, etc
    // see device-os-test-runner docs: https://github.com/particle-iot/device-os-test-runner
    const dut = this.particle.devices[0]; 

    ///
    // Assertions against the minimum RAM SLO
    ///
    const thresh = Object.assign({}, THRESHOLDS['default'], THRESHOLDS[dut.platform.name]);

    // show actuals first before assertions
    console.log(`actual_free_mem=${startupStats.free_mem} target_free_mem=${thresh.targetFreeRam} platform=${dut.platform.name}`);
    
    // make free ram assertion
    expect(startupStats.free_mem).to.be.at.least(thresh.targetFreeRam);

    ///
    // Assertions against the minimum flash space SLO
    ///
    console.log(`actual_app_flash_size=${startupStats.app_flash_size} target_app_flash_size=${thresh.targetAppFlashSize} platform=${dut.platform.name}`);
    expect(startupStats.app_flash_size).to.be.below(thresh.targetAppFlashSize);
});
