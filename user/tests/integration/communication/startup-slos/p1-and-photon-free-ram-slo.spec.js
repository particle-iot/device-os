// Why this different SLO?
// The official Device OS calls for 60 kB of free RAM, however, P1 and Photon's have never met that quota and won't
// so we exclude it here.
suite('startup free ram SLO for photon and p1');
platform('photon', 'p1');
test('slo startup stats', async function () {
    const target = 45000;
    const unparsedJson = await this.particle.receiveEvent('startup_stats');
    const startupStats = JSON.parse(unparsedJson);
    console.log(`free_mem=${startupStats.free_mem} free_mem_target=${target}`);
    expect(startupStats.free_mem).to.be.at.least(target);
});