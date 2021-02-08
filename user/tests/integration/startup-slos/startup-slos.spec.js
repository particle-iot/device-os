suite('Device startup service level objectives (SLOs)');

// intentionally omit `platform('...');` this test is relevant to ALL Particle platforms

test('slo startup stats', async function () {
    const unparsedJson = await this.particle.receiveEvent('startup_stats');
    const startupStats = JSON.parse(unparsedJson);
    console.log("startupStats JSON", startupStats);

    ///
    // Assertions against the minimum RAM SLO
    ///
    const minimumRAMTargetDefault = 60000;
    const minimumRAMTargetForPhotonAndP1 = 45000;
    let target = minimumRAMTargetDefault; 

    // make exceptions on target for photon/p1
    const dut = this.particle.devices[0]; // see device-os-test-runner for documentation
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
    // TODO

    ///
    // Assertions against the "connects" quickly SLO
    ///
    // TODO
});