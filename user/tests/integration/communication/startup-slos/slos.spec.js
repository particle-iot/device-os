// We cannot include all platforms via `platform('gen2', 'gen3');` because P1 and Photon don't meet the SLO. see the other spec.js file for assertions on that.
// Run `device-os-test list --tags` for a full list of tags that could go in this list as we intend to have a platform meet the SLO long-term
suite('startup free ram SLO for gen3, electron, and tracker');
platform('gen3', 'electron', 'tracker');
test('slo startup stats', async function () {
    const target = 60000;
    const unparsedJson = await this.particle.receiveEvent('startup_stats');
    const startupStats = JSON.parse(unparsedJson);
    console.log(`free_mem=${startupStats.free_mem} free_mem_target=${target}`);
    expect(startupStats.free_mem).to.be.at.least(target);
});