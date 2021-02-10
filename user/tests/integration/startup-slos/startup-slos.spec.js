suite('Device startup service level objectives (SLOs)');

// intentionally omit `platform('...');` this test is relevant to ALL Particle platforms

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
    const minimumRAMTargetDefault = 60000;
    const minimumRAMTargetForPhotonAndP1 = 45000;
    let target = minimumRAMTargetDefault; 

    // make exceptions on target for photon/p1
    if (dut.platform.name == 'photon' || dut.platform.name == 'p1') {
        target = minimumRAMTargetForPhotonAndP1;
    }
    // show actuals first before assertions
    console.log(`actual_free_mem=${startupStats.free_mem} target_free_mem=${target} platform=${dut.platform.name}`);
    
    // make free ram assertion
    expect(startupStats.free_mem).to.be.at.least(target);

    ///
    // Assertions against the minimum flash space SLO
    ///

    // See rational on this magic number: https://app.clubhouse.io/particle/story/72460/build-device-os-test-runner-integration-test-that-validates-the-minimum-flash-space-and-connects-quickly-slo#activity-72937
    const flashSLOTarget = 18105;
    console.log(`actual_app_flash_size=${startupStats.app_flash_size} target_app_flash_size=${flashSLOTarget} platform=${dut.platform.name}`);
    expect(startupStats.app_flash_size).to.be.below(flashSLOTarget);

    ///
    // Assertions against the "connects" quickly SLO
    ///
    const connectsQuicklyTarget = 60000;
    console.log(`actual_millis_to_connected=${startupStats.millis_to_connect} target_millis_to_connected=${connectsQuicklyTarget} platform=${dut.platform.name}`);
    expect(startupStats.millis_to_connect).to.be.below(connectsQuicklyTarget);
});