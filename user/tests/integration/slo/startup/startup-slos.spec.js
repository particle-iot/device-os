// The assertions here help to validate the following:
//
// Device OS releases should guarantee a minimum RAM for user applications
//   Given canonical example firmware
//   100% of Device OS releases will provide at least 60 kB of available RAM for user applications
//
// Device OS releases should guarantee sufficient flash space for user applications
//   The size of a relevant canonical app, which includes library overhead required by Device OS, does not increase more than 15% each release.

suite('Device startup service level objectives (SLOs)');

platform('gen3', 'gen4');
// Enabling system thread, in order to account for its overhead in the measurements
systemThread('enabled');

const util = require('util')

// Parameters validated by this test
const THRESHOLDS = {
    p2: {
        targetAppFlashSize: 28800,
        targetFreeRam: 3040870,        // 2.9MB
        targetTime: {
            pre_startup: 110000,
            pre_startup_duration: 220,
            startup_duration: 390000,
            setup_duration: 55
        }
    },
    trackerm: {
        targetAppFlashSize: 28800,
        targetFreeRam: 2924544,        // 2.8MB
        targetTime: {
            pre_startup: 110000,
            pre_startup_duration: 220,
            startup_duration: 425000,
            setup_duration: 55
        }
    },
    msom: {
        targetAppFlashSize: 28800,
        targetFreeRam: 2924544,        // 2.8MB
        targetTime: {
            pre_startup: 110000,
            pre_startup_duration: 220,
            startup_duration: 425000,
            setup_duration: 55
        }
    },
    argon: {
        targetTime: {
            pre_startup: 270000,
            pre_startup_duration: 150,
            startup_duration: 32000,
            setup_duration: 170
        }
    },
    boron: {
        targetTime: {
            pre_startup: 490000, // XXX: Look into this, should be under 300000, but there are outliers
            pre_startup_duration: 150,
            startup_duration: 85000,
            setup_duration: 50
        }
    },
    bsom: {
        targetTime: {
            pre_startup: 300000,
            pre_startup_duration: 150,
            startup_duration: 70000,
            setup_duration: 50
        }
    },
    b5som: {
        targetTime: {
            pre_startup: 300000,
            pre_startup_duration: 150,
            startup_duration: 70000,
            setup_duration: 50
        }
    },
    esomx: {
        targetTime: {
            pre_startup: 300000,
            pre_startup_duration: 150,
            startup_duration: 77000,
            setup_duration: 50
        }
    },
    tracker: {
        targetTime: {
            pre_startup: 320000,
            pre_startup_duration: 150,
            startup_duration: 70000,
            setup_duration: 50
        }
    },
    electron2: {
        targetTime: {
            pre_startup: 300000,
            pre_startup_duration: 150,
            startup_duration: 77000,
            setup_duration: 50
        }
    },
    // See rational on this magic number: https://app.clubhouse.io/particle/story/72460/build-device-os-test-runner-integration-test-that-validates-the-minimum-flash-space-and-connects-quickly-slo#activity-72937
    default: {
        targetAppFlashSize: 24400,
        targetFreeRam: 60000,
        targetTime: {
            pre_startup: 320000,
            pre_startup_duration: 150,
            startup_duration: 77000,
            setup_duration: 50
        }
    }
};

test('01_prepare', async function () {

});

test('02_prepare', async function () {

});

test('03_slo_startup_stats', async function () {
    const unparsedJson = device.mailBox.pop().d;
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

    const actualTime = {
        pre_startup: startupStats.time.pre_startup,
        pre_startup_duration: startupStats.time.startup - startupStats.time.pre_startup,
        startup_duration: startupStats.time.setup - startupStats.time.startup,
        setup_duration: startupStats.time.loop - startupStats.time.setup,
    };

    // Startup time assertions
    console.log(`startup_time=${util.inspect(startupStats.time)} actual_time=${util.inspect(actualTime)} target_time=${util.inspect(thresh.targetTime)} platform=${dut.platform.name}`);
    expect(startupStats.time.pre_startup).to.be.below(startupStats.time.startup);
    expect(startupStats.time.startup).to.be.below(startupStats.time.setup);
    expect(startupStats.time.setup).to.be.below(startupStats.time.loop);

    expect(actualTime.pre_startup).to.be.below(thresh.targetTime.pre_startup);
    expect(actualTime.pre_startup_duration).to.be.below(thresh.targetTime.pre_startup_duration);
    expect(actualTime.startup_duration).to.be.below(thresh.targetTime.startup_duration);
    expect(actualTime.setup_duration).to.be.below(thresh.targetTime.setup_duration);
});
