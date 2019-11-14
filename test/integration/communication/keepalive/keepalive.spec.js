suite('Keepalive interval');

platform('gen2', 'gen3');

test('default interval gets published during the handshake', async function() {
	const value = await this.particle.receiveEvent('porticle/device/keepalive');
	expect(value).to.be.above(0);
});

test('Particle.keepAlive() overrides the default interval', async function() {
	const value = await this.particle.receiveEvent('porticle/device/keepalive');
	expect(value).to.equal(10000);
});
